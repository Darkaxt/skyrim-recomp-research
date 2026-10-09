# Full-engine translation mapping scope

Authority: the user's request to map everything requiring translation or another
explicit implementation decision, with broad coverage supporting fixes, QA and
eventual changes to engine scheduling/parallel pipelines.

This deliverable is investigation and planning, not authorization to implement
the mapped engine, change scheduling, launch gameplay or publish new material.
Formal implementation stages are unnecessary for this mapping exercise.

## Required mapping

- M1: reconcile the complete pinned PE envelope: executable code, read-only and
  initialized/zero-fill data, unwind/EH, TLS, relocations, imports, resources and
  entry/startup boundaries. Account for all discovered function records, actual
  body ranges and unresolved regions; retain unknowns rather than invent owners.
- M2: provide a subsystem catalogue covering runtime foundations, content and
  world systems, simulation, scripts, presentation, persistence, platform and
  mod compatibility. Each entry identifies implementation treatment, dependencies,
  present proof, next QA evidence, shared-state risks and parallelism prerequisites.
- M3: separate translated engine code, native bridges, behavioral reimplementation,
  existing content/bytecode/shaders and external tooling. No dependency may
  disappear by being labelled third-party, generated, unreachable or optional.
- M4: distinguish discovery, naming, lifting, native equivalence, subsystem
  integration, regression QA and modified/concurrent behavior. No byte/function
  count is a completion percentage. Current passing slices retain their tested
  limits. Existing FID/community labels are hypotheses with explicit provenance.
- M5: explain a dependency-respecting route to larger workflows and eventually
  parallel execution, including ownership, determinism, save barriers, callback
  affinity and native-mod hook conflicts. No speedup or thread safety claim from
  translation alone; no arbitrary per-module completion dates.
- M6: save reproducible authored mapping tools and readable catalogue/diagram;
  pin source/input hashes, verify count/range reconciliation, document omissions
  and commit the local deliverable. Original game/mod files remain read-only;
  all outputs stay on E:. Detailed game-derived indices stay ignored locally.
- M7 (user-requested sizing/effort extension): quantify the current total and
  partial group/domain attribution using function counts and actual body ranges;
  reconcile unassigned and overlapping bytes. Separate these measurements from
  full subsystem sizes, data/content/extension scope and engineering effort.
  Document planning assumptions, uncertainty and the evidence needed to replace
  provisional effort judgments with measured estimates. Do not invent complete
  per-group sizes or completion dates from sparse labels.

## Completion

Mapping completion means every current structural record has a ledger entry,
every identified implementation domain has a declared disposition and proof gap,
and unresolved discovery/ownership is explicitly represented. It does not mean
all executable behavior has been identified, translated, integrated or tested.
New runtime discoveries must extend the map rather than evade its coverage gate.
