# Project evolution

## Static survey and manual baseline

The initial survey identified an analyzable Windows x64 image and selected small
boundaries with explicit memory, call, unwind and TLS dependencies. Ghidra labels
were hypotheses, checked against current bytes. Three authored manual candidates
passed native differential harnesses for projection, actor-value callback and
frustum/cache initialization. Manual reconstructed bodies and private analysis
are intentionally absent from public history.

## Automatic generation: failures retained as evidence

Unmodified pcrecomp generated these slices without TODOs, but stock projection
lost COMISS's quiet-NaN invalid flag. Stock frustum exposed the same flag issue.
A generic COMISS helper made projection pass 255600 exact comparisons. The stock
callback passed 405504, including callbacks that mutate flags or replace a table.

Comparison-only frustum repair still differed in a NaN cache sign bit. Routing
scalar arithmetic through operand-ordered SSE instructions restored exact serial
state. The precise optimizer transformation causing the older sign difference
was not isolated. Strict frustum passed 57344 serial checks and controlled TLS/
concurrency, then failed exception cleanup with an initialization guard stuck at
-1. A build and zero unsupported-opcode count had not proved correctness.

These are historical experiment results, not current success claims for stock
or comparison-only stateful variants. Their failures are not reclassified.

## Cleanup and additional modules

The next test plan started with exception cleanup. Metadata-driven cleanup now
runs automatically lifted actions when MSVC C++ exceptions cross generated
frames. All six injected positions and retries pass; disabling cleanup restores
the actual guard failure. The native CRT synchronization protocol remains real.

Actor-value lookup subsequently passed exact CRT traces and guarded fixtures.
Four CRC helpers passed guarded input/output, alignment, seed and integer tests.
See [current results](next-slices-results.md) for corpus and reuse boundaries.

## Public reproducibility

Public history begins with this sanitized authored source set. It deliberately
does not import private research history, which contains manual game-derived
reconstructions. `research.py --setup --run` reproduces current gates from locally
supplied compatible input. Dependencies are pinned; original/game-derived output
stays ignored. Extraction helpers retain only preparation functionality in the
public export; private manual-candidate runners are omitted.

Next research should test genuinely new boundaries, such as nested cleanup and
object ownership, before drawing conclusions about larger engine subsystems.
Those are future research directions, not claims delivered by these gates.

## Connected file ownership oracle

F04-001 selected a real 11-entry file-object closure. F04-002 now verifies its
native read-only open/size/read/seek/transfer/close workflow, including real OS
failures and early cleanup/retry. The shared sparse loader now combines page
permissions across pieces and releases failed mappings; existing automatic gates
still pass. A corrupted-read control is rejected and actual handle release is
independently reconciled. See [results and lessons](file-lifetime-native.md).

The extraction metadata is tracked, so a fresh checkout can reproduce the native
oracle from the pinned executable without the private analysis database.
Automatic translation of this connected path remains NOT STARTED (C01-001).
