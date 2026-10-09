# Expected effort and what can be estimated

The coverage groups define an inventory taxonomy. They are not yet sized work
packages. [Measured sizes](engine-sizes.md) quantify the current partial
attribution; they do not support a credible full-engine schedule or a numerical
effort split between groups. This is a limitation of the present feasibility
study, not evidence that the unattributed work is small.

## Current planning judgment

The passing slices justify continuing feasibility work. They do not justify
expecting the full engine, broad existing-mod compatibility and scheduling
redesign to be delivered in a few months of occasional attention. For that
combined target, use **a multi-year research horizon**, with a real possibility
that particular compatibility goals or runtime boundaries prove impractical.
This is a conservative planning judgment, not a measured duration, guaranteed
lower bound or commitment. Automation may improve throughput; its benefit on
subsystem integration has not been demonstrated here.

The next useful numerical allowance is for **one bounded offline resource and
object-lifetime workflow**, not a whole subsystem: construct owned state, read
one selected resource through a verified boundary, publish a result, inject a
failure and release everything correctly. It must include a native oracle and
effective negative controls. It excludes engine startup, a rendered world,
Papyrus, save compatibility and mod execution.

My provisional allowance is **100–300 focused engineering hours**, including
investigation, fixture/oracle construction, translation/bridge repairs,
integration, review and QA. Confidence is low: this is an engineering judgment
for planning the next experiment, not an extrapolation from recorded throughput.
It assumes Windows x64, the existing pinned build and tools, retained native
backends, a deliberately small dependency closure and no new startup/protection
obstacle. Discovering a new lifetime/EH/ABI dependency can put it outside the
range. Do not treat its upper end as a deadline or a guaranteed maximum.

| Sustained focused project time | Calendar equivalent of that allowance |
|---|---|
| 5 hours/week | 20–60 weeks |
| 10 hours/week | 10–30 weeks |
| 40 hours/week | 2.5–7.5 weeks |

These are arithmetic scenarios, not delivery dates. They exclude breaks and
unattended machine time; AI assistance is part of the engineering workflow,
without an assumed fixed speed multiplier. First choose an actual connected
workflow and measure it before using these figures for commitments.

## Why group effort cannot follow the current byte counts

The following risk judgments are qualitative hypotheses. They identify the
dominant work and evidence needed for a group estimate; they are not a ranking
of measured subsystem size. The groups overlap in integration and QA obligations,
so independent allowances could not simply be summed.

| Group | Main effort drivers | Evidence needed to estimate the full group |
|---|---|---|
| Foundations | Instruction semantics, ABI/EH/TLS, allocation, ownership, dispatch and synchronization; shared by nearly every workflow | Representative nested cleanup, object lifecycle, allocator and job/callback workflows; dependency reuse measured |
| Content | Archive/stream formats, plugin linking and override semantics, asset decoding, caches and failures | One archive-to-owned-resource workflow plus representative plugin and asset families |
| World | Streaming, reference identity, unload, LOD, terrain and changing scene state | Repeated cell load/update/unload with lifetime evidence and a classified streaming closure |
| Simulation | Large interacting rule sets, actors, AI, combat, animation, physics, VM and observable event ordering | Separate representative actor/physics/animation/script workflows followed by their real interactions |
| Presentation | Scene/render state, GPU resources, UI, input, audio/video, callbacks and teardown | One actual frame with retained backends, then UI/input/audio interactions and resource recreation |
| Persistence | Coherent world/VM capture, graph restoration, original save formats and extension state | Save/load of a connected workflow and a representative original-save compatibility corpus |
| Platform | Imports, dynamic interfaces, callbacks, handle ownership, initialization and services | Actual boundary traces including failure and shutdown; static import counts alone are insufficient |
| Compatibility | Plugin hooks, raw pointers/layouts, scripts/content assumptions, renderer injectors and co-saves | An explicitly supported plugin/content set with per-extension contracts and integrated tests |
| Tooling and QA | Build/reproduction workflows, diagnostics, regression corpus, long-run and visual/audio checks | Measured verification cost as each supported workflow is added; QA continues across groups |
| Unknown | Most function ownership, indirect targets, transitive effects, data ownership and missed code | Broader trustworthy attribution and representative dependency closures; resolve uncertainty rather than assume a uniform cost/function |

Full group person-hours are **not estimated**. Nor is there a defensible numerical
estimate yet for first playable boot, broad vanilla behavior, broad mod support
or safe pipeline redesign. Those outcomes require different acceptance criteria;
one must not be substituted for another to make an estimate appear achievable.

## How to turn this into a measured estimate

Expand ownership using build-matched RTTI/vtables, constructor/destructor
relationships, referenced registries/data and callers/callees. Keep supporting
evidence and confidence with each cluster, including shared runtime facilities.
Call-graph proximity alone is not proof of an owner, and broad Function ID names
must not be promoted into confident subsystem sizes.

For the next connected workflow, record focused engineering effort separately
for discovery, new semantic/runtime contracts, reuse of existing support,
integration and QA/debugging. Also retain dependency closure, original oracle,
negative controls and failure/teardown evidence. Generated function count or
unattended translation time is not a substitute for those measurements.

Repeat that measurement on contrasting boundaries: resource/object lifetime,
script/event state, and graphics/native callback integration. Use the completed
workflows to estimate analogous clusters, leaving unfamiliar clusters explicitly
unestimated. Track changes in the scope as more functions, data and dynamic
targets receive owners. Use actual reused versus new contracts to revise the
allowances; do not multiply one small CRC function's cost by 176001.

Changing scheduling needs an additional budget after recovering the relevant
state/lifetime contracts: intentional behavior specifications, deterministic
publication, race/lifetime QA, plugin compatibility and end-to-end performance
measurement. It is not a small fixed surcharge on translation. See
[parallel-pipeline constraints](parallel-pipelines.md).

All of this remains planning. No new game subsystem, gameplay run or mod
execution is authorized or performed by this sizing extension.
