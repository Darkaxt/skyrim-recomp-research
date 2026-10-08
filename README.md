# Skyrim recompilation research

Exploring whether Skyrim's engine can be recovered into an inspectable,
testable implementation, with the long-term aim of making engine fixes,
deeper QA and changes to engine behavior easier to develop.

The first step is validating automatic translation: run selected original x64
routines and their generated counterparts against the same inputs, then compare
results, memory changes and interactions with the runtime.

## Current findings

Five focused experiments pass their native differential checks:

| Experiment | Verified behavior |
|---|---|
| Camera projection | Boolean results, memory changes, calling convention and floating-point state |
| Actor-value callbacks | Return values, callback order and table/flag changes |
| Frustum/cache initialization | Serial state, thread-local storage, controlled concurrent initialization and exception cleanup/retry |
| Actor-value name lookup | Indices, duplicate precedence and calls to the real runtime string comparison |
| CRC/hash | Four routines, including guarded buffers and integer inputs |

These experiments establish useful translation and runtime boundaries. They do
not yet demonstrate a complete engine subsystem or a playable replacement.
The [results and reusable solutions](docs/next-slices-results.md) explain the
challenges, repairs and limits of each experiment.

## Research direction

The next work is to connect these proofs into larger workflows: allocation and
object lifetime, resource loading, scene processing, simulation and persistence.
That requires recovering data and dependency contracts alongside function bodies.
Mod compatibility and changes to scheduling will need their own validation;
translation alone does not establish safe parallel execution.

Follow the [testing plan](docs/testing-plan.md) and
[project evolution](docs/project-evolution.md) for the research sequence and
completed milestones.

## Try the experiments

The offline harnesses are reproducible on Windows x64 with a compatible local
copy of Skyrim Special Edition. Follow the
[reproduction guide](docs/getting-started.md) for prerequisites, supported game
version and setup, then run:

```powershell
python research.py --setup --run
```

No Ghidra installation or live gameplay is needed for these experiments.

This is an unofficial research project. Authored tooling and harnesses are
[MIT licensed](LICENSE); see [provenance and third-party notices](THIRD_PARTY_NOTICES.md)
for input and dependency details.
