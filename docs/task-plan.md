# Connected workflow task plan

Authority: [file-lifetime specification](file-lifetime-spec.md). The
[engine map](engine-coverage-map.md) and [effort assessment](engine-effort.md)
provide planning context.

Each row is a bounded implementation stage. Domain prefixes indicate ownership;
an ID does not represent completion of its whole engine domain. There are
currently **zero ACTIVE stages**. F04-002 completed on 2026-10-09;
the next scheduled run should select C01-001.

| Task ID / stage | Status | Depends on | Outcome |
|---|---|---|---|
| F04-001: pin one resource/object path | COMPLETE | none | Verified selection of the 11-entry native file-object workflow; see file-lifetime-selection.md |
| F04-002: establish the original oracle | COMPLETE | F04-001 | Verified native file workflow, real failure/ownership trace, mixed-page fix and applicable regressions; see file-lifetime-native.md |
| C01-001: translate and integrate the normal path | NOT STARTED | F04-002 | Generated path matches the same original successful workflow, including teardown |
| C01-002: prove failure and partial cleanup | NOT STARTED | C01-001 | The integrated path handles specified failures, partial construction and subsequent reuse correctly |
| T02-001: reconcile and calibrate the slice | NOT STARTED | C01-002 | Close the connected workflow with applicable regression evidence and measured effort/remaining limits |

## F04-001 acceptance criteria

1. Select a real pinned-build engine path involving resource access and object
   lifetime. Verify candidate identities and actual instruction/control-flow
   evidence; foreign labels and Function ID names remain hypotheses.
2. Record the specific entry points, body/dependency closure, data/global/TLS
   requirements, allocation/destruction and native API/CRT callbacks in ignored
   local evidence. Distinguish verified dependencies from unresolved targets.
3. Write the selected workflow's authoritative specification: input/observable
   output, valid initial state, normal lifecycle/teardown, failure cases, oracle,
   negative control and exact acceptance criteria for F04-002 through T02-001.
   Resolve ordinary details without approval; preserve the boundaries in D3/D4.
4. Establish from evidence how the native fixture can run without game startup,
   arbitrary engine-memory fabrication, successful stubs or protection unpacking.
   Do not mark this ID COMPLETE merely for listing candidate names. If a selected
   path is unsuitable, investigate a narrower real path within this ID.

## F04-002 acceptance criteria

The selected authority is [file-lifetime-spec.md](file-lifetime-spec.md). Its
mixed code/table page-permission fix, native API tracing and oracle corpus are
required implementation/verification work for this stage.

Execute the specified original instructions with valid declared dependencies;
verify input identity, resource result, allocation/ownership trace, permitted
memory changes, ABI/runtime state and normal teardown. Establish repeatable
fixtures and an effective oracle/control check before comparing generated code.
Retain commands and evidence. Passing this stage proves the native fixture only.

## C01-001 acceptance criteria

Use the selected closure and exact defined observations in
[file-lifetime-spec.md](file-lifetime-spec.md).

Automatically translate the selected closure and integrate actual native bridges
where the workflow specification permits them. Match the original normal-path
results, lifecycle/resource trace, permitted memory writes and ABI/runtime state
over the specified fixture corpus, including repeated use and normal teardown.
A deliberately incorrect candidate must be rejected. Document unsupported failure
behavior explicitly; C01-002 owns it, and the overall slice remains incomplete.

## C01-002 acceptance criteria

Apply the same integrated original/generated workflow to the specified allocation,
read/parse and partial-construction failure cases that are real for the selected
path. Verify cleanup/exception propagation, no leaked or prematurely freed owned
resources, and correct retry/reuse where the original contract permits it. Reject
a meaningful disabled/broken-cleanup control. Never invent an error interface
absent from the real path or weaken exact comparisons to obtain a pass.

## T02-001 acceptance criteria

Reconcile every criterion of the selected workflow specification and all required
blockers/deferrals. Verify relevant existing translation/runtime regressions when
shared support changed; otherwise reuse applicable current results. Recheck input
identity and source/artifact evidence; retain reproducible commands, boundaries,
challenges/solutions and effort by discovery/new contracts/integration/QA. Update
the measured effort assessment without projecting a completion percentage from
this one slice. Commit the verified authored scope locally. No public push is
part of this task ID.

## Tracking fields for each run

Update the row status and record acceptance criteria satisfied/remaining,
blockers with resolution conditions, tracked deferrals with named later owner
and required verification, evidence paths, relevant commit and next action in
[current progress](progress.md). Initially no blocker or tracked deferral
has been identified; these tasks are ordinary work not yet started.
