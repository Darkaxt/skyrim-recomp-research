# Native file-object oracle: F04-002

Status: **COMPLETE**. Authority: [file-lifetime-spec.md](file-lifetime-spec.md)
and [daily task plan](task-plan.md). The selected original instructions
now execute the real read-only file workflow in an isolated native harness.
No generated translation was implemented or started in this task.

## Verified behavior

The original object initializes, opens a fixture, reports its size, reads and
seeks, transfers ownership, permits closing the moved-from source before using
the destination, and closes its one live handle. Repeated close performs no
additional OS close. Both initialization forms work with guarded storage at
either boundary. Object padding, unused destination storage, file names, output
extents and unrelated buffers remain unchanged where required.

The corpus uses lengths 0, 1, 31, 4097 and 65536 with zero, alternating and
deterministic patterns. It covers zero/exact/short/oversized reads, EOF, all three
seek origins and reuse. Two independently relocated mappings execute the same
workflow with normal and reversed pack-piece ordering. All seven original
unwind records resolve through the Windows runtime; the four leaf/tail routines
have no invented unwind record. The mapper's code and tables remain executable
and read-only on their shared page, and all 188 mapper inputs per mapping match
independently interpreted return blocks.

Missing file, missing parent and a real exclusive-handle sharing conflict produce
the expected native errors and mapped status. Negative seek leaves its caller
output unchanged. Read after close zeros its count and preserves its buffer;
size after close zeros its output and returns the routine's fixed status 7,
without calling the error mapper. Early cleanup at seven lifecycle points,
followed by fresh open/read/close, releases the actual resource each time.

The checked corpus comprises 120 normal workflows, 28 failure workflows and 56
partial-cleanup/retry workflows. Independent trace reconciliation accounts for
236 created and closed native handles across 204 workflows. These are fixture
coverage observations; they are not a measure of whole-engine completion.

The authored call probe checks nonvolatile GPRs, XMM6–XMM15, stack/canary state
and MXCSR control preservation. An authored RBX-clobber control must produce
the corresponding failure bit. The incorrect-read control corrupts a byte
returned by a successful real ReadFile; the oracle rejects it specifically for
incorrect content. No successful API substitutes are used.

## Challenges and reusable solutions

| Challenge | Cause and solution | Reuse boundary |
|---|---|---|
| Mapper table removes execution from its page | The loader assigned each piece's permission directly, so later read-only data could overwrite code permission. Accumulate all page requirements, derive final protection once, and test both piece orders. Code plus read-only data is RX; code plus mutable data is rejected. Flush every executable page. | Use for future sparse instruction packs with mixed code/data pages. This is not a full PE loader. |
| Rejected pack leaks its reserved image | A constructor failure bypassed the object's destructor. Release the allocation, and any successful registration, in the constructor's failure path. A truncated-pack regression queries the actual allocation state after rejection. | Constructor acquisition/failure cleanup must be verified separately from successful teardown. |
| Trace bookkeeping changes error state | Logging or allocation may overwrite thread-local last error. Capture the real API's error immediately and restore it after bookkeeping; test a nontrivial sentinel. Check required failure errors exactly and avoid inventing guarantees for unspecified success-state errors. | Apply to future native bridges that observe OS-thread state. |
| Real handles differ and may be recycled | Assign an identity to each successful creation, track live generations and verify call/close order. Check the object against its actual handle and use GetHandleInformation after teardown to confirm release. | Future native/generated comparisons need semantic identities for opaque handles and pointer regions; scalar values and defined memory remain exact. |
| Independent mapper expectations without guessed SDK enums | Hash the actual selector/target tables and interpret the six simple return blocks separately from executing the original mapper. Check the complete bounded input range and unsigned fallback cases. | Do not replace build-specific numerical evidence with foreign enum names. |
| A new state assertion compared different lifecycle points | The stricter closed-size check initially compared against the still-open object snapshot. Isolating its error and state checks confirmed ERROR_INVALID_HANDLE was correct. Snapshot immediately after intentional close, then require read/size failure to preserve that input state. Preserve both failed results. | Define permitted writes relative to each operation's input state, including intentional intervening ownership changes. |
| Shared loader repair might regress earlier slices | Rebuild and execute the existing functional gates once with the final loader, including the disabled-cleanup failure control. Subsequent verification reconciles current source/artifact hashes without rerunning unchanged work. | Broaden testing for an actual shared dependency change; use focused feedback for harness-only changes. |

## Reproduction and evidence

Run in the private research checkout using its existing pinned dependencies:

```powershell
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:PYTHONPATH = (Join-Path (Get-Location) 'local/tools/translation-python')
python scripts/run_file_lifetime_native.py
python scripts/verify_file_lifetime_native.py
```

The driver keeps packs, fixtures, executables and traces under
`local/automatic/file-lifetime/`. It checks the original EXE hash before and
after execution, validates code/table/unwind bytes against the selected closure,
and confirms all six import identities directly from the PE. Existing fixture
content must match before reuse; ambiguous existing files are not overwritten.

Recovery integration subsequently moved the required range/hash metadata into
`validation/file_lifetime_manifest.json`. Reproduction no longer depends on the
private Ghidra/closure export. The historical loader-control results are retained
in `docs/file-lifetime-historical-controls.json`; the current verifier distinguishes
that recorded history from freshly executed current controls.

Primary private records are `native-results.json`, `native-trace.csv` and
`F04-002-verification.json` in that directory. They retain commands, build/run
results, source/input/artifact hashes, exact corpus summaries and trace ownership
reconciliation. `loader-before-fix.json` retains all three reproduced loader
failures; `loader-after-fix.json` is the historical first green result before
the later indentation-only source change. Current loader results and hashes are
in `native-results.json`. The two `failed-pre-close-snapshot*.json` records
preserve the subsequently diagnosed harness assertion failure.

The shared regression command was `python research.py --run`; its inspected
results cover projection, callback dispatch, frustum exception cleanup and
retry, its disabled-cleanup failure control, real-CRT lookup and CRC. Its log is
`local/logs/F04-002-existing-gates.log`; current gate evidence is
`local/automatic/next-slices-verification.json`. The final native verifier
rechecks that these results still apply to the current shared source.

All generated evidence stays private. Game/mod inputs remain read-only. No
game process, game/mod DLL, entry point, save/profile mutation, foreground window
or public publication is part of this gate.

## Remaining scope

F04-002 acceptance criteria remaining: none. Blockers: none. Tracked deferrals:
none. The next required stage, **C01-001**, must automatically translate the
selected entries and integrate dispatch, tables and typed native bridges before
matching the normal corpus. **C01-002** owns generated failure behavior and
broken-ownership/close controls; **T02-001** owns whole-slice reconciliation.
These required later stages remain NOT STARTED and are not claims of translated
behavior from this native-only gate.

This proves an offline native file-handle lifetime oracle. Engine heap behavior,
archives/VFS, broad streaming, general C++ exception behavior, gameplay and
parallel engine pipelines remain unproven by this slice.

Measured autonomous elapsed for this task: **23 minutes**. See
[the run log](progress.md) for timing boundaries, machine execution
times and unmeasured human/category effort.
