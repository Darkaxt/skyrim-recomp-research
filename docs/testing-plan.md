Exception cleanup is the next gate to resolve.

Work through these gates in order, using isolated original instructions as the
oracle and automatically lifted code as the candidate. No gameplay is required.

| Gate | Test | Pass condition | Status |
|---|---|---|---|
| 1. Exception cleanup | Recover cleanup-only MSVC metadata and run its cleanup actions when native C++ exceptions cross generated frames. | Existing frustum serial/TLS/concurrency checks plus all six constructor exception positions and successful retries; a disabled-cleanup control must fail. | COMPLETE |
| 2. Actor-value name lookup | Translate the real lookup loop and bridge its string-comparison import. | Exact index/sentinel, input preservation and comparison trace for first/middle/last/missing names, mixed case, duplicate entries and guarded strings. | COMPLETE |
| 3. CRC/hash module | Translate real engine CRC helpers and their table dependencies. | Exact hashes and bounded memory access for empty/short/long buffers, varied seeds, alignments and bit patterns; reject a corrupted candidate. | COMPLETE |
| 4. Public project evolution | Recheck all gates, document results/repairs, prepare an explicit public file set and create a public GitHub repository. | All preceding gates pass; public sources reproduce from locally supplied inputs; no game binaries, lifted/decompiled bodies, packs, Ghidra databases, credentials or personal mod inventories in any public commit; published files match the reviewed local set. | ACTIVE |

Authority and full acceptance criteria: `next-slices-spec.md`. One ACTIVE stage.
Public creation/push is authorized by the user's conditional request and occurs
only after the functional gates pass. Authored project sources and experiment
summaries are publishable deliverables; game-derived artifacts stay local.
