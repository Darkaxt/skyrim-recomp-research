# Next slices and public repository specification

Authority: the user's request to write a short testing plan, make exception
cleanup and additional modules work, then create a public GitHub repository to
record project evolution if the gates pass. Implementation, verification, local
commits and conditional public publication are already authorized.

## Constraints

All project/dependency/temp/cache output stays on E:. Read existing compilers on
C:. Original game/mod files are read-only. No gameplay, game startup, mod-DLL
execution, profile/save changes or protection unpacking. Preserve stock and
previous repaired failures as historical evidence. New work must not redefine
those historical runs as passes. No epsilon comparisons or successful stubs.

## Required gates

- **N1 / AC1:** Implement a metadata-driven repair for the observed cleanup-only
  MSVC C++ exception boundary. Discover its IP states and unwind actions from
  pinned PE metadata; lift cleanup funclets automatically. Do not special-case
  the root algorithm with a hand-written guard reset or replace the original
  cleanup action with a no-op. The existing 57344 serial comparisons, real TLS,
  six-thread/five-waiter initialization, throws at all six constructor positions
  and successful retries must pass. Preserve native CRT dependency declarations
  and demonstrate disabled-cleanup rejection. Clearly bound support to tested
  cleanup-only scopes; catch matching and arbitrary exception systems are not
  implied. Add a focused authored nested-cleanup fixture if needed to validate
  state/action ordering beyond the one real engine scope.
- **N2 / AC2:** Pin and automatically translate the real actor-value name lookup
  function. Enumerate table/name/import memory dependencies; use actual host CRT
  comparison with a typed bridge. Compare exact result and memory, and trace
  comparison arguments/order/count for first/middle/last/missing, mixed case and
  duplicate-name cases under guarded memory. A broken candidate must fail.
- **N3 / AC3:** Pin and automatically translate engine CRC/hash helpers after
  locating their complete boundaries and constants. Use the original instructions
  as the oracle over meaningful lengths/seeds/alignment/bit patterns, guard
  inputs and compare output/memory exactly. Reject an incorrect candidate. Record
  precisely which hash routines are proven; this gate does not include a general
  allocator, save format or engine container implementation.
- **N4 / AC4:** Reconcile and verify all functional gates and original input
  identities, document reusable challenges/solutions and unresolved broader
  capabilities, clean expendable intermediates and commit locally. Only after
  N1-N3 pass, create a public repository on the authenticated user's GitHub
  account with a reasonable available project name. Publish a reviewed explicit
  allowlist of authored tooling, harnesses, documentation and sanitized measured
  results. Exclude original/lifted/decompiled game code/data and personal/private
  environment details from public history. Include license/provenance, clear
  local-input requirements and a supported public reproduction command. Verify
  the remote branch/content/visibility against the prepared local commit.

## Completion and limits

The short plan is the stage ledger. A functional failure blocks its stage; do
not publish under the conditional authorization while a required gate fails.
Do not broaden into whole-engine boot, graphics/audio, allocation, saves, mods,
arbitrary concurrency or other architectures. The deliverable demonstrates
these gates and a public research project, not a playable engine replacement.
