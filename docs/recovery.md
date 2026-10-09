# Recovery

The public repository preserves authored source, specifications, task progress,
planning reports and the metadata needed to extract the tested slices. The
file-lifetime manifest contains ranges and hashes, not original instructions or
table bytes. A fresh checkout can reproduce the oracle without a private Ghidra
project or static closure export.

## Restore public research

Clone this repository on an E: drive, install the prerequisites in the
[reproduction guide](getting-started.md), and supply your own executable with the
documented hash. Then run the original gates and the native file oracle:

```powershell
python research.py --setup --run
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:PYTHONPATH = Join-Path (Get-Location) 'local/tools/translation-python'
python scripts/run_file_lifetime_native.py
python scripts/verify_file_lifetime_native.py
```

The last verifier checks current native evidence and the original shared-loader
gates. Historical pre-repair control results are preserved as recorded metadata;
they are not presented as freshly rerun old defects. Generated native packs,
tables, traces and executables remain under ignored `local/`.

The public source does not preserve the original executable, private raw evidence,
Ghidra database, local Git history, installed mod collections or saves. Recreating
tests also does not recreate historical observations byte-for-byte: handles,
addresses and timings vary. Historical private evidence needs its own backup.

## Full private recovery

The project owner maintains a separate private recovery repository containing
the private research history and a compressed snapshot of evidence and the
Ghidra project. That repository is independent of the public fork network.
Its recovery manifest records every archived file's path, size and SHA256, the
archive-part hashes and the source revision. Its README describes restoration
and verification; source and snapshot recovery are checked from a fresh remote
clone before reporting a backup complete.

Downloaded tools/source dependencies and expendable caches are reinstalled from
the documented/pinned dependencies. The private snapshot does not replace a
backup of the original game, mod collections or saves. The original pinned game
build must be supplied separately if it can no longer be obtained from Steam.

A remote backup protects against local disk loss. Only revisions that have
actually been pushed are recoverable there. The owner authorized a private
recovery checkpoint after each completed daily task. Incremental archives retain
changed evidence and deletions over the verified full baseline; unchanged Ghidra
data is reused. The restore command reconstructs the latest checkpoint state.
Public publication remains separate. An interrupted or unsuccessful backup must
be reported explicitly; unpushed work is still exposed to local disk loss.
