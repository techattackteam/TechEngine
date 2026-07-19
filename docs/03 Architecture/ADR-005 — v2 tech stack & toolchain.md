# ADR-005 — v2 tech stack & toolchain

- **Status:** Accepted
- **Date:** 2026-07
- **Deciders:** Miguel (Lead Engineer), with AI as technical lead
- **Related:** [[ADR-004 — Fresh start (v2) with v1 as reference]] · feeds **B1**
  (module layout), **B2** (renderer approach), **B3** (build + testing baseline)
- **Task:** B0 — the *what*. B3 executes the *how*.
- **Amended 2026-07:** repositioned from *"Windows-only ship target"* to
  **OS-independent by design, Windows primary/reference** (Portability row + rationale).
  No technical choice changed — the *no-speculative-OS-abstraction-layer* decision
  stands; only scope/positioning clarified.

## Context

v2 is planned before it is built (ADR-004). Before architecture (B1) or a build
baseline (B3), the load-bearing **stack + toolchain** choices must be fixed: they
constrain every later decision and are expensive to reverse once code exists.

Forces:
- **Solo dev, Windows-first**, day-job cadence — tooling must reduce mistakes and
  regressions without a team to babysit it. The v1 audit found **zero tests, no
  test-bed, no CI, no enforced format/lint, multi-config-fragile CMake** (F6, F8,
  F26). That gap is the main thing this ADR closes.
- **Learn-while-building risk.** The point of v2 is a clean, planned foundation —
  not learning three new APIs at once. Choices bias toward *proven and known*
  over *new and shiny* (see the reflection and Vulkan decisions below).
- **Port-what-worked.** v1's library choices largely worked; the salvage table
  ([[Lessons from v1 (reference prototype)]]) blesses most of them. This ADR
  ratifies those and only changes where there was concrete pain.

## Decision

### Summary table

| Axis                            | Choice                                                                       | Change vs v1                         |
| ------------------------------- | ---------------------------------------------------------------------------- | ------------------------------------ |
| Language / std                  | **C++20**, no modules, no reflection (yet)                                   | same std, disciplined use            |
| Primary dev / reference platform | **Windows / MSVC (VS 2022, ≥ 19.3x)** — built daily, shipped first          | same                                 |
| Portability                     | **OS-independent by design** — portable deps + no OS headers in engine code; **no bespoke OS-abstraction layer** until a 2nd *ship* platform is real | strengthened |
| CI portability leg              | **Linux / Clang** (≥ 16) — **validates OS-independence** + gives UBSan/TSan; not (yet) a ship target | **new**                              |
| Graphics API                    | **OpenGL 4.5+ (core, DSA)** behind a thin internal device seam               | same API, add seam (F22)             |
| Build                           | **CMake ≥ 3.20 + CMakePresets**, no `GLOB`                                   | fix globs (F6) + presets (F26)       |
| Dependencies                    | **FetchContent** (version-pinned) + local-source override for forked deps    | was ad-hoc                           |
| GL loader                       | **glad2**                                                                    | was GLEW                             |
| Window / input                  | **GLFW**                                                                     | same                                 |
| Math                            | **glm**                                                                      | same                                 |
| Logging                         | **spdlog** — *with* source-loc + one assert strategy                         | fix F20/F10                          |
| Physics                         | **Jolt**                                                                     | same                                 |
| Audio                           | **miniaudio**                                                                | same                                 |
| Model import                    | **assimp**                                                                   | same (revisit if build weight bites) |
| Image load                      | **stb_image**                                                                | same                                 |
| Editor UI                       | **Dear ImGui (docking branch)**                                              | same                                 |
| Asset + scene serialization     | **custom binary** — deferred to its own system ADR                           | new, replaces yaml for assets        |
| Settings / config serialization | **toml++** (human-readable)                                                  | replaces yaml-cpp                    |
| Test framework                  | **Catch2 v3**, run via **CTest**                                             | **new** (was none)                   |
| Format / lint                   | **clang-format + clang-tidy**, config checked in, CI-enforced                | **new**                              |
| CI                              | **GitHub Actions** — Win/MSVC + Linux/Clang matrix                           | **new**                              |
| Sanitizers                      | **ASan** (Win/MSVC) · **UBSan + TSan** (Linux/Clang)                         | **new**                              |

### Load-bearing rationale

**C++20, no modules, no reflection.** C++20 is fully supported by MSVC + CLion
with no surprises; C++23 *library* features (`std::expected`, `std::print`,
deducing-this) may be used where MSVC ships them. We **do not** adopt C++20
modules (MSVC+CMake support is still rough — would reintroduce build fragility,
cf. F26) or **C++26 static reflection** (no production MSVC support; building
serialization on an unshipped feature is exactly the speculative dependency the
audit warns against — Theme 1). Reflection's original motivation was
serialization, but v1's serialization pain was **identity, not reflection** (F1).
The custom binary serializer (own ADR) will expose a **trait/registration seam**
so reflection is adoptable later *without* a rewrite.

**OS-independent engine; Windows primary; Linux/Clang in CI.** TechEngine is written
to be **OS-independent** — no `<windows.h>` (or any OS header) leaks into engine
code, and OS-touching work goes through portable libraries (GLFW) or the `platform`
module seam (ADR-006). Windows/MSVC is the **primary development + reference
platform** (built daily, shipped first), **not** a ceiling on where the engine can
run. What we do *not* do is pay for a **bespoke OS-abstraction layer** speculatively:
portability is bought now through portable deps + discipline, and a real HAL is
deferred until a second platform is an actual *ship* target (Theme 1). The
**Linux/Clang CI leg** earns its keep twice — it **validates that OS-independence
continuously** (a stricter compiler catches non-portable code the day it's written)
and it is the *only* way to get **UBSan + TSan** (MSVC has neither). Net sanitizer
story: ASan on Windows, UBSan/TSan on Linux.

**OpenGL 4.5+ behind a device seam.** Learning Vulkan/D3D12 while rebuilding the
engine would blow the focus of the whole v2 move — GL is known and productive.
Target **4.5 core with DSA** (cleaner, modern GPU model). Guardrail tied to the
renderer god-object (F22): passes talk to a **thin internal device/command seam**,
not raw GL, so a future backend can slot in without touching every pass. This is a
*small* seam, **not** a full RHI — a full abstraction now would be speculative
generality. Lands concretely in B2.

**FetchContent, with a local-source escape hatch.** Deps are acquired by CMake,
version-pinned by `GIT_TAG` — no external package manager (vcpkg rejected on
Miguel's experience: footprint + friction). A single mechanism keeps ceremony low.
The one advantage submodules hold — easy patching of a forked dep and fully offline
checkouts — is preserved *without* adopting submodules as a parallel system: any dep
can be overridden to a local checkout (or a submodule) via
`FETCHCONTENT_SOURCE_DIR_<dep>`, leaving the `FetchContent_Declare` calls as the one
source of truth. Accepted cost: clean CI checkouts rebuild deps from source, so
**CI must cache** (ccache / actions cache) or times creep — a B3 item.

**Catch2 v3 + CTest.** Testing surface is **deterministic core** (ECS, math,
serialization, resources); **rendering is verified by demo scenes + captures, not
unit tests** (per `CLAUDE.md`). That surface is value/data assertions, so gmock's
mocking (GoogleTest's main edge) is a weak fit at the cost of the heaviest builds.
doctest wins compile time but has a thinner ecosystem. Catch2 v3 is the balance:
`SECTION`-based shared setup without fixture classes, best-in-class failure
messages, large ecosystem, acceptable compile cost now that v3 is a compiled lib.
**CTest** is the runner (drives the Catch2 binary in CI) — orthogonal to the
framework choice.

**Format/lint checked in, not IDE-local.** A committed `.clang-format` and
`.clang-tidy` make style **mechanical and reproducible** — identical in CLion, in
CI, and for AI collaborators — instead of living in one head. CLion consumes both
natively. Closes the "no enforced style" half of F8; CI runs
`clang-format --dry-run --Werror` + clang-tidy, plus `/W4 /WX`.

## Consequences

**Positive**
- Every gap the audit called out (F6, F8, F26, and the F20/F10 log/assert mess)
  has an owner in this stack: tests, test-bed target (B3), CI, sanitizers,
  presets, enforced format/lint, source-loc logging.
- Choices bias to *known-good*, so B1/B2/B3 build on stable ground, not on
  toolchain experiments.
- The device seam + serialization seam keep the two most likely future pivots
  (render backend, reflection) reversible without a rewrite.

**Negative / open**
- **FetchContent + clean CI = slow builds** unless caching is set up (B3).
- **No TSan/ASan-UBSan on the primary platform** — data-race/UB coverage depends on
  the Linux leg staying green; if it's allowed to rot, that coverage silently dies.
- **assimp is heavy** (build time + the 1048-LOC `MeshLoader` it grew in v1, F7).
  Kept for format breadth; flagged to revisit if glTF-first becomes the real
  pipeline (`fastgltf` + `tinyobjloader`).
- **Custom binary serialization is now on the critical path** for assets/scenes and
  owes its own ADR (format, versioning, endianness, the reflection seam). Until
  then, no asset persistence.
- **C++20 chosen means reflection is re-litigated later**, not never — a future ADR
  can supersede if/when MSVC ships it and the serializer wants it.

## Alternatives considered

- **Latest C++ (C++23/26) for static reflection** — deferred. No production MSVC
  reflection; would put load-bearing serialization on an unshipped feature. The
  real v1 serialization pain was identity (F1), which reflection doesn't fix.
- **vcpkg / Conan for deps** — rejected. FetchContent chosen on Miguel's experience
  (footprint/friction); revisit only if a dep can't be built via FetchContent.
- **Git submodules for deps** — rejected as the *default* mechanism (submodule UX
  tax: detached HEAD, `--recursive` footguns, manual update dance — a recurring cost
  for a solo dev). Its real wins (patch a forked dep, offline vendored checkout) are
  kept via the `FETCHCONTENT_SOURCE_DIR_<dep>` override, so a submodule can still
  back a specific dep when one is actually forked — without a second dep system.
- **Vulkan / D3D12 now** — rejected. Learning a new API while rebuilding the engine
  defeats the focus of the v2 move; GL 4.5 is productive and known.
- **GoogleTest** — rejected. Its strengths (gmock, death tests, scale) are the
  weakest fit for a solo core-logic suite, at the heaviest compile cost.
- **doctest** — viable runner-up; wins compile time, loses ecosystem/features.
  Reconsider only if Catch2 compile times become a measured pain.
- **yaml-cpp (v1) kept for all serialization** — rejected. Justified only while it
  carried assets+scenes; those move to custom binary, leaving a tiny settings
  surface where toml++ (config-purpose-built) fits better.
- **GLEW (v1) for GL loading** — rejected for glad2 (generated, no DLL, targets a
  specific core profile cleanly).

## What would move this decision

So it can be **Accepted deliberately, not by default** — evidence that should
change an axis:

- **Reflection:** MSVC ships production P2996 support **and** a serialization
  prototype shows it materially beats the hand-rolled trait seam → new ADR to adopt.
- **Deps/FetchContent:** a required dependency can't be built cleanly via
  FetchContent, or CI build times stay painful *after* caching → reconsider vcpkg.
- **Graphics:** a target feature (compute-heavy path, bindless, explicit
  multithreaded submission) that GL 4.5 can't serve well → promote the device seam
  to a real RHI + Vulkan backend (its own ADR).
- **Model import:** assimp build weight or the glTF-only pipeline reality →
  switch to `fastgltf` + `tinyobjloader`.
- **Test framework:** measured Catch2 compile-time pain in the core suite → doctest.
- **Portability leg:** if the Linux/Clang leg is chronically red and ignored, either
  fix the commitment or drop it (and lose UBSan/TSan **and continuous OS-independence
  validation** — the price just went up) — don't let it rot silently.

> Add to [[ADR Index]]. Once Accepted, treat as immutable — supersede with a new ADR.
