	# 📓 Lessons from v1 (reference prototype)

The mined wisdom of the current codebase — what to **port**, **redesign**, or
**drop** in v2. See [[ADR-004 — Fresh start (v2) with v1 as reference]].

> ✅ **Verdicts filled from the deep audit (2026-07-19).** Honest calls — "it
> worked, keep it" is a valid answer; v2 is not an excuse to redesign what was fine.
> Cross-cutting direction lives in [[v1 Code Audit]] §6 (not duplicated here).

## Salvage table

| Subsystem (v1) | What worked | What hurt | v2 verdict | Ref |
|----------------|-------------|-----------|-----------|-----|
| Render graph (`graph/`, `passes/`) | explicit, greppable passes; clean minimal graph | one shared `RenderResources` blackboard; no declared resource deps | **redesign** — keep the pass idea, add declared read/write deps | F22 |
| Passes (GBuffer, shadows, fog, godray, bloom, AO, exposure, post) | working feature set + shaders/math | structure fused to `RenderResources` | **port selectively** — shaders/math > structure | F22 |
| ECS (`ArchetypesManager`, `Archetype`) | archetype model + transition edges are sound | IDs from a reset-able static counter (**bug**); `parallelEach` leaks a mutex | **keep the model, redesign identity** (+ real job-system iteration) | F1, F15 |
| Resources (`ResourceSystem` + caches/loaders) | UUID model; consistent post-refactor | 650-LOC god-class; per-type copy-paste; `shared_ptr` everywhere | **redesign API (values/handles), port the UUID+cache model** | F7, F13 |
| Scene / CameraSystem | works | not deeply audited yet | **port, then review** in B1 | — |
| Events (`EventManager`, `type_index`) | type-keyed pub/sub is a good shape | unsubscribe unsound (compares `std::function`); per-event heap alloc; copyable | **redesign** — keep type-keying, fix subscription identity + allocation | F28 |
| Serialization | works | rides on component IDs (F1) | **port after fixing IDs** | F1 |
| Physics (Jolt) | Jolt integration works | keyed by string `Tag`; Euler round-trip drift; `cMaxBodies=1024` | **port, key off `Entity`, fix the euler round-trip** | F17 |
| Audio (miniaudio) | miniaudio integrates | plays from disk each call; no `AudioClip` resource; fire-and-forget | **redesign** as resource-backed with playback handles | F18 |
| Script (native C++ → DLL via cmake) | native speed; hot-reload concept | SDK exposes private `src/` types (**broken**); synchronous shell build freezes editor | **keep native decision, redesign delivery** — real SDK boundary + async build + own ADR | F9, F11 |
| Editor / runtimes | functional tooling | in the game/render loop; embeds Client+Server; god-panels | **rebuild** — host/observer out of the frame loop; decouple | F2, F14 |
| Server | — | pure stub, never ticked, yet coupled + SDK-shipped everywhere | **drop for now** — reintroduce as an optional module only when multiplayer is real | F2, F34 |
| Build / module layout | modular targets | globs, no presets/tests/test-bed, no API boundary, Core-in-2-DLLs, multi-config-fragile | **redesign** — presets, tests, test-bed, real boundaries, one linkage | F3, F4, F6, F8, F26, F27 |
| Threading | — | none by design; 3 ad-hoc models where it exists | **introduce** — one job system + task graph as a foundation | F15 |
| Logger / Profiler / Timer / FileSystem | usable | modeled as Systems; no source-loc; profiler bolted on | **redesign as helpers** (not Systems) + source-loc logging + assert strategy | F16, F20 |

## Porting notes

Reference code is reachable at the `v1-reference` tag:
`git show v1-reference:engine/client/src/renderer/graph/RenderGraph.hpp`
