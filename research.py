"""Public entry point. Install pinned dependencies and run offline gates entirely on E:."""
import sys
sys.dont_write_bytecode = True
import argparse
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parent

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--setup',action='store_true');parser.add_argument('--run',action='store_true')
    args=parser.parse_args()
    if ROOT.drive.upper()!='E:':raise RuntimeError('Place this research checkout on E: before running')
    temporary=ROOT/'local/tmp';temporary.mkdir(parents=True,exist_ok=True)
    deps=ROOT/'local/tools/translation-python';source=ROOT/'local/sources/pcrecomp'
    env=os.environ.copy();env.update(TEMP=str(temporary),TMP=str(temporary),PYTHONDONTWRITEBYTECODE='1',PYTHONPATH=str(deps),
        PIP_CACHE_DIR=str(ROOT/'local/cache/pip'),PIP_DISABLE_PIP_VERSION_CHECK='1',PYTHONPYCACHEPREFIX=str(ROOT/'local/cache/pycache'))
    commit='0a138b6ba423af7dd1d6955fb763dc2bfd2b058e'
    if args.setup:
        source.parent.mkdir(parents=True,exist_ok=True)
        if not source.exists():
            subprocess.run(['git','clone','--no-checkout','https://github.com/sp00nznet/pcrecomp',str(source)],cwd=ROOT,env=env,check=True)
            subprocess.run(['git','-C',str(source),'checkout','--detach',commit],env=env,check=True)
        actual=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True,env=env).strip()
        dirty=subprocess.check_output(['git','-C',str(source),'status','--porcelain'],text=True,env=env)
        if actual!=commit or dirty:raise RuntimeError('Existing lifter differs; preserve it and use a clean research checkout')
        # Avoid overwriting an existing dependency tree. A separate clean checkout
        # is the explicit recovery procedure for mismatched dependency versions.
        if deps.exists() and any(deps.iterdir()):
            probe=subprocess.run([sys.executable,'-c',"import capstone,pefile;assert capstone.__version__=='5.0.7';assert pefile.__version__=='2024.8.26'"],cwd=ROOT,env=env)
            if probe.returncode:raise RuntimeError('Existing dependencies differ; use a clean research checkout')
        else:
            deps.mkdir(parents=True,exist_ok=True)
            subprocess.run([sys.executable,'-m','pip','install','--no-cache-dir','--no-compile','--target',str(deps),
                'capstone==5.0.7','pefile==2024.8.26'],cwd=ROOT,env=env,check=True)
    command=[sys.executable,str(ROOT/'scripts/verify_next_slices.py')]+(['--run']if args.run else [])
    if args.run or not args.setup:return subprocess.call(command,cwd=ROOT,env=env)
    return 0

if __name__=='__main__':raise SystemExit(main())
