# Reproduce the offline experiments

## Prerequisites

- Windows x64 and Python 3.12+ with pip.
- Git and GCC/MinGW `g++` (tested with 15.2).
- Visual Studio C++ x64 tools (tested with MSVC 14.44).
- Your own Steam Skyrim Special Edition executable, version **1.7.104.0**,
  with SHA256
  `846efccf0c1374d71f892907f46549560f2fcb0a75cb87a3eed438baa0f1402f`.

The current runners require the checkout on **E:** and direct generated files,
temporary files, dependencies and caches there. This is a constraint of the
current research tooling. Existing compilers and the original game installation
can reside elsewhere. Other executable hashes fail the input gate.

## Run

From an E: checkout in PowerShell, set `SKYRIM_EXE` to your executable's actual
path. For example:

```powershell
git clone https://github.com/Darkaxt/skyrim-recomp-research.git E:\skyrim-recomp-research
Set-Location E:\skyrim-recomp-research
$env:SKYRIM_EXE = 'D:\Steam\steamapps\common\Skyrim Special Edition\SkyrimSE.exe'
python research.py --setup --run
```

If GCC is absent from PATH, set `SKYRIM_GXX` to `g++.exe`. If automatic MSVC
discovery fails, set `SKYRIM_VCVARS` to `vcvars64.bat`.

Setup fetches pinned pcrecomp, Capstone and pefile dependencies. The run checks
input identity, extracts the declared slices, translates them, builds the native
differential harnesses and runs the functional gates and negative controls.
A failed gate or required control makes the command fail. It does not launch
Skyrim or load mod DLLs. Ghidra is not needed for this reproduction workflow.

To verify retained evidence against current source and artifact hashes without
rebuilding, run:

```powershell
python research.py
```

## Inspect the evidence

The integrated report is `local/automatic/next-slices-verification.json`.
Detailed logs, generated code, input packs and harness executables also stay in
the ignored `local/` directory. Preserve them locally when investigating a
failure; they may contain material extracted from your game and must not be
committed or distributed.

The [acceptance criteria](next-slices-spec.md) and
[measured results](next-slices-results.md) describe what the gates prove and
their limits. See [third-party notices](../THIRD_PARTY_NOTICES.md) for provenance.

## Native file-object workflow

After the original gates pass, run the commands in the [recovery guide](recovery.md)
to recreate and verify the native file oracle. This gate uses the tracked
range/hash manifest, not a private Ghidra database. It does not yet translate
the connected path.
