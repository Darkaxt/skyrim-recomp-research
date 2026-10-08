"""Explicit local input/tool paths; generated files and caches stay in this E: workspace."""
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
EXE = Path(os.environ.get('SKYRIM_EXE', r'D:\Steam\steamapps\common\Skyrim Special Edition\SkyrimSE.exe'))
EXPECTED = '846efccf0c1374d71f892907f46549560f2fcb0a75cb87a3eed438baa0f1402f'
COMPILER = Path(os.environ.get('SKYRIM_GXX') or shutil.which('g++.exe') or 'g++.exe')

def vcvars():
    explicit = os.environ.get('SKYRIM_VCVARS')
    if explicit: return Path(explicit)
    locator = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    if locator.exists():
        import subprocess
        found = subprocess.check_output([str(locator), '-latest', '-products', '*',
            '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
        if found: return Path(found) / 'VC/Auxiliary/Build/vcvars64.bat'
    raise RuntimeError('MSVC C++ toolchain missing; set SKYRIM_VCVARS to vcvars64.bat')
