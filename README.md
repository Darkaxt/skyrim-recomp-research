# Skyrim recompilation research

Offline feasibility experiments comparing automatically lifted Windows x64
Skyrim routines with their original instructions. The current gates pass after
explicit instruction-semantic and cleanup repairs. This is an unofficial,
version-pinned research project; it is not a playable engine replacement.

| Proven boundary | Current result |
|---|---|
| Projection | Exact Boolean, memory, ABI and FP-state agreement after COMISS repair |
| Actor-value callback | Exact return, callback trace and table/flag mutation agreement |
| Frustum/cache initialization | Exact serial state and TLS; controlled six-thread initialization; six exception positions and retries |
| Actor-value name lookup | Exact indices, duplicate precedence and real CRT comparison traces |
| CRC/hash | Four real routines; exact guarded buffer and integer results |

Read the [short testing plan](docs/testing-plan.md),
[scope and acceptance criteria](docs/next-slices-spec.md),
[measured results and reusable solutions](docs/next-slices-results.md), and
[project evolution](docs/project-evolution.md).

## Reproduce locally

Use Windows x64, Python 3.12+ with pip, Git, GCC/MinGW g++ (tested 15.2),
and Visual Studio C++ x64 tools (tested MSVC 14.44). Put this checkout on **E:**;
tools, dependencies, generated code, evidence, temporary files and caches stay
there. Existing compilers can reside elsewhere. Supply your own executable with
the exact hash in [provenance](THIRD_PARTY_NOTICES.md); no game files are included.

```powershell
Set-Location E:\skyrim-recomp-research
$env:SKYRIM_EXE = 'D:\Steam\steamapps\common\Skyrim Special Edition\SkyrimSE.exe'
# Set SKYRIM_GXX to g++.exe if it is not on PATH.
# Set SKYRIM_VCVARS to vcvars64.bat if vswhere cannot locate MSVC.
python research.py --setup --run
```

The command fetches pinned pcrecomp/Capstone/pefile dependencies, checks the input,
extracts only declared slices, lifts them, compiles and runs native differential
harnesses with negative controls. It fails if a functional gate or required
control fails. It never launches Skyrim or loads mod DLLs. No Ghidra installation
is needed to reproduce these pinned experiments. Run `python research.py` to
verify retained evidence against current source/artifact hashes without rebuilding.

Raw logs, source hashes, generated bodies, input packs and executables remain
under ignored `local/`; the integrated report is
`local/automatic/next-slices-verification.json`. Do not add that directory to Git.
Public history contains authored tooling/harnesses/docs and measured summaries,
with no original, manually reconstructed, decompiled or lifted game bodies.

## What remains unproven

Whole-engine boot, renderer/audio/input integration, allocation, saves, native
mod compatibility and another architecture are untested. Cleanup support covers
one real cleanup-only scope; nested destruction/catch matching/general SEH are
unproven. Concurrency covers one immutable camera/frame initialization. Lookup
uses ASCII fixtures in the initial C locale. These selected slices do not yield
a whole-engine success percentage or schedule estimate.

For new slices, pin their complete dependencies and establish the original
instruction oracle and an effective negative control before broadening support.
