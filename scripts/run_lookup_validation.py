"""Automatically lift and compare the pinned name lookup against its native instructions."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import pefile
from lift_automatic import ROOT, OUTPUT, UPSTREAM, DEPS, EXE, EXPECTED, sha, environment
from run_offline_validation import COMPILER


def main():
    if ROOT.drive.upper() != 'E:' or sha(EXE) != EXPECTED:
        raise RuntimeError('Workspace/input identity mismatch')
    folder = OUTPUT / 'lookup'; folder.mkdir(exist_ok=True)
    pe = pefile.PE(str(EXE), fast_load=True)
    pe.parse_data_directories(directories=[1])
    imports = [(e.dll.decode(), s.name.decode()) for e in pe.DIRECTORY_ENTRY_IMPORT
               for s in e.imports if s.address-pe.OPTIONAL_HEADER.ImageBase == 0x17C91E8]
    if imports != [('api-ms-win-crt-string-l1-1-0.dll', '_stricmp')]:
        raise RuntimeError('Lookup import identity changed')
    d = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    records = {a:(b,u) for a,b,u in struct.iter_unpack('<III',pe.get_data(d.VirtualAddress,d.Size))}
    begin=0x443320; end,unwind=records[begin]
    if end != 0x44338B: raise RuntimeError('Lookup extent changed')
    h=pe.get_data(unwind,4)
    if h[0]>>3: raise RuntimeError('Unexpected lookup handler')
    pieces=[(begin,pe.get_data(begin,end-begin),1),(unwind,pe.get_data(unwind,4+((h[2]+1)&~1)*2),2),
            (0x17C91E8,bytes(8),3),(0x219DEC8,bytes(8),3)]
    pack=bytearray(struct.pack('<8s6I',b'SKYSLICE',1,len(pieces),begin,end,unwind,pe.OPTIONAL_HEADER.SizeOfImage))
    for rva,data,kind in pieces: pack.extend(struct.pack('<III',rva,len(data),kind)); pack.extend(data)
    (folder/'lookup.pack').write_bytes(pack)
    sys.path[:0]=[str(DEPS),str(UPSTREAM/'tools/lift')]
    import lift64_cpu
    lifter=lift64_cpu.Lifter(image_size=pe.OPTIONAL_HEADER.SizeOfImage,
        read_va=lambda va,size:pe.get_data(va-0x140000000,size),image_base=0x140000000)
    generated='#include "cpu64.h"\n'+lifter.lift_function(pe.get_data(begin,end-begin),0x140000000+begin)
    if 'RECOMP_TODO(' in generated: raise RuntimeError('Unsupported lookup instruction')
    (folder/'candidate-generated.cpp').write_text(generated)
    (folder/'cpu64.h').write_bytes((UPSTREAM/'runtime/recomp64_cpu/cpu64.h').read_bytes())
    command=[str(COMPILER),'-std=c++20','-O2','-fno-strict-aliasing','-static','-I',str(folder),
             '-I',str(ROOT/'validation'),'-I',str(UPSTREAM/'runtime/recomp64_cpu'),
             str(ROOT/'validation/lookup.cpp'),str(ROOT/'validation/lookup_adapter.cpp'),'-o',str(folder/'candidate.exe')]
    build=subprocess.run(command,cwd=folder,env=environment(),capture_output=True,text=True)
    evidence=dict(stage='lookup',input_sha256=EXPECTED,imports=imports,build_exit_code=build.returncode,
        build_stdout=build.stdout,build_stderr=build.stderr,runs=[])
    if build.returncode==0:
        for negative in (True,False):
            run=[str(folder/'candidate.exe'),str(folder/'lookup.pack')]+(['--negative-control'] if negative else [])
            r=subprocess.run(run,cwd=folder,env=environment(),capture_output=True,text=True)
            evidence['runs'].append(dict(negative_control=negative,exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr))
        evidence['executable_sha256']=sha(folder/'candidate.exe')
    sources=[Path(__file__),ROOT/'scripts/research_config.py',ROOT/'scripts/lift_automatic.py',ROOT/'scripts/run_offline_validation.py',
             ROOT/'validation/lookup.cpp',ROOT/'validation/lookup_adapter.cpp',ROOT/'validation/lookup.h',
             ROOT/'validation/native_slice.h',folder/'candidate-generated.cpp',folder/'cpu64.h',
             UPSTREAM/'tools/lift/lift64_cpu.py',UPSTREAM/'tools/lift/lift32_cpu.py']
    evidence['source_sha256']={str(p.relative_to(ROOT)):sha(p) for p in sources}
    evidence['data_sha256']={'lookup.pack':sha(folder/'lookup.pack')}
    (OUTPUT/'lookup-results.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps(evidence,indent=2))

if __name__=='__main__': main()
