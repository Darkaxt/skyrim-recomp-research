# Engine coverage catalogue

Generated from `engine-modules.json`. These are coverage domains, not implementation stages. Dependency arrows are planning contracts, not a proven acyclic execution schedule.

Each entry remains a whole-subsystem proof gap unless explicitly stated otherwise. Passing small slices do not mark a domain complete.

## F01: Instruction semantics and calling conventions

Group: Foundations. Dependencies: none declared.

**Treatment:** Translate engine instructions; implement explicit ABI and memory model.

**Current evidence:** Automatic projection/callback/frustum/lookup/CRC slices; scalar-SSE repairs.

**Next QA gate:** Opcode census and conformance fixtures for remaining SIMD, atomics, flags, stack layouts and indirect transfers.

**State/lifetime audit:** Register/FP state, stack, pointer provenance, native/generated transitions.

**Parallelism prerequisite:** Thread-local execution state and FP environment; no shared simulated CPU.

## F02: Startup, initialization and shutdown

Group: Foundations. Dependencies: F01, F03, F04, X01.

**Treatment:** Recover engine lifecycle; provide a deliberate launcher/platform entry boundary.

**Current evidence:** Entry point lies in .bind; static .text analysis does not prove startup.

**Next QA gate:** Inventory ordered initializers, config, singleton creation, dependency failure and teardown; boot an owned minimal lifecycle.

**State/lifetime audit:** Process globals, initialization order, shutdown dependencies and callbacks.

**Parallelism prerequisite:** Keep ordered bootstrap/teardown until independent tasks and completion dependencies are proven.

## F03: CRT, exceptions, RTTI and TLS

Group: Foundations. Dependencies: F01.

**Treatment:** Translate embedded runtime behavior or use declared matching native runtime bridges.

**Current evidence:** One cleanup-only MSVC scope and real loader TLS/CRT synchronization pass.

**Next QA gate:** Nested destruction, catches/rethrows, stack guards, alternate funclets, thread creation/destruction and runtime ABI.

**State/lifetime audit:** Per-thread epochs and runtime state; cleanup of partially built objects.

**Parallelism prerequisite:** Each worker needs valid TLS, FP/runtime state and exception propagation/cleanup policy.

## F04: Allocation, pools and object lifetime

Group: Foundations. Dependencies: F01, F03.

**Treatment:** Translate allocators and lifetime rules before redesign.

**Current evidence:** MemoryManager labels exist; no allocator or lifetime workflow validated.

**Next QA gate:** Allocate/construct/use/destroy/free; aligned allocations; reuse; allocation-failure cleanup and cross-thread release.

**State/lifetime audit:** Pool ownership, free lists, arena epochs, reference counts and destruction affinity.

**Parallelism prerequisite:** Document which allocator/object can move between workers and when reclamation is safe.

## F05: Containers, strings, hashing and math

Group: Foundations. Dependencies: F01, F04.

**Treatment:** Translate helpers; retain or replace CRT imports with explicit semantics.

**Current evidence:** CRC helpers, string-lookup bridge and selected camera/actor-value math pass.

**Next QA gate:** Container growth/erase/iteration, interning, locale/Unicode, sorting and numerical edge cases.

**State/lifetime audit:** Backing storage, intern tables, iterator validity and cached values.

**Parallelism prerequisite:** Read-only snapshots or explicit exclusive ownership for mutable collections.

## F06: Jobs, synchronization, time and random state

Group: Foundations. Dependencies: F03, F04, F07.

**Treatment:** Translate existing scheduling behavior; redesign only behind separately verified contracts.

**Current evidence:** One six-thread/five-waiter static initialization protocol passes; no general scheduler proof.

**Next QA gate:** Identify existing workers, queues, locks, completion events, frame clocks, random streams, cancellation and shutdown.

**State/lifetime audit:** Queue ownership, lock order, event delivery order, frame epochs and random-state mutations.

**Parallelism prerequisite:** Explicit dependencies and deterministic publication; measure contention rather than assuming the engine is wholly serial.

## F07: Object model, handles, factories and virtual dispatch

Group: Foundations. Dependencies: F01, F03, F04.

**Treatment:** Translate layouts and dispatch contracts; retain compatible native boundaries where required.

**Current evidence:** One virtual callback/receiver boundary passes; RTTI byte matches provide class hypotheses.

**Next QA gate:** One real object graph with factories, handles, refcounts, vtables, callbacks and destruction.

**State/lifetime audit:** Stable identity, handle generation, graph ownership and callback reentrancy.

**Parallelism prerequisite:** Resolve raw-pointer escape and shared mutable object graphs before moving work.

## C01: Files, archives and resource streams

Group: Content. Dependencies: F04, F05, F06, X01.

**Treatment:** Translate resource/VFS behavior; bridge OS I/O; preserve existing archive content.

**Current evidence:** BSResource SDK surface and installed BSA packages observed; no loader proof.

**Next QA gate:** Loose/archive override resolution, stream seek/read, decompression, malformed input, async completion and cancellation.

**State/lifetime audit:** File handles, stream position, resource cache entries and request lifetime.

**Parallelism prerequisite:** Read/decompress independent resources in workers; publish only fully constructed resources.

## C02: Plugin records, FormIDs and load order

Group: Content. Dependencies: C01, F05, F07.

**Treatment:** Translate record loaders, link/fixup and override semantics; preserve ESM/ESP/ESL data.

**Current evidence:** Installed master/light-plugin files and TESDataHandler SDK surface; not validated.

**Next QA gate:** Master dependencies, light IDs, overrides, localized strings, references, malformed records and duplicate definitions.

**State/lifetime audit:** Form registry, ID allocation, fixup tables and final override selection.

**Parallelism prerequisite:** Parse independent records separately; deterministic linking and publication must preserve load-order semantics.

## C03: Asset decoding and caches

Group: Content. Dependencies: C01, F04, F07.

**Treatment:** Translate readers/decoders and object construction; preserve or deliberately convert content formats.

**Current evidence:** SDK asset/resource classes; format inventory is incomplete.

**Next QA gate:** Inventory NIF/mesh/texture/animation/material/font/audio/UI/shader inputs; decode one asset family through construction and release.

**State/lifetime audit:** Decoded buffers, shared asset caches, deduplication and eviction.

**Parallelism prerequisite:** Independent decode is a candidate; cache registration and GPU/scene attachment require owned handoff.

## W01: Cells, references, streaming and LOD

Group: World. Dependencies: C02, C03, F06, F07, P01.

**Treatment:** Translate world lifecycle and streaming state machines.

**Current evidence:** TES/cell/streaming SDK names provide hypotheses; no real world workflow.

**Next QA gate:** Load/link/attach a cell; stream neighboring cells; unload while requests/callbacks are outstanding.

**State/lifetime audit:** Live cell/reference graphs, attachment state, streaming request lifetimes and LOD links.

**Parallelism prerequisite:** Prepare detached cell content off-thread; attach/detach through explicit scene/world ownership barriers.

## W02: Terrain, vegetation, water and world geometry

Group: World. Dependencies: W01, C03, P02, S07.

**Treatment:** Translate generation/loading/query behavior and renderer/physics integration.

**Current evidence:** Terrain/grass/water/collision SDK families; unvalidated.

**Next QA gate:** One terrain tile including collision, vegetation/LOD and water transitions with resource lifetime checks.

**State/lifetime audit:** Terrain meshes, grass instances, water state, collision attachment and cache invalidation.

**Parallelism prerequisite:** Tile preparation is a candidate after immutable inputs and deterministic attachment are defined.

## W03: Weather, climate, lighting and world time

Group: World. Dependencies: W01, F06, P02, P05.

**Treatment:** Translate update rules and their simulation/presentation dependencies.

**Current evidence:** Weather/climate SDK families; unvalidated.

**Next QA gate:** Advance controlled world time through weather transitions and compare dependent light/audio/event state.

**State/lifetime audit:** Shared world clock, climate state, lighting parameters and event order.

**Parallelism prerequisite:** Calculate from a stable frame snapshot; commit a consistent environment state.

## S01: Actors, values, movement and process state

Group: Simulation. Dependencies: F07, W01, S06, S07.

**Treatment:** Translate actor state and update behavior.

**Current evidence:** Actor-value lookup/clamp slices only; no integrated actor simulation.

**Next QA gate:** One actor creation/update/movement/damage/death/destruction path with callbacks and stable identity.

**State/lifetime audit:** Actor process state, movement, values, active effects and cross-actor references.

**Parallelism prerequisite:** Partition only after cross-actor effects and movement/physics publication are explicit.

## S02: AI, perception, packages and navigation

Group: Simulation. Dependencies: S01, W01, S07, F06.

**Treatment:** Translate decision-making and navigation behaviors.

**Current evidence:** AI/package/navmesh SDK surfaces; unvalidated.

**Next QA gate:** One deterministic perception/path/decision/action sequence with changing obstacles and actor interactions.

**State/lifetime audit:** Perception caches, actor package stacks, navmesh queries and action side effects.

**Parallelism prerequisite:** Read-only query/decision work may use snapshots; ordered world mutations need explicit commit semantics.

## S03: Combat, magic, perks, effects and projectiles

Group: Simulation. Dependencies: S01, S02, S07, S08, S09.

**Treatment:** Translate gameplay rules and observable ordering.

**Current evidence:** Effect/casting/gameplay SDK labels; unvalidated.

**Next QA gate:** One attack/spell through hit detection, modifiers, effects, events and cleanup; compare event order and RNG use.

**State/lifetime audit:** Targets, health/effects, projectile ownership, event queues and random streams.

**Parallelism prerequisite:** Parallel queries/calculation require stable snapshots; damage/effect commits must preserve defined ordering.

## S04: Inventory, equipment, crafting, economy and progression

Group: Simulation. Dependencies: S01, C02, S08, S09, P04.

**Treatment:** Translate state changes and presentation/script/save connections.

**Current evidence:** Inventory/equipment/crafting SDK surface; unvalidated.

**Next QA gate:** Acquire/equip/use/remove an item with effects, UI updates, script events and persistence.

**State/lifetime audit:** Container entries, stacks, equipment graphs, currency, progression and notifications.

**Parallelism prerequisite:** Keep multi-object transactions coherent; do not split ownership-dependent mutations arbitrarily.

## S05: Quests, dialogue, scenes, factions and crime

Group: Simulation. Dependencies: S01, S08, S09, C02, P04, P05.

**Treatment:** Translate state machines and condition/event evaluation.

**Current evidence:** Quest/dialogue/faction SDK families; unvalidated.

**Next QA gate:** One quest/dialogue transition with conditions, aliases, scene actions, faction/crime effects and reload.

**State/lifetime audit:** Quest stages, alias bindings, dialogue state and ordered cross-system effects.

**Parallelism prerequisite:** Evaluate pure conditions on snapshots; transitions and their side effects require ordered publication.

## S06: Animation, behavior graphs, skeletons and skinning

Group: Simulation. Dependencies: C03, P01, F06, F07.

**Treatment:** Translate embedded behavior/runtime and loading; preserve existing animation content.

**Current evidence:** Animation/behavior/Havok SDK types; unvalidated.

**Next QA gate:** Load one skeleton/graph, advance animation, emit events, apply root motion and release resources.

**State/lifetime audit:** Graph instances, pose buffers, root motion, event ordering and actor/physics coupling.

**Parallelism prerequisite:** Per-instance pose evaluation is a candidate with immutable inputs and separate output buffers.

## S07: Physics, collision, ragdolls and character controllers

Group: Simulation. Dependencies: F04, F06, F07, C03.

**Treatment:** Translate embedded physics code or explicitly replace its behavior boundary.

**Current evidence:** Havok RTTI/type surfaces observed; no physics equivalence.

**Next QA gate:** Construct/step/query/destroy a small world; contacts, raycasts, activation, characters and animation coupling.

**State/lifetime audit:** Physics world locks, bodies, islands, contact callbacks and deferred mutations.

**Parallelism prerequisite:** Recover existing engine/middleware threading rules first; behavioral replacement has its own equivalence burden.

## S08: Papyrus VM, bytecode and native bindings

Group: Simulation. Dependencies: F03, F04, F06, F07, C02, S09.

**Treatment:** Translate VM/runtime and native functions; execute existing PEX content.

**Current evidence:** BSScript/SkyrimVM SDK families; no VM execution proof.

**Next QA gate:** Execute a script with native calls, latent suspension/resume, events, handles, collection and save/load.

**State/lifetime audit:** VM stacks, handles, object bindings, scheduler queues and native call thread affinity.

**Parallelism prerequisite:** VM internals may be concurrent already; native game mutations need explicit affinity and ordering contracts.

## S09: Events, callbacks, conditions and console commands

Group: Simulation. Dependencies: F06, F07.

**Treatment:** Translate dispatch/registration and observable ordering.

**Current evidence:** One callback bridge and ordered lookup trace; no general event system proof.

**Next QA gate:** Register/dispatch/unregister with reentrancy, receiver destruction, deferred work and shutdown.

**State/lifetime audit:** Subscriber lists, callback captures, deferred queues and reentrant mutation.

**Parallelism prerequisite:** Use ordered messages where justified; never move callbacks without auditing their state and thread assumptions.

## P01: Scene graph, transforms, cameras and visibility bounds

Group: Presentation. Dependencies: F05, F07, C03.

**Treatment:** Translate graph operations and camera math.

**Current evidence:** Projection and frustum/cache slices pass; scene graph/lifetimes remain untested.

**Next QA gate:** Construct/update/clone/cull/detach/destroy one real scene graph and observe dependent caches.

**State/lifetime audit:** Parent-child links, transforms, dirty flags, camera/frame cache and node lifetime.

**Parallelism prerequisite:** Stable graph snapshots or ownership partitions before parallel transform/culling work.

## P02: Renderer preparation, materials and draw ordering

Group: Presentation. Dependencies: P01, C03, W03, F06, P03.

**Treatment:** Translate renderer CPU behavior; change scheduling only after visual equivalence.

**Current evidence:** D3D11/DXGI imports and renderer SDK families; no frame rendered.

**Next QA gate:** One repeatable rendered frame with materials, shadows, visibility, transparency and resource updates.

**State/lifetime audit:** Material caches, render lists, scene snapshots, pass dependencies and GPU resource lifetime.

**Parallelism prerequisite:** Partition independent preparation; preserve transparent ordering, pass dependencies and publication boundaries.

## P03: GPU resources, shaders, command submission and presentation

Group: Presentation. Dependencies: X01, C03, F04.

**Treatment:** Bridge D3D11/DXGI initially; preserve shader behavior or explicitly port/rewrite it.

**Current evidence:** D3D11/DXGI/D3DX imports; shader-cache directory; no GPU validation.

**Next QA gate:** Inventory shader variants and resource formats; create/upload/draw/present/resize/device-recover with visual comparisons.

**State/lifetime audit:** Device contexts, command lists, textures/buffers, fences and swap-chain ownership.

**Parallelism prerequisite:** One context at a time per D3D11 context; independently owned deferred contexts are a design candidate, not a proven speedup.

## P04: UI, Scaleform/ActionScript, menus, text and localization

Group: Presentation. Dependencies: C03, S09, P03, P06.

**Treatment:** Translate embedded UI integration/runtime or provide an explicit compatible replacement; preserve UI assets.

**Current evidence:** GFx/Scaleform SDK families; unvalidated.

**Next QA gate:** Load one menu/movie, display localized text, deliver input/native callbacks, close and recreate it.

**State/lifetime audit:** Movie/menu state, font caches, UI callbacks and game-state bindings.

**Parallelism prerequisite:** Respect UI ownership; off-thread preparation must not mutate live movie/game state unexpectedly.

## P05: Audio, music, dialogue, spatial sound and lip sync

Group: Presentation. Dependencies: C03, S09, W03, X01.

**Treatment:** Translate audio scheduling/integration; bridge native audio backend or explicitly replace it.

**Current evidence:** X3DAudio imports and audio SDK families; backend dynamic/COM paths not fully resolved.

**Next QA gate:** Decode/play/position/stop one sound; dialogue/music transitions, callbacks, device loss and resource release.

**State/lifetime audit:** Audio voices, mix/control queues, callback affinity and shared timing.

**Parallelism prerequisite:** Owned audio-thread command handoff; independent decode is a candidate after lifetime/cancellation proof.

## P06: Input, window events, controls and camera interaction

Group: Presentation. Dependencies: X01, F06, S09.

**Treatment:** Translate mapping/game control behavior; bridge Windows/HID/controller APIs.

**Current evidence:** USER32/DINPUT/XINPUT/HID imports; no input workflow.

**Next QA gate:** Capture/replay ordered input through focus, menus, rebinding, game controls and disconnect/reconnect.

**State/lifetime audit:** Window/input thread, ordered device events, mappings and frame sampling.

**Parallelism prerequisite:** Capture input centrally and publish immutable input frames; preserve observable event order.

## P07: Video playback and movie integration

Group: Presentation. Dependencies: C01, P03, P05, P06.

**Treatment:** Translate engine integration; retain supplied Bink backend or explicitly replace playback behavior.

**Current evidence:** Nine Bink imports and installed Bink DLL; unvalidated.

**Next QA gate:** Open/play/skip/close a movie with synchronized audio, input and teardown.

**State/lifetime audit:** Decoder buffers, audio/video clocks and presentation ownership.

**Parallelism prerequisite:** Backend-supported decode queues only; synchronization/lifetime must remain explicit.

## D01: Save/load, serialization and versioned state

Group: Persistence. Dependencies: F07, C02, W01, S01, S08, S09.

**Treatment:** Translate serialization and restore semantics; preserve existing save contracts deliberately.

**Current evidence:** Save/load SDK wrappers call original engine functions; no save proof.

**Next QA gate:** Round-trip a minimal state including handles/scripts/references; load original saves; interrupted writes and missing/modded content.

**State/lifetime audit:** Consistent world/VM snapshot, reference fixups, background writes and load barriers.

**Parallelism prerequisite:** Capture a coherent owned snapshot before background serialization; define quiescence and resume explicitly.

## X01: OS APIs, runtime imports and dynamic libraries

Group: Platform. Dependencies: F01, F03.

**Treatment:** Explicit native bridges or replacements; do not translate the OS/drivers themselves.

**Current evidence:** Complete pinned static import table; one actual CRT bridge and selected OS synchronization calls exercised.

**Next QA gate:** Disposition every import, dynamic LoadLibrary/GetProcAddress target, COM interface, callback ABI and failure behavior.

**State/lifetime audit:** OS handles, callbacks, COM apartment/thread affinity and native resource teardown.

**Parallelism prerequisite:** Respect each API's threading contract; bridge boundaries do not automatically make calls thread-safe.

## X02: Steam, achievements, Creations and online services

Group: Platform. Dependencies: X01, C01, P04, S09.

**Treatment:** Translate engine integration; retain explicit platform interfaces or document feature replacements.

**Current evidence:** Steam/WinHTTP/WS2/crypto imports; service paths not exercised.

**Next QA gate:** Map startup/offline/failure/callback behavior and feature-specific state; no silent successful stubs.

**State/lifetime audit:** Platform callbacks, request state, service caches and shutdown order.

**Parallelism prerequisite:** Async service requests with explicit completion/callback ownership; offline policy must be a deliberate product decision.

## M01: Data, script, asset and UI mods

Group: Compatibility. Dependencies: C01, C02, C03, S08, P04, D01.

**Treatment:** Preserve loader/format/native-script contracts; test existing mod content explicitly.

**Current evidence:** Two historical MO2 instance inventories; no mod compatibility execution.

**Next QA gate:** Curated data/script/asset/UI cases with override priority, native-function bindings and save round trips.

**State/lifetime audit:** Load-order state, VM/UI extension hooks and content cache invalidation.

**Parallelism prerequisite:** Changes must preserve callback/event ordering relied on by content and scripts.

## M02: SKSE, native DLL plugins, hooks and binary patches

Group: Compatibility. Dependencies: F01, F03, F07, S08, S09, D01.

**Treatment:** Translate/port selected extension behavior or preserve a deliberately supported ABI/hook boundary.

**Current evidence:** Native-plugin sample inventories; no plugin DLL loaded; older/current runtime versions differ.

**Next QA gate:** Per-plugin inventory of patches, overwritten behavior, addresses/layouts, trampoline calls, APIs, thread assumptions and co-saves.

**State/lifetime audit:** Untracked raw pointers, direct memory writes, arbitrary detours and callbacks into engine state.

**Parallelism prerequisite:** Arbitrary old binary patch compatibility may constrain layout/scheduling changes; source-port or narrowed stable APIs need explicit support policy.

## M03: Renderer injectors, shader extensions and engine-fix behavior

Group: Compatibility. Dependencies: M02, P02, P03, X01.

**Treatment:** Catalogue each extension/fix; retain compatible hooks or implement its intended behavior explicitly.

**Current evidence:** EngineFixes/DisplayTweaks/CommunityShaders and other sampled plugin metadata; no behavior absorbed.

**Next QA gate:** Separate bug fix from workaround; map intercepted renderer/runtime paths and validate visual/behavior/performance regressions.

**State/lifetime audit:** Renderer hooks, resource interception, timing changes and extension-owned state.

**Parallelism prerequisite:** Audit hook affinity and reentrancy before concurrent rendering/resource work; API interception can impose serialization.

## T01: Creation Kit, launchers, compilers and developer tools

Group: Tooling. Dependencies: C02, C03, S08, D01.

**Treatment:** Keep external tools initially; map their output/runtime contracts rather than blindly translating every installed executable.

**Current evidence:** Creation Kit/launcher/Papyrus compiler installed; not surveyed as engine runtime dependencies.

**Next QA gate:** Confirm required generated content and workflow compatibility; classify any newly discovered runtime-linked dependency.

**State/lifetime audit:** Tool outputs, versioned formats and build provenance.

**Parallelism prerequisite:** External authoring/build parallelism is separate from live-engine scheduling.

## T02: QA, diagnostics, builds and reproducibility

Group: Tooling. Dependencies: F01.

**Treatment:** Authored verification tooling; no game behavior translation.

**Current evidence:** Native differential gates, negative controls, source/input/artifact hashes and clean public reproduction pass.

**Next QA gate:** Expand to connected subsystem traces, long-run lifetime checks, save/visual/audio compatibility and performance profiles.

**State/lifetime audit:** Repeatable fixtures, oracle separation, test-state isolation and evidence provenance.

**Parallelism prerequisite:** Race-sensitive tests need controlled schedules plus adversarial stress; exact and modified-behavior oracles remain separate.

## U00: Unassigned code/data and dynamically discovered behavior

Group: Unknown. Dependencies: none declared.

**Treatment:** Explicit investigation queue; never treat unknown or unexecuted behavior as unnecessary.

**Current evidence:** All discovered functions/ranges are indexed; most ownership/signatures and indirect transfers remain unverified.

**Next QA gate:** Resolve function owners, signatures, all executable gaps, virtual/callback targets, dynamic loading, data references and trigger coverage.

**State/lifetime audit:** Unknown global reads/writes, pointer escapes, ordering and callbacks.

**Parallelism prerequisite:** No concurrency admission until transitive effects and ownership are established.
