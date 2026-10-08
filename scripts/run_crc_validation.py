"""Pin complete contiguous CRC bodies, including shrink-wrapped unwind fragments."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import pefile
from lift_automatic import ROOT, OUTPUT, UPSTREAM, DEPS, EXE, EXPECTED, sha, environment
from run_offline_validation import COMPILER

RANGES=[(0xA9AE90,0xA9B186),(0xCE2510,0xCE2563),(0xCE2570,0xCE25DE),(0xCE25F0,0xCE26B6)]

def main():
    if ROOT.drive.upper()!='E:' or sha(EXE)!=EXPECTED: raise RuntimeError('Workspace/input mismatch')
    folder=OUTPUT/'crc';folder.mkdir(exist_ok=True)
    pe=pefile.PE(str(EXE),fast_load=True);d=pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    all_runtime=list(struct.iter_unpack('<III',pe.get_data(d.VirtualAddress,d.Size)))
    runtime=[f for f in all_runtime if any(a<=f[0]<b for a,b in RANGES)]
    pieces=[(a,pe.get_data(a,b-a),1) for a,b in RANGES]
    for a,b,u in runtime:
        h=pe.get_data(u,4);flags=h[0]>>3
        if flags&3:raise RuntimeError('Unexpected CRC exception handler')
        size=4+((h[2]+1)&~1)*2+(12 if flags&4 else 0)
        data=pe.get_data(u,size)
        if flags&4 and struct.unpack('<III',data[-12:]) not in runtime:
            raise RuntimeError('CRC chained unwind parent missing')
        pieces.append((u,data,2))
    pieces.extend((rva,pe.get_data(rva,size),2) for rva,size in [(0x19B7550,4096),(0x1A21BE0,1024)])
    a,b=RANGES[0];u=next(u for x,y,u in runtime if x==a)
    pack=bytearray(struct.pack('<8s6I',b'SKYSLICE',2,len(pieces),a,b,u,pe.OPTIONAL_HEADER.SizeOfImage))
    pack.extend(struct.pack('<I',len(runtime)))
    for f in runtime:pack.extend(struct.pack('<III',*f))
    for rva,data,kind in pieces:pack.extend(struct.pack('<III',rva,len(data),kind));pack.extend(data)
    (folder/'crc.pack').write_bytes(pack)
    sys.path[:0]=[str(DEPS),str(UPSTREAM/'tools/lift')]
    import lift64_cpu
    lifter=lift64_cpu.Lifter(image_size=pe.OPTIONAL_HEADER.SizeOfImage,
        read_va=lambda va,size:pe.get_data(va-0x140000000,size),image_base=0x140000000)
    generated='#include "cpu64.h"\n'+'\n'.join(lifter.lift_function(pe.get_data(a,b-a),0x140000000+a) for a,b in RANGES)
    if 'RECOMP_TODO(' in generated:raise RuntimeError('Unsupported CRC instruction')
    (folder/'candidate-generated.cpp').write_text(generated)
    (folder/'cpu64.h').write_bytes((UPSTREAM/'runtime/recomp64_cpu/cpu64.h').read_bytes())
    command=[str(COMPILER),'-std=c++20','-O2','-fno-strict-aliasing','-static','-I',str(folder),
             '-I',str(ROOT/'validation'),'-I',str(UPSTREAM/'runtime/recomp64_cpu'),
             str(ROOT/'validation/crc.cpp'),str(ROOT/'validation/crc_adapter.cpp'),'-o',str(folder/'candidate.exe')]
    build=subprocess.run(command,cwd=folder,env=environment(),capture_output=True,text=True)
    evidence=dict(stage='crc',input_sha256=EXPECTED,ranges=[[hex(a),hex(b)]for a,b in RANGES],
        runtime_functions=[[hex(v)for v in f]for f in runtime],build_exit_code=build.returncode,
        build_stdout=build.stdout,build_stderr=build.stderr,runs=[])
    if build.returncode==0:
        for negative in (True,False):
            run=[str(folder/'candidate.exe'),str(folder/'crc.pack')]+(['--negative-control']if negative else [])
            r=subprocess.run(run,cwd=folder,env=environment(),capture_output=True,text=True)
            evidence['runs'].append(dict(negative_control=negative,exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr))
        evidence['executable_sha256']=sha(folder/'candidate.exe')
    sources=[Path(__file__),ROOT/'scripts/research_config.py',ROOT/'scripts/lift_automatic.py',ROOT/'scripts/run_offline_validation.py',
             ROOT/'validation/crc.cpp',ROOT/'validation/crc_adapter.cpp',ROOT/'validation/crc.h',
             ROOT/'validation/native_slice.h',folder/'candidate-generated.cpp',folder/'cpu64.h',
             UPSTREAM/'tools/lift/lift64_cpu.py',UPSTREAM/'tools/lift/lift32_cpu.py']
    evidence['source_sha256']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    evidence['data_sha256']={'crc.pack':sha(folder/'crc.pack')}
    (OUTPUT/'crc-results.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps(evidence,indent=2))

if __name__=='__main__':main()
