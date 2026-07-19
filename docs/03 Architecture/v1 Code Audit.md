# 🔍 v1 Code Audit

Deep structural audit of the **v1 reference prototype** — how it's built + what
aged badly. Verdicts (port / redesign / drop) live in
[[Lessons from v1 (reference prototype)]]. See [[ADR-004 — Fresh start (v2) with v1 as reference]].

**Date:** 2026-07-19 (deep pass) · **Scope:** whole engine+runtime (~31k LOC) ·
**Method:** all source `CMakeLists.txt` (dep graph) → LOC map → targeted deep reads
of every central abstraction. Evidence = `path:line`, verified this pass.

---

## 1. Targets & dependency graph

| Target | Type | Links | Role |
|--------|------|-------|------|
| Core | **static** | glm spdlog yaml-cpp assimp Jolt | ECS, events, resources, scene, physics, audio, script, serialization, Logger |
| Client | **dll** | Core, glew, glfw | renderer (graph+passes), ui, window, input |
| Server | **dll** | Core | **pure stub** — `Server : Core` forwards every call, adds nothing (`Server.cpp:17-45`) |
| App | static | — | `Application` + `EntryPoint` |
| Editor | **exe** | App+Client+Server, imgui, ImGuizmo | embeds a full Client **and** Server in-proc; bundles both runtimes; copies engine SDK into `templates/` |
| RuntimeClient/Server | exe | resp. Client/Server | standalone runtimes |

Key edges: Editor → Server (though Server is a stub and its tick is commented out,
`Editor.cpp:142`); **Core static → linked into 2 DLLs** (duplicated globals, F4);
**Core → Editor source** via a `../../../../` include (F12, a layering inversion).

## 2. Central abstractions

| Piece | What | Ref |
|-------|------|-----|
| `Core` | base app: owns `SystemsRegistry`, hardcoded register+init lists | `core/Core.cpp:13-33` |
| `SystemsRegistry` | **service locator** by `type_index`, systems as `shared_ptr`, run serially | `systems/SystemsRegistry.hpp`,`.cpp:19` |
| ECS | archetype + transition edges; IDs from a **reset-able static counter** | `Archetype.hpp:17,30`; `ArchetypesManager.cpp:7` |
| `Query` | typed `each` / `parallelEach` (raw threads) | `components/Query.hpp` |
| Events | pub/sub by `type_index`, events + observers as `shared_ptr` | `eventSystem/EventManager.hpp` |
| Resources | one `ResourceSystem` god-class, 6 hand-rolled per-type APIs, all return `shared_ptr` | `resources/ResourceSystem.hpp` |
| Renderer | god-object System owning graph + a shared `RenderResources` blackboard | `renderer/Renderer.hpp`; `graph/RenderResources.hpp` |
| Scripting | user C++ → DLL by **shelling to CMake+MSBuild** (`_popen`) | `ScriptsCompiler.cpp:36-71` |
| Editor | holds `Server`+`Client`+its own registry; drives an embedded client each frame | `Editor.hpp:18-20`; `Editor.cpp:130-145` |

## 3. Complexity hotspots (LOC)

`ContentBrowserPanel.cpp` 1497 · `MeshLoader.cpp` 1048 · `ComponentDrawerRegistry.cpp`
667 · `ResourceSystem.cpp` 650 · `InspectorPanel.cpp` 526 · `SceneView.cpp` 512 ·
`EntityHierarchyPanel.cpp` 482 · `Components.hpp` 437 (all components in one header).

---

## 4. Findings

Sev: 🔴 correctness/blocker · 🟠 architecture · 🟡 maintainability · ⚪ note.
Verdicts → salvage table in [[Lessons from v1 (reference prototype)]].

### 4.1 Core / ECS / threading

| ID  | Sev | Finding                                                                                                  | Evidence                                                                                                  | Why it matters                                                                                                                                                                                                                                                                                                                                                            |
| --- | --- | -------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| F1  | 🔴  | **ECS component IDs from a reset-able static counter**                                                   | `Archetype.hpp:17,30`; `ArchetypesManager.cpp:7`; **live workaround** `Editor.cpp:87-94`                  | `ComponentType<T>::get()` caches an ID from a process-global static, but each `ArchetypesManager` ctor does `familyCounter=0`. Editor builds a Client **and** a Server world → 2nd reset makes the next new type collide with an existing ID. Serialization rides on these IDs. The editor already **hand-primes all 8 built-ins after init** to dodge it — proof it bit. |
| F5  | 🟠  | **Service-locator `SystemsRegistry` reached-in everywhere**                                              | passed into every system ctor `Core.cpp:17-21`; `getSystem<T>` throws at runtime `SystemsRegistry.hpp:27` | Deps are invisible + resolved at runtime, not compile time. Every system holds a back-ref and pokes siblings. Kills testability and any per-project system composition.                                                                                                                                                                                                   |
| F15 | 🟠  | **No job system; naive threading**                                                                       | `Query.hpp:90-119`; `Parallel.hpp:29-50`; systems serial `SystemsRegistry.cpp:19`                         | `parallelEach`/`parallelFor` **spawn raw `std::thread`s per call and join** (no pool, no graph). Systems run one-by-one. Jolt runs its *own* pool → 3 disjoint threading models. `parallelEach` also leaks a `shared_mutex` into the user callback signature (`Query.hpp:133`) — different API than `each`.                                                               |
| F16 | 🟠  | **"Everything is a System"**                                                                             | `Logger`,`Timer`,`Profiler`,`Viewport`,`FileSystem` all `: public System` (grep)                          | Stateless helpers modeled as lifecycle systems. `Logger`'s state is entirely `static` → being a System buys nothing. Inflates the registry and the serial update loop.                                                                                                                                                                                                    |
| F19 | 🟡  | **Profiler bolted on, allocates in hot loop; dead timing code**                                          | `SystemsRegistry.cpp:16,21`; dead block `:33-45`                                                          | `hasSystem<Profiler>()` + a **`std::string` built per system per frame** in `onUpdate`. `onFixedUpdate` has a `return;` with an unreachable per-system timing loop after it.                                                                                                                                                                                              |
| F20 | 🟡  | **No unified assert/logging strategy; Logger has no source-loc**                                         | `Logger.hpp:47-51`; `assert()` scattered; `std::cout` in physics `PhysicsEngine.cpp:29`                   | Error handling is a grab-bag: raw `assert()`, `throw runtime_error`, `TE_LOGGER_CRITICAL`+`__debugbreak()`, and `cout`. Log macros forward to spdlog with **no function/line** (spdlog `SPDLOG_` source-loc macros unused). No engine assert.                                                                                                                             |
| F10 | ⚪   | `TE_LOGGER_CRITICAL` `__debugbreak()`s **even in release**, un-`do{}while`-wrapped multi-statement macro | `Logger.hpp:51`                                                                                           | Ships a debugger trap; breaks inside `if/else`.                                                                                                                                                                                                                                                                                                                           |
| F24 | 🟡  | **`ComponentsFactory` bugs + hidden coupling**                                                           | `ComponentsFactory.cpp:15-18,25-27,63-116`; global `EventManager*` `:10`                                  | `createTag` leaks a `new char[]` into a `std::string`; `createTransform` **ignores all its args**; creating a physics-component **calls into `PhysicsEngine`** as a side effect.                                                                                                                                                                                          |
| F25 | 🟡  | `Transform` uses **Euler angles** (self-flagged); round-trips lose info                                  | `Components.hpp:76,108-111`                                                                               | Gimbal + lossy `quat→euler` on every physics writeback (see F17).                                                                                                                                                                                                                                                                                                         |

### 4.2 Resources / asset pipeline / filesystem

| ID | Sev | Finding | Evidence | Why it matters |
|----|-----|---------|----------|----------------|
| F13 | 🟠 | **`shared_ptr` overused instead of values/refs/handles** | every `ResourceSystem` getter returns `shared_ptr` `ResourceSystem.hpp:44-125`; 225 `shared_ptr` uses engine-wide | Atomic refcount churn on every resource fetch, incl. render hot paths. Resources want a stable value/handle + `const&` access, not shared ownership everywhere. |
| F7 | 🟡 | **God-files + copy-pasted per-type resource API** | `ResourceSystem.cpp` 650; `MeshLoader.cpp` 1048; `Components.hpp` 437 | 6 resource types each get ~5 near-identical hand-written methods; a generic `IResourceCache<T>` exists but the public API doesn't use it. |
| F30 | 🟠 | **Filesystem impl is editor-only; core has just the interface** | `IFileSystem` in `core/include`; only impl `FileSystem : System, IFileSystem` in `editor` `FileSystem.hpp:15` | Runtimes have no `IFileSystem` implementation, and all asset **loaders live in the editor** — so a shipped runtime can't import assets. Also the FS is a System (see F16). |
| F31 | ⚪ | Resource identity = UUID + name; cross-refs by UUID; caches assume **single-thread** structural edits | `IResource.hpp:31-41`; `ResourceSystem.hpp:131-133` | Fine model, but bakes in the no-threading assumption (relevant to F15). |

### 4.3 Renderer / UI

| ID | Sev | Finding | Evidence | Why it matters |
|----|-----|---------|----------|----------------|
| F22 | 🟠 | **Renderer god-object + shared `RenderResources` blackboard** | `Renderer.hpp:38-59,99-121`; `graph/RenderResources.hpp:19-76` | The graph/passes split is clean, but every pass reads/writes one mutable struct holding *all* FBOs/SSBOs/textures, with attachment contracts encoded in **comments** (`:27`). No declared per-pass resource deps (ADR-001's open item). Renderer also exposes ~15 mutable settings getters the editor pokes. |
| F32 | 🟡 | Render sort allocates | `Renderer.hpp:31-32` | `Renderable::operator<` compares `meshUUID.toString()` → string alloc+compare per comparison in the sort. |
| F23 | 🟡 | **Dual CPU/GPU caches, event-synced** | `GpuResourceManager.hpp:15-31` | CPU `ResourceSystem` (shared_ptr) + GPU `GpuResourceManager` both keyed by UUID, kept in sync via events. Reasonable, but double storage; `GpuResourceManager` exposes a **public `SystemsRegistry&`**. |
| F33 | 🟡 | UI is decomposed but has a fixed vertex cap + two paths | `UIRenderer.hpp:31` | Retained widgets (well split into files) + an immediate `UIRenderer` with a hard `m_maxVertices=30000` (silent overflow). Not the monolith the renderer once was — mild. |

### 4.4 Physics / audio / scripting / server

| ID | Sev | Finding | Evidence | Why it matters |
|----|-----|---------|----------|----------------|
| F11 | 🔴 | **Scripting API hands scripts private (`src/`) types that are never shipped** | `Script.hpp:19-21` exposes `Scene*`/`EventDispatcher*`/`ResourceSystem*`; template copies only `include/` `editor CMakeLists:137-141`; `ResourceSystem.hpp` lives in `core/src` | The base `Script` gives scripts a `ResourceSystem*` **whose header is private and not in the shipped SDK**. Same for anything in `src/`. This is the concrete reason **scripting is broken** after the include/src split. |
| F12 | 🔴 | **Core includes Editor source** (layering inversion) | `PhysicsEngine.cpp:22` → `../../../../runtime/editor/src/resources/loaders/AssimpLoader.hpp` | `engine/core` reaches up into `runtime/editor`. Core can't build/ship without the editor tree; direction of dependency is backwards. |
| F9 | 🟠 | **Scripting compiles by shelling to CMake+MSBuild, synchronously** | `ScriptsCompiler.cpp:16,28,57-70` | `_popen` + a char-by-char `fgetc` read **blocks the calling (editor) thread until the whole build finishes** → the editor freeze Miguel saw. Needs full VS toolchain; generator hardcoded `"Visual Studio 17 2022"`; `char ch=fgetc` EOF bug. |
| F2 | 🟠 | **Editor hard-couples to a Server that does nothing** | `Editor.hpp:18-19`; registers `RuntimeSimulator<Server>` `Editor.cpp:53`; **`server.tick` commented out** `:142`; ships server SDK `editor CMakeLists:143-153` | A full Server is constructed, registered, init'd, SDK-shipped — and never ticked. Speculative generality taxing every layer + per-project duplication. |
| F17 | 🟠 | **Physics keyed by string Tag; Euler round-trip; hard body cap** | `PhysicsEngine.hpp:62-64`; `PhysicsEngine.cpp:143-147,49` | Bodies/triggers/colliders in `unordered_map<std::string, …>` → rename breaks the link, string hashing in the sim loop. Every writeback does `quat→euler→degrees` into the Transform (drift/jitter → the instability). `cMaxBodies=1024` hardcoded. |
| F18 | 🟠 | **Audio too thin; not a resource** | `AudioSystem.hpp:40-44` | `playSound(path)` re-loads from disk each call (no `AudioClip` resource/caching), returns `void` (no handle to stop/adjust), no buses/mixing. ECS emitter/listener integration is superficial. |
| F28 | 🟠 | **EventManager: unsubscribe is unsound; per-event heap alloc** | `EventManager.hpp:20,52-56` | Removing an observer compares `std::function`s (not equality-comparable) → observers can't be reliably removed → dangling-callback risk. Every event is a `shared_ptr<Event>` (per-frame alloc). EventManager is also copyable (`:31-33`). |
| F34 | ⚪ | Server exists only to forward to Core; copy-assigns its registry | `Server.cpp:9-15,17-45` | Confirms F2 from the other side — no server logic to salvage. |

### 4.5 Editor / tooling / build

| ID | Sev | Finding | Evidence | Why it matters |
|----|-----|---------|----------|----------------|
| F14 | 🟠 | **Editor runs inside the game/render loop, one thread** | `Editor.cpp:134-144`; `RuntimeSimulator.hpp:137-138` | `update()` runs ImGui begin → all editor systems → `client.tick` (full sim+render) → ImGui end, sequentially. Even stopped, the embedded client renders every frame. So **editor cost = frame cost**, and any heavy editor task (script build, import) freezes the render. Exactly Miguel's pain. |
| F3 | 🟠 | **No API boundary — `src/` is PUBLIC; editor includes every `src`** | `core CMakeLists:21-26`; `editor CMakeLists:12-21` | `include/` vs `src/` is cosmetic; internals leak; "internal" refactors break the editor and the script SDK (feeds F11). |
| F4 | 🟠 | **Static Core linked into 2 DLLs → duplicated globals** | `core CMakeLists:13`; `client:23`,`server:14`; `Logger.hpp:11`; workaround `ScriptEngine.hpp:28,61` | Two copies of Core's globals (Logger, `familyCounter`). The script engine **passes a log sink across the DLL boundary** to re-unify logging — a workaround that proves the duplication hurts. |
| F6 | 🟡 | **`GLOB_RECURSE` sources in every target** | all `CMakeLists.txt` | Add/remove a file needs a manual reconfigure → silent build drift. |
| F8 | 🟡 | **Zero tests; no test-bed; triplicated per-config CMake** | all targets; `editor CMakeLists` POST_BUILD ×10 | No test target/framework and **no standalone scene to run the engine without the editor**. Every target repeats 3 `set_target_properties` config blocks + heavy POST_BUILD copying. |
| F26 | 🟡 | **CMake fragile under multi-config; undefined var** | `client CMakeLists:30-41,10` | Branches on `CMAKE_BUILD_TYPE STREQUAL` (empty under a VS multi-config generator → `FATAL_ERROR`); assumes single-config (CLion) dirs. `add_library` references undefined `ENGINE_CLIENT_HEADERS`. |
| F27 | 🟡 | **Engine shaders copied per-project → manual re-copy to edit** | `templates/.../assets/shaders/*`; `editor CMakeLists:117-135` | Each created project embeds its own copy of the engine shaders; editing an engine shader means re-copying into every project in use. No shared shader location or hot path back to the engine. |
| F21 | 🟡 | **No entity picking in the scene view** | `SceneView.cpp` (no `raycast`/`pick`/`select` — grep empty) | Can't click an entity in the viewport to select it; selection is hierarchy-panel only. Missing feature. |
| F35 | ⚪ | Editor config emitter malformed; dead fixedUpdate/stop | `Editor.cpp:34-37,147-153` | `BeginMap … EndSeq … EndMap` mismatch; editor `fixedUpdate()`/`stop()` bodies commented out. |

---

## 5. Miguel's known problems → evidence (all confirmed)

| Known problem | Finding(s) | Confirmed at |
|---------------|-----------|--------------|
| Editor+server+client → resource boundaries, dup, DLL mess | F2, F3, F4, F14 | `Editor.hpp:18-20`; `Editor.cpp:68-74,142` |
| No clear boundaries; `SystemsRegistry` passed around | F5, F16 | `Core.cpp:17-21`; `SystemsRegistry.hpp:27` |
| Scripting broken by include/src split | **F11**, F3 | `Script.hpp:19-21`; `editor CMakeLists:137-141` |
| No test-bed; manual shader copy | F8, F27 | `editor CMakeLists:117-135`; no test target |
| Little/no multithreading; want job system + task graph | F15 | `Query.hpp:90-119`; `Parallel.hpp:29-50` |
| Too many `shared_ptr` vs stack/refs | F13 | `ResourceSystem.hpp:44-125` (225 uses) |
| No proper assert/logging strategy | F20, F10 | `Logger.hpp:47-51`; scattered `assert`/`cout` |
| Logger has no function/line context | F20 | `Logger.hpp:47-51` |
| No profiler from the start (hacked) | F19 | `SystemsRegistry.cpp:16,21,33-45` |
| Editor in the game/core loop → fps drops/freeze | **F14**, F9 | `Editor.cpp:134-144`; `ScriptsCompiler.cpp:28` |
| Server / multiplayer forgotten | F2, F34 | `Server.cpp:17-45`; `Editor.cpp:142` |
| Can't click an entity to select it | F21 | `SceneView.cpp` (grep empty) |
| Physics ok but wants stability | F17 | `PhysicsEngine.cpp:143-147,49` |
| Audio too simplistic | F18 | `AudioSystem.hpp:40-44` |
| UI ok but monolithic | F33 (mild — actually decomposed) | `engine/client/src/ui/*`; `UIRenderer.hpp:31` |
| "Almost everything a System" (fs/logger/profiler → helpers) | F16 | grep `: public System` |

Bonus (not on the list): **F12** core→editor include, **F1** live in the editor,
**F24** factory bugs, **F26/F35** CMake + editor-config bugs, **F28** unsound unsubscribe.

## 6. Themes for v2

1. **Kill speculative generality.** No client/server/DLL split until the server is
   real; one clean module story (F2, F3, F4, F34).
2. **Real boundaries + one linkage story.** Curated public headers, private `src`,
   Core not duplicated across DLLs. The script SDK is the acid test (F3, F4, F11).
3. **Stable identity.** Component/type IDs survive multiple worlds + DLLs; don't
   hang serialization off a mutable counter (F1). Physics/audio key off `Entity`,
   not strings (F17).
4. **Explicit dependencies** over service-locator reach-in; pick what's a *system*
   vs a *helper* (Logger/Profiler/Timer/FS/clock are helpers) (F5, F16).
5. **Concurrency by design.** One job system + task graph; ECS iteration, resource
   load, physics all schedule onto it — not 3 ad-hoc threading models (F15).
6. **Ownership discipline.** Values/handles/`const&` over `shared_ptr` by default;
   reserve shared ownership for genuine shared lifetimes (F13).
7. **Editor out of the game loop.** Editor is a host/observer, not a frame-cost
   passenger; heavy tasks (script build, import) run async/off-thread (F9, F14).
8. **Tooling from day one.** Engine test-bed target, unit tests for deterministic
   core, real profiler + assert/log strategy with source-loc, `CMakePresets`, no
   globs (F6, F8, F19, F20, F26, F27).
