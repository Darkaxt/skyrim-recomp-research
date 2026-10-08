"""Reproduce current automatic gates; fail on any unexpected result or stale evidence."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
from lift_automatic import ROOT, OUTPUT, UPSTREAM, DEPS, COMMIT, EXE, EXPECTED, sha, environment

def check(ok,message):
    if not ok: raise RuntimeError(message)

def execute(script,*arguments):
    logs=ROOT/'local/logs';logs.mkdir(exist_ok=True)
    result=subprocess.run([sys.executable,str(ROOT/'scripts'/script),*arguments],cwd=ROOT,
                          env=environment(),capture_output=True,text=True)
    (logs/(script+'-'+ '-'.join(arguments).replace('--','')+'.log')).write_text(result.stdout+result.stderr)
    check(result.returncode==0,f'Driver failed: {script}: {result.stderr}')

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--run',action='store_true');args=parser.parse_args()
    check(ROOT.drive.upper()=='E:','All outputs must stay on E:')
    (ROOT/'local/tmp').mkdir(parents=True,exist_ok=True);OUTPUT.mkdir(exist_ok=True)
    check(sha(EXE)==EXPECTED,'Pinned game input changed')
    check(subprocess.check_output(['git','-C',str(UPSTREAM),'rev-parse','HEAD'],text=True).strip()==COMMIT,'Lifter commit changed')
    check(not subprocess.check_output(['git','-C',str(UPSTREAM),'status','--porcelain'],text=True),'Lifter checkout modified')
    sys.path.insert(0,str(DEPS));import capstone
    check(capstone.__version__=='5.0.7','Capstone version changed')
    if args.run:
        for stage in ('projection','dispatch'):
            execute('lift_automatic.py',stage)
            first=sha(OUTPUT/f'{stage}-stock.c');execute('lift_automatic.py',stage)
            check(first==sha(OUTPUT/f'{stage}-stock.c'),'Generation is not deterministic')
        execute('run_automatic_validation.py','--stage','projection','--variant','compare')
        execute('run_automatic_validation.py','--stage','dispatch','--variant','stock')
        execute('run_automatic_frustum.py','--variant','cleanup')
        execute('run_automatic_frustum.py','--variant','cleanup','--disable-cleanup')
        execute('run_lookup_validation.py');execute('run_crc_validation.py')
    summaries={};evidence_hashes={}
    labels=('projection-compare','dispatch-stock','frustum-cleanup','frustum-cleanup-disabled','lookup','crc')
    for label in labels:
        path=OUTPUT/f'{label}-results.json';r=json.loads(path.read_text());folder=OUTPUT/label
        evidence_hashes[label]=sha(path)
        check(r['input_sha256']==EXPECTED,f'{label} wrong input')
        for name,digest in r['source_sha256'].items():check(sha(ROOT/name)==digest,f'{label} stale source: {name}')
        for name,digest in r['data_sha256'].items():check(sha(folder/name)==digest,f'{label} changed data: {name}')
        if 'builds' in r:
            check(len(r['builds'])==3 and all(x['exit_code']==0 for x in r['builds']),f'{label} build failed')
            check(r['loader_tls_template_verified'],f'{label} invalid TLS')
            for name,digest in r['hashes'].items():check(sha(folder/name)==digest,f'{label} executable changed')
        else:
            check(r['build_exit_code']==0,f'{label} build failed')
            check(sha(folder/'candidate.exe')==r['executable_sha256'],f'{label} executable changed')
        negative,actual=r['runs']
        check(negative['negative_control'] and negative['exit_code']==1 and 'differential failure' in negative['stderr'].lower(),f'{label} invalid negative control')
        check(not actual['negative_control'],f'{label} wrong actual run')
        if label.endswith('-disabled'):
            checkpoints=[json.loads(x)for x in actual['stdout'].splitlines()]
            check(actual['exit_code']==2 and actual['stderr']=='Exception abort cleanup failed\n'
                and checkpoints[-1]['candidate']==1 and checkpoints[-1]['position']==1 and checkpoints[-1]['guard']==-1,
                'Disabled cleanup did not reproduce actual guard failure')
            summaries[label]={'expected_failure_verified':True,'guard':-1};continue
        check(actual['exit_code']==0 and not actual['stderr'],f'{label} functional failure: {actual["stderr"]}')
        lines=[json.loads(x)for x in actual['stdout'].splitlines()];summary=lines[-1]
        check(summary['status']=='pass',f'{label} missing pass')
        if label=='projection-compare':check(summary['trials']==255600,'Projection corpus changed')
        if label=='dispatch-stock':check(summary['trials']==405504,'Dispatch corpus changed')
        if label=='lookup':check(summary==dict(status='pass',trials=1336,table_entries=164,exact_trace=True,guard_pages=True,real_crt=True),'Lookup corpus changed')
        if label=='crc':check(summary==dict(status='pass',buffer_trials=20340,integer_trials=1564,routines=4,seeds=6,alignments=16,patterns=5,guard_pages=True,max_length=65536),'CRC corpus changed')
        if label=='frustum-cleanup':
            check(summary['serial_trials']==57344 and summary['reconstruction_cleanup_and_retry']
                and summary['contending_threads']==6 and summary['waiters']==5,'Frustum corpus changed')
            states=[x for x in lines if x.get('checkpoint')=='exception_state']
            check([(x['candidate'],x['position'])for x in states]==[(c,p)for c in (0,1)for p in range(1,7)]
                  and all(x['caught']==1 and x['guard']==0 and x['lock_recursion']==0 for x in states),'Cleanup states incomplete')
        summaries[label]=summary
    result=dict(status='PASS',input_sha256=EXPECTED,pcrecomp_commit=COMMIT,gates=summaries,evidence_sha256=evidence_hashes,
                verifier_sha256=sha(__file__),game_launched=False)
    (OUTPUT/'next-slices-verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__=='__main__':main()
