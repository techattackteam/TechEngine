# ADR-008 — v2 build & testing baseline

- **Status:** Accepted
- **Date:** 2026-07
- **Deciders:** Miguel (Lead Engineer), with AI as technical lead
- **Related:** [[ADR-005 — v2 tech stack & toolchain]] (the *what*) ·
  [[ADR-006 — v2 core architecture & module layout]] (the *shape*) ·
  [[ADR-007 — v2 networking & ECS replication foundation]] · working notes:
  [[B3 — Build & Testing Notes]] · feeds **B4** (code conventions)
- **Task:** B3 — the *how*. Executes ADR-005's toolchain + ADR-006's module graph as a
  concrete, buildable skeleton. First code lands here.

## Context

ADR-005 fixed the stack, ADR-006 the module graph. Both are still paper — v2 has **zero
code** (only the vault). B3 turns them into a repository that configures, builds, tests,
and lints on both CI legs, and that every later task extends by copying an existing
module rather than inventing structure. Since it is the first commit of real code, its
choices are load-bearing: every include path, every `CMakeLists.txt`, and the CI
contract all reference the layout decided here, and reshaping a tree after N modules
exist is exactly the churn we're avoiding.

What this ADR is **not**: it does not re-decide the toolchain (ADR-005) or the module
seams/linkage (ADR-006 §1). It decides the **physical realization** — directory tree,
the CMake authoring pattern, preset matrix, dependency wiring, test-target layout, and
CI pipeline — plus a short list of net-new build conventions.

**The v1 build is the anti-pattern this closes.** The audit's F6/F8/F26 are concrete
here:
- `engine/core/CMakeLists.txt:1` — `file(GLOB_RECURSE ENGINE_CORE_SRC "src/*.cpp")`: a
  new file silently changes the build, stale caches miss files (F6).
- same file — `target_include_directories(TechEngineCore PUBLIC include src)`: **`src/`
  exported PUBLIC**, so every consumer can `#include` a private header (F3 — the
  boundary ADR-006 §3 makes structural).
- hand-written `findCmake/Find*.cmake` per dep + `find_package(... REQUIRED)`: deps
  resolved from the machine, not pinned (ADR-005 → FetchContent).
- root `CMakeLists.txt`: `set(CMAKE_CXX_STANDARD 20)` + `/MP` and nothing else — no
  presets, no `/W4 /WX`, no sanitizers, no tests, no CI (F8/F26).
- `TECH_ENGINE_CORE_EXPORT` compile-defs — dead export macros under static linkage
  (ADR-006 §3 drops them).

## Decision

### 1. Repository layout

```
/CMakeLists.txt              # top-level: project(), options, deps, add_subdirectory
/CMakePresets.json           # the build contract (§3); CMakeUserPresets.json gitignored
/.clang-format .clang-tidy   # checked in, CI-enforced (feeds B4)
/.github/workflows/ci.yml    # the pipeline (§6)
/cmake/                      # all reusable CMake — the one place logic lives
  techengine_module.cmake    #   the module helper (§2)
  warnings.cmake             #   te_warnings INTERFACE target (§5)
  sanitizers.cmake           #   preset-driven san flags (§5)
  deps.cmake                 #   every FetchContent_Declare, pinned (§4)
/engine/                     # the 5 static libs (ADR-006 §1)
  base/     { include/TechEngine/base/…  src/…  tests/… }
  platform/ { include/TechEngine/platform/… src/… tests/… }
  core/     { include/TechEngine/core/…  src/…  tests/… }
  client/   { include/TechEngine/client/… src/… tests/… }
  app/      { include/TechEngine/app/…   src/…  tests/… }
/apps/                       # leaf exes — thin main(), loop lives in app (ADR-006 §1)
  runtime/        # runtime-server/ added when netcode lands
  editor/
/sdk/                        # te_sdk INTERFACE + curated sdk/include/ tree (ADR-006 §3)
  smoke/          # compile-only leak test (§7)
/tests/
  integration/    # cross-module tests that belong to no single module
/external/                   # committed 3rd-party source ONLY when it can't be fetched
  glad/           # generated GL loader (glad2) — the sole vendored dep
```

**No v1-style `libs/` tree.** Under FetchContent (ADR-005) third-party libs are a
build artifact, not repo content — see §4 for the three cases.

- **Grouped, not flat.** Libs under `engine/`, exes under `apps/`, matching ADR-006's
  lib/exe split so the tree *is* the dependency story. Keeps v1's `engine/` grouping
  (which was fine); drops v1's `runtime/runtimes/{client,server}` nesting for a flat
  `apps/`.
- **Per-module `include/TechEngine/<module>/` + `src/`** — the ADR-006 §3 public/private
  split is physical: PUBLIC `include`, PRIVATE `src`. The `TechEngine/<module>/` prefix
  makes every include read `#include <TechEngine/core/…>` and prevents header basename
  collisions across modules.
- **`cmake/` is the only place build logic lives** — module `CMakeLists.txt` files are
  declarative (list sources, name deps); anything with an `if()` or a loop goes in a
  `cmake/*.cmake` helper. This is the direct antidote to v1's per-target copy-paste.

### 2. CMake authoring — one thin `techengine_module()` helper

A single function in `cmake/techengine_module.cmake` stamps out a library module:

```cmake
techengine_module(core
  DEPS      base platform            # our modules (PUBLIC link)
  DEPS_3RD  Jolt::Jolt tomlplusplus  # third-party
  PRIVATE_3RD ...                    # .cpp-only deps
)
# expands to: add_library(te_core STATIC ${sources})  [explicit list, no GLOB]
#   target_include_directories(te_core PUBLIC include PRIVATE src)
#   target_link_libraries(te_core PUBLIC ... te_warnings)
#   alias TechEngine::core ; set folder/output dirs ; enable clang-tidy
```

- **Sources are an explicit list** passed by the module (F6 dies) — the helper never
  globs.
- The helper encodes the invariants that must be identical everywhere: the
  `PUBLIC include / PRIVATE src` split, `te_warnings` linkage, the `TechEngine::` alias,
  C++20, tidy. A drift in one module becomes impossible.
- **Reject a heavier CMake "framework."** The helper stays a thin, readable wrapper — a
  solo dev + AI must be able to read a module's `CMakeLists.txt` and see exactly its
  sources and deps. Cleverness in CMake is a maintenance tax (v1 proved the opposite
  failure — no abstraction — but the fix is *one* helper, not a DSL).

### 3. Presets — `CMakePresets.json` is the build contract

The matrix from ADR-005 (**Debug / Release / RelWithDebInfo** × Win-MSVC / Linux-Clang,
+ sanitizer variants) is expressed as presets, so local builds and CI run **identical**
configurations.

- **`configurePresets`** via inheritance: a hidden `base` (generator, `_deps` layout,
  `CMAKE_EXPORT_COMPILE_COMMANDS` for tidy), then `windows-msvc` / `linux-clang`, then
  per-config `*-debug` / `*-release` / `*-relwithdebinfo`, then sanitizer leaves:
  `windows-asan`, `linux-ubsan`, `linux-tsan` (each sets a cache var `TE_SANITIZER=...`
  consumed by `sanitizers.cmake`). On the Windows VS **multi-config** generator all
  three configs come from one configure (`CMAKE_CONFIGURATION_TYPES`); on Linux/Clang
  (single-config Ninja) each config is its own preset.
- **RelWithDebInfo is not decorative — it is the config that hosts debuggable native
  script DLLs.** A native script DLL (ADR-006 §3, the one runtime-loaded game module)
  and its host engine must agree on the MSVC **CRT + `_ITERATOR_DEBUG_LEVEL`** when any
  STL object/allocation crosses the boundary: Debug (`/MDd`, IDL 2) ⇔ Debug plugin;
  Release/RelWithDebInfo (`/MD`, IDL 0) ⇔ Release/RelWithDebInfo plugin. RelWithDebInfo
  is **release-ABI *plus* PDBs**, so a *shipped* editor built RelWithDebInfo can load a
  user's RelWithDebInfo script DLL and let them step through **their own code** — without
  a Debug (`/MDd`) editor (which isn't redistributable anyway). Per artifact:
  - `runtime` / `runtime-server`: **Debug + Release** (for now — add RelWithDebInfo
    later for optimized-build profiling + symbolicated crash dumps, CLAUDE.md perf).
  - `editor`: **Debug** (dev) + **RelWithDebInfo** (shipped, so it can host debuggable
    script DLLs).
  > **Deferred to the scripting ADR:** the actual editor↔script-DLL debug + hot-reload
  > contract, and how ABI-clean the SDK boundary is (façade/PIMPL/handles vs. raw STL —
  > a cleaner boundary softens the CRT/IDL coupling above). B3 only guarantees the
  > **config exists in the matrix**; it does not decide the scripting model.
- **`buildPresets` + `testPresets`** mirror them so `cmake --build --preset X` and
  `ctest --preset X` are the whole local + CI command surface.
- **`CMakeUserPresets.json` is gitignored** — local machine overrides (e.g.
  `FETCHCONTENT_SOURCE_DIR_*` for a forked dep, ADR-005) live there, never in the
  committed file.

### 4. Dependencies — one pinned manifest

- **All `FetchContent_Declare` live in `cmake/deps.cmake`**, each `GIT_TAG`-pinned to a
  commit/tag (ADR-005), `FetchContent_MakeAvailable(...)` called once from the top-level
  `CMakeLists.txt` before `add_subdirectory(engine)`. One source of truth; a module just
  names the target it links.
- **Local/forked override** via `FETCHCONTENT_SOURCE_DIR_<dep>` (ADR-005) — kept in
  `CMakeUserPresets.json`, so the declare list stays canonical.
- **CI caches or times creep** (ADR-005's open item): cache the `_deps` source/build and
  use **ccache** (Linux) / a compiler cache action, keyed on a hash of `deps.cmake` +
  compiler. A clean CI checkout otherwise rebuilds every dep from source each run.

**Where third-party libs physically live — three cases** (drops v1's vendored `libs/` +
`findCmake/`):
1. **Normal fetched dep** (Jolt, glm, spdlog, GLFW, Catch2, toml++, assimp, …) → fetched
   into `<build>/_deps/`, a **build artifact, gitignored**, never in source control. The
   `deps.cmake` declare is the only committed trace.
2. **Forked / offline dep** → a local checkout or submodule that
   `FETCHCONTENT_SOURCE_DIR_<dep>` points at (path in `CMakeUserPresets.json`); committed
   only if you deliberately choose a submodule.
3. **Not a fetchable CMake project** — in practice just **glad2's generated GL loader** →
   committed under `external/glad/` and wrapped in a small target. The one vendored
   exception.
   **Single-header libs (stb_image, miniaudio) are still fetched** from their repos and
   wrapped in an `INTERFACE` target in `deps.cmake` — *not* vendored (v1 dropped them in
   `libs/` and `src/audio/`; we don't).

### 5. Warnings & sanitizers — applied to *our* targets only

The one real gotcha: `/WX` must never reach FetchContent'd third-party code, or vendored
deps fail to build on a warning we don't own.

- **`te_warnings`** — an `INTERFACE` target (`cmake/warnings.cmake`) carrying `/W4 /WX`
  (MSVC) and `-Wall -Wextra -Wpedantic -Werror` (Clang). Linked **only** by
  `techengine_module()` and the leaf exes — third-party targets get their defaults.
- **Sanitizers** are preset-driven (§3): `sanitizers.cmake` reads `TE_SANITIZER` and
  adds the compile/link flags to `te_warnings`' consumers only. ASan on Win/MSVC, UBSan
  + TSan on Linux/Clang (ADR-005). Never hard-coded per target.
- **clang-tidy** runs via `CMAKE_CXX_CLANG_TIDY` on our targets (the `.clang-tidy` from
  ADR-005), off for `_deps`.

### 6. Tests — per-module Catch2 exes under CTest

Executes ADR-006's "per-module Catch2 targets, not core-only."

- **Each module owns a colocated `tests/` dir** producing its own exe
  (`te_core_tests`, `te_base_tests`, …) linked against that module + Catch2. A module is
  self-contained: its public surface and the test of that surface sit together. **No
  single monolithic test exe** — that reintroduces a god-target and couples unrelated
  modules' link times.
  > Reconciles ADR-006 §1's "tests" exe row: that row is shorthand for the test
  > *surface*; the realization is N per-module exes, not one binary.
- **`catch_discover_tests`** registers each exe's cases with CTest, so
  `ctest --preset …` runs everything and CI reports per-case.
- **Cross-module integration tests** (touch >1 module) live in top-level
  `tests/integration/` as their own exe — they belong to no single module.
- **A helper `techengine_test(<module>)`** mirrors §2 for test exes (Catch2 link, tidy
  off or relaxed, discovery) to keep them boilerplate-free.
- **Rendering is not unit-tested** — demo scenes + captures (ADR-005/CLAUDE.md). The
  `tests` surface is deterministic core (ECS, math, serialization, resources).

### 7. SDK smoke — the F11 acid test, automated

A `te_sdk_smoke` target under `sdk/smoke/`: a single `.cpp` that includes only
`<TechEngine/sdk/…>` and links `te_sdk` **alone** (the `INTERFACE` target, ADR-006 §3).
If any engine-private type leaks into the public SDK surface, the include fails to
resolve and the target fails to compile. Wired as a CI build step (and a CTest build
fixture), so a leak breaks the build the day it's introduced.

### 8. Net-new build conventions (feed B4)

- **Link what you use, directly.** Every target links exactly the libs it directly
  `#include`s/calls — never rely on transitive visibility for correctness (a `client`
  that uses `base` declares `base`, even though `platform` also pulls it). Static libs
  dedup at the final exe link, so a direct edge is free and a link-graph change can't
  silently break a distant consumer.
- **Visibility rule:** `PUBLIC` if the dep appears in the target's public headers,
  `PRIVATE` if only in `.cpp` — for both `target_link_libraries` and
  `target_include_directories`.
- **No `GLOB`** — explicit source lists (§2).
- **No export macros** — dead under static linkage (ADR-006 §3; drops v1's
  `TECH_ENGINE_*_EXPORT`).

### 9. CI pipeline — GitHub Actions, gated on PRs

CI is the enforcement layer: nothing in this ADR is real unless a red check blocks the
merge. (ADR-005 chose GitHub Actions.)

- **Trigger:** `pull_request` targeting `master` (+ `push` to `master` as a backstop).
  Fits the workflow — CLAUDE.md branches off `master`, never commits directly, so every
  change lands via a PR and CI runs there.
- **Required status checks** (branch protection — a red one blocks the merge button, the
  antidote to silent rot): build+test on **both legs** (Win-MSVC, Linux-Clang) ×
  {Debug, Release} + `clang-format`/`clang-tidy` + `te_sdk_smoke`.
- **Sanitizers — Option A: all per-PR** (ASan/Win · UBSan/Linux · TSan/Linux). Affordable
  because matrix legs run **in parallel** (≈ one extra build's wall-clock, not the sum)
  and the deterministic core suite runs in seconds even under TSan's 5–15×. Three caveats
  live with this:
  - **Cost — no paid CI for now:** billed minutes must stay inside the **free
    allowance** (private repo ≈ 2,000 min/mo; **Windows legs bill 2×**, Linux 1× — the
    sanitizer legs are the biggest driver). If they push past it → **switch to Option B
    wholesale**: all sanitizers to a nightly `schedule` + `workflow_dispatch`, plain
    build+test per-PR. This is the *cost* trigger (demotes everything); distinct from the
    latency trigger below (demotes only TSan).
  - **Latency trigger:** if PR p50 wall-clock crosses **~10–12 min** (it'll be the
    *growing* suite under TSan, not the build) → move **TSan to a nightly `schedule`**.
    A number, not a feeling.
  - **TSan false positives can block a clean merge:** TSan ships **required but with a
    checked-in suppressions file**; promote to zero-tolerance once it's proven stable.
- **Caching is mandatory** (§4) — each sanitizer is its own cache namespace; a cold leg
  rebuilds heavy deps (assimp) under instrumentation. ccache + `_deps` cache keyed on
  `deps.cmake` + compiler + `TE_SANITIZER`.
- **Stage order (fail cheap first):** format/tidy → build → ctest → SDK-smoke, per leg.

### Scaffold checklist (first commit(s))

- [ ] `cmake/` helpers: `techengine_module`, `warnings`, `sanitizers`, `deps`
- [ ] top-level `CMakeLists.txt` + `CMakePresets.json` (full matrix + san presets)
- [ ] 5 lib skeletons (`engine/*`) with the public/private include split, each compiling
      empty
- [ ] leaf exe targets `apps/runtime`, `apps/editor` (thin `main()`)
- [ ] one real per-module test exe (e.g. `te_base_tests`) proving the Catch2+CTest path
- [ ] `sdk/` INTERFACE target + `te_sdk_smoke`
- [ ] `.clang-format`, `.clang-tidy`
- [ ] `.github/workflows/ci.yml`: matrix build → format+tidy → ctest → sanitizer legs →
      SDK smoke, with dep/ccache caching

## Consequences

**Positive**
- F6, F3, F8, F26 die in the first commit: explicit sources, structural private headers,
  presets + `/WX` + tidy + tests + CI + sanitizers all present from line one.
- Adding a module is *copy a sibling dir + one `techengine_module()` call* — structure
  is not re-invented per task, and every module is enforced-consistent.
- Local and CI builds are the same presets — "works on my machine" divergence is
  designed out.
- The boundary and SDK guarantees from ADR-006 are actually *checked* (private headers
  unreachable; SDK-smoke fails on a leak), not just documented.

**Negative / open**
- **CI build time** rides entirely on caching working (ADR-005's open item) — a cold
  cache rebuilds all of FetchContent. First real risk to watch.
- **More up-front CMake** than a single globbed target — mitigated by the one helper;
  still far less than v1's per-target hand-authoring.
- **Per-module test exes multiply link steps** — N small links vs. one big one. Fine at
  5 modules; revisit if link time bites (below).
- **Sanitizer coverage depends on the Linux leg staying green** (ADR-005) — TSan/UBSan
  live only there; a chronically-red leg silently kills that coverage.
- **The Windows/Linux preset split doubles the config surface** to keep maintained — the
  cost of the CI quality lever.

## Alternatives considered

- **Keep v1's `find_package` + hand-written `Find*.cmake`** — rejected: unpinned,
  machine-dependent resolution; ADR-005 already chose FetchContent. The `findCmake/`
  tree is prior art, not a model.
- **A single monolithic test exe** — rejected: couples unrelated modules' compile/link,
  recreates a god-target, and contradicts ADR-006's per-module targets. Revisit only on
  measured link-time pain.
- **Flat top-level module dirs** (`/base /core …`) — rejected: loses the lib-vs-exe
  grouping that makes ADR-006's dependency story legible at a glance; `engine/` + `apps/`
  reads as the module graph.
- **A richer CMake function library / DSL** — rejected: cleverness in the build is a tax
  a solo dev + AI pay every session; one thin helper is the whole abstraction. (v1's
  failure was *no* abstraction, not too little — the fix is one helper, not a framework.)
- **`GLOB_RECURSE` "for convenience"** (v1) — rejected outright: silent, cache-stale
  builds (F6); explicit lists are the point.
- **Global `/WX`** — rejected: breaks FetchContent'd deps; warnings ride the
  `te_warnings` interface so only our code is held to `-Werror`.
- **Colocated integration tests inside a module** — rejected for cross-module tests: they
  belong to no single module and would force an artificial owner; unit tests stay
  colocated, integration goes top-level.

## What would move this decision

So it's Accepted deliberately, not by default — evidence that should change an axis:

- **Test granularity:** measured link-time regression from N per-module test exes → merge
  toward fewer/one exe. (Cheap to reverse — the trigger is a number, not a feeling.)
- **CMake helper:** if `techengine_module()` grows conditionals/special-cases per module,
  the abstraction is wrong — split it or drop back to explicit `CMakeLists.txt`.
- **Layout:** if colocated `tests/` dirs bloat modules or slow configure meaningfully →
  central `tests/` tree mirroring `engine/`.
- **Dep caching:** if CI times stay painful *after* ccache + `_deps` caching → reconsider
  vcpkg (ADR-005's stated FetchContent exit).
- **Linux leg:** chronically red/ignored → fix the commitment or drop it and lose
  UBSan/TSan (ADR-005) — don't let it rot silently.
- **Presets:** if the Win/Linux preset matrix drifts out of sync in practice → a
  generated/toolchain-file approach instead of hand-maintained presets.

> Add to [[ADR Index]]. Once Accepted, treat as immutable — supersede with a new ADR.
