# Next slice evidence and reusable solutions

## Exception cleanup — verified

The cleanup variant passes the unchanged native frustum harness: 57344 exact
serial comparisons, real loader TLS, six threads with five observed waiters,
exceptions at each of six constructor positions, guard release and successful
retries. Disabling cleanup fails at the first candidate throw with guard -1;
the ordinary incorrect-candidate control also fails. Evidence is retained in
`local/automatic/frustum-cleanup-results.json` and
`local/automatic/frustum-cleanup-disabled-results.json`.

Challenge: upstream lifting does not carry MSVC cleanup state across generated
C++ frames. Registering original unwind records on host-generated functions
would describe the wrong stack. Solution: decode original FuncInfo/IP states,
capture virtual CPU state before calls, use RtlVirtualUnwind on that original
virtual stack to recover the establisher frame, and dispatch automatically
lifted cleanup actions in decreasing unwind-state order before rethrowing.
The actual original CRT abort helper performs the guard release.

Challenge: the cleanup leaf has no runtime-function entry. Solution: accept a
bounded decoded LEA-to-RCX/direct-tail-jump leaf form; reject other unbounded
forms instead of guessing an extent. Non-leaf extents come from .pdata.

Limits: one real cleanup scope, one action, native MSVC C++ exceptions in one
immutable-camera/frame initialization scenario. Catch matching, nested object
destruction, SEH, arbitrary leaf funclets and general exception conformance have
not been demonstrated. The state-chain implementation is bounded and validated
structurally; no broader runtime proof is claimed.

## Actor-value name lookup — verified

The unchanged automatically lifted loop matches the original instructions over
1336 cases: every one of 164 positions, three missing queries per style, mixed
case, empty names, duplicate precedence and varying string lengths. Guard pages
surround the table and each string/query; table, metadata and string contents
remain unchanged. Comparison argument pointers, order and counts match both
the native trace and an independently specified expected prefix. Candidate
virtual stack and nonvolatile GPR preservation are checked. The broken candidate
fails on the first lookup. Evidence: `local/automatic/lookup-results.json`.

Challenge: the engine calls an indirect CRT import rather than an internal
helper. Solution: verify its PE import identity, resolve the actual host API-set
`_stricmp`, and use a typed traced bridge for native and generated calls. A
success stub or replacement comparison would hide this boundary. This proves
ASCII case-insensitive fixtures in the host's initial C locale, not all locale
or arbitrary high-byte string behavior.

## CRC/hash module — verified

Four automatically lifted routines pass: seeded buffer CRC (RVA A9AE90),
zero-seed buffer CRC (CE2510), 32-bit value CRC (CE2570), and 64-bit value CRC
(CE25F0). They use two original read-only tables; there are no external calls.
20340 buffer cases cover 0..65536-byte lengths, six seeds, 16 alignment offsets,
both guarded placements and five patterns. 1564 integer cases cover individual
bits, complements, extremes and deterministic random values. Exact outputs,
output-page contents, input preservation and virtual nonvolatile/stack state
pass; integer results also match native byte-buffer hashing of the same value.
The corrupted-result control fails. Evidence: `local/automatic/crc-results.json`.

Challenge: community CRC labels belong to a different build. Solution: locate
actual polynomial-table bytes in the pinned input and trace their references;
verify calling conventions and full control flow from current instructions.
Challenge: shrink-wrapped functions have several chained .pdata fragments.
Solution: lift complete contiguous control-flow bodies, retain every original
runtime-function fragment and chained parent in the native oracle pack, and
reject missing parents. A single .pdata entry is not a whole-function boundary.

No seed is invented for the zero-seed helpers. Seed variation applies to the
routine that actually accepts a seed. These are bounded hashing proofs, not
evidence that engine containers, allocation or save serialization work.
