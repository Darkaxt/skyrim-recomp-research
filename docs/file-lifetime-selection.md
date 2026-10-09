# F04-001 selection result

Historical milestone, 2026-10-08; see [current progress](progress.md).

Status at selection closure: **COMPLETE**, selection and specification only. Native execution,
automatic translation and integrated file behavior are **NOT STARTED**.

Selected target: the pinned engine's 16-byte file-handle wrapper, covering real
initialization, read-only open, size/read/seek, ownership transfer and close.
Its [authoritative workflow specification](file-lifetime-spec.md) defines the
following task IDs and their exact oracle, corpus and failure requirements.

## Acceptance reconciliation

| F04-001 criterion | Inspected evidence |
|---|---|
| 1: choose a real pinned path | Current executable SHA checked; native instructions independently decoded and compared with Ghidra metadata; semantic roles derive from instructions and imports |
| 2: pin dependencies and state | 11 complete bodies, 864 code bytes, seven ordinary unwind records, four stack-neutral leaf/tail routines, 208 table bytes, six real native imports; all selected branches/calls resolved |
| 3: specify the connected workflow | Valid initial state, observed fields, normal lifecycle, transfer/teardown, real failure cases, exact observations, opaque handle identity, negative controls and subsequent task criteria documented |
| 4: establish offline fixture feasibility | Original constructors initialize guarded raw storage; OS creates actual handles; no engine singleton/heap/game TLS/vtable dependency; explicit isolated mapping/unwind/bridge design, with the loader integration prerequisite assigned to F04-002 |

The static closure probe and independent consistency checks passed. They verify
the selected dependency envelope and test design; they do not demonstrate that
the native fixture already executes successfully. There is no unresolved
criterion or blocker for this selection stage. The next stage has ordinary
implementation work, including the loader fix below.

## Challenges and reusable solutions

**The obvious stream wrapper was too broad.** The real
BSResourceNiBinaryStream constructor/destructor leads into resource-manager
initialization, virtual stream dispatch, reference counts and heap teardown.
It cannot be made standalone by populating a few guessed fields or returning
success from omitted helpers. Select the lower-level real file-object workflow
first; this does not prove that high-level stream or heap behavior works.

**A matching layout is not a proven class/enum identity.** The observed field
offsets fit a community declaration, but imported class names and error enums
are not the oracle. Retain descriptive roles, verify actual numeric mapping and
record object state directly from the pinned instructions.

**The error mapper has code and data in one section/page.** Its six-target
switch needs both a target-RVA vector and a compact selector table. Index bounds,
target starts, hashes and the image-base anchor are verified; copying just its
function-body range would omit the tables.

**The shared native loader currently overwrites page permissions.** Its
`pages[page] = ...` assignment makes mixed code/read-only-data protection depend
on piece order. F04-002 must combine all required page permissions, retain RX for
code plus read-only tables, reject unsupported writable/executable combinations,
and verify reversed piece order and applicable regressions. Do not work around
the issue by arranging a favorable pack order.

**OS handle values cannot be compared as fixed numbers across two executions.**
Trace each real handle's identity and ownership transitions, compare that run's
object fields to its actual returned handle, and compare final invalid states
exactly. This is an explicit comparison contract, not a replacement resource or
a relaxed numerical result check. Trace bookkeeping must preserve last-error
state before the original mapper observes it.

## Evidence and reproduction

Selection used a private read-only Ghidra metadata export. The resulting
range/hash manifest is now tracked as `validation/file_lifetime_manifest.json`,
so the [native oracle](file-lifetime-native.md) reproduces without that export.

Retained local evidence:

- `local/analysis/lifetime/file-lifetime-closure.json`: source/code/table/unwind
  hashes, all selected control transfers and import/data references.
- `local/analysis/lifetime/F04-001-verification.json`: inspected independent
  identity, consistency, fixture-shape and shared-page checks.
- `local/analysis/lifetime/F04-001-timing.json`: monotonic session timing.
- `local/analysis/lifetime/candidates/`: private read-only Ghidra exports of the
  narrowed file path and rejected high-level constructor/destructor.
- `local/logs/lifetime-candidates*.log`: successful read-only/no-reanalysis export.

No game startup, original-input mutation, native game instruction execution,
mod execution, harness build or public push occurred in this selection ID.

## Timing

Measured autonomous session elapsed: **707.420 seconds (11 minutes 47 seconds)**,
from selection start through inspected verification. Automation setup and final
status/commit bookkeeping are excluded. This includes reasoning, source reads,
tool execution and specification work. It is not measured human engineering
effort and does not calibrate the complete workflow's earlier 100–300-hour
planning allowance. Runtime integration and translation remain unmeasured.
