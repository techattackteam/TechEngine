# 🔧 B3 — Build & Testing Notes

Working notes feeding the **B3 build + testing baseline ADR** (the *how* — executes
ADR-005/006). Decisions already made live in the ADRs (linked); this collects
**net-new conventions** + a scaffold checklist so B3 starts warm.

## CMake conventions (net-new here)

- **Link what you use.** Every target links *exactly* the libraries it **directly**
  uses (directly `#include`s / calls). **Never rely on transitive visibility for
  correctness** — e.g. `client` declares `base` itself, even though `base` is reachable
  via `platform`; otherwise a change to `platform`'s link visibility silently breaks
  `client`. Static libs dedup at the final exe link, so a direct edge costs nothing.
- **Link visibility rule:** `PUBLIC` if the dep appears in the target's **public
  headers** (propagate to consumers); `PRIVATE` if used only in `.cpp`. Applies to both
  `target_link_libraries` and `target_include_directories`.
- **Per-module include boundary** (from [[ADR-006 — v2 core architecture & module layout]] §3):
  `include/TechEngine/<module>/` **PUBLIC**, `src/` **PRIVATE** — a private include must
  not resolve for a consumer.
- **No `GLOB`** — explicit source lists (v1 F6).
- **One shared `techengine_module()` helper** to stamp out the per-module boilerplate
  (include dirs, visibility, warnings) consistently.

## What B3 executes (decisions already in the ADRs — don't restate, link)

- **Toolchain / std / deps / CI / sanitizers / test framework** → [[ADR-005 — v2 tech stack & toolchain]]:
  C++20, MSVC + Linux/Clang CI matrix, **CMakePresets**, **FetchContent** (+ CI
  dependency **caching** — ccache / actions cache — or build times creep), Catch2 v3 +
  CTest, clang-format + clang-tidy CI-enforced, `/W4 /WX`, ASan (Win) / UBSan+TSan (Linux).
- **Module/target graph + linkage** → [[ADR-006 — v2 core architecture & module layout]] §1:
  `base → platform → core → client → app` static libs + leaf exes (`runtime`, `editor`,
  `tests`), one linkage story, no engine DLLs.
- **Tests** → per-module Catch2 targets under CTest, **not core-only**; rendering
  verified by demo scenes + captures, not unit tests.
- **★ SDK smoke CI target** → compile a sample script against `te_sdk` **alone**; a
  private type leaking into the SDK fails CI (the F11 acid test, automated).
- **Test-bed = sample project(s)** loaded by `runtime`/`editor` (content), not a
  separate exe; `app` exposes a headless/sim-only mode (exercised by `tests`).

## Scaffold checklist (for the ADR / first commit)

- [ ] `CMakePresets.json` (Debug/Release × Win-MSVC / Linux-Clang; ASan/UBSan/TSan presets)
- [ ] `techengine_module()` helper
- [ ] 5 lib skeletons with public/private include split
- [ ] leaf exe targets (`runtime`, `editor`, `tests`)
- [ ] FetchContent manifest + CI caching
- [ ] `.clang-format`, `.clang-tidy` (feeds B4)
- [ ] CI workflow: build matrix + tests + sanitizers + format/tidy + SDK smoke
