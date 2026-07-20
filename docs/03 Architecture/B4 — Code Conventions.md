# 📐 B4 — Code Conventions

The code style every session follows. Companion to [[ADR-008 — v2 build & testing baseline]] §8
("net-new build conventions feed B4"). Read by human AND AI.

**Rule 0:** match the surrounding file over any line here. If a file is the outlier,
fix the file — don't fork the convention.

**Source of truth for *formatting* = Miguel's CLion scheme** (global `Default`), not
`.clang-format`. CLion's ClangFormat support is **OFF** (`EnableClangFormatSupport=false`),
so a `.clang-format` would be *ignored* today — this note records the scheme in prose
instead. (If we ever want CI format-gating per ADR-008 §9, that's a separate decision:
turn ClangFormat on and generate a matching `.clang-format`.)

## C++ naming

| Element                         | Style              | Example                    |
| ------------------------------- | ------------------ | -------------------------- |
| Namespace                       | one flat `TechEngine` | `namespace TechEngine {` |
| Types (class/struct/enum/alias) | `PascalCase`       | `RenderGraph`, `FrameCtx`  |
| Functions / methods             | `camelCase`        | `baseVersion()`, `run()`   |
| Members                         | `m_camelCase`      | `m_frameCount`             |
| Locals / params                 | `camelCase`        | `passIndex`                |
| Files (paired `.hpp`/`.cpp`)    | `PascalCase`, match primary type | `Base.hpp` / `Base.cpp` |
| Include path                    | `<TechEngine/<module>/File.hpp>` | `<TechEngine/core/Core.hpp>` |

- **No `te_` / snake_case prefixes on C++ identifiers.** `baseVersion`, not `te_base_version`.
- Header include path is always `TechEngine/<module>/` (basename-collision-proof, ADR-008 §1).

## Formatting (from the CLion scheme)

- **Indent 4 spaces**, no tabs. Continuation indent 8.
- **Braces on the same line** (K&R / attach) — *everywhere*: functions, types,
  namespaces, `if`/`for`/`while`, lambdas. `} else {`, `} catch`, `} while` stay **joined**
  (else/while/catch not on a new line).
- **Namespace body is indented** (+4). Matches CLion `NAMESPACE_INDENTATION=All`.
- **No `} // namespace TechEngine` closing comment** — single flat namespace, the label is
  noise. (Stripped from the scaffold 2026-07-20.)
- `#pragma once` in every header (no include guards).
- Pointer/reference **type-attached, left**: `const char* p`, `Foo& r` — as in the scaffold.
- Right margin is 380 (effectively no hard wrap) — wrap by readability, not a column count.

## Comments

- **Comment *why*, not *what*.** A comment earns its place by explaining intent, a
  non-obvious trick, or a gotcha — or as API/type documentation. If it just restates the
  code, delete it.
- **No per-line / per-include narration.** Do **not** annotate every `#include` (`// core
  -> base`, `// proves X resolves`). The dependency graph lives in the `CMakeLists.txt` +
  [[ADR-006 — v2 core architecture & module layout]], not in inline noise.
- A short comment on a genuinely non-obvious line is fine (e.g. a call that exists only to
  force a link). One line that pulls its weight, not a running commentary.
- Prefer a self-explanatory name over a comment that explains a bad one.

## CMake target naming

- **Module libraries → real target `TechEngine<Module>`** (PascalCase, v1 nomenclature):
  `TechEngineBase`, `TechEngineCore`, `TechEngineClient`, `TechEnginePlatform`,
  `TechEngineApp`. This refines the *illustrative* `te_<module>` spelling in
  [[ADR-008 — v2 build & testing baseline]] §2 (the ADR's real decisions — one helper,
  explicit sources, the `TechEngine::` alias — are unchanged).
- **Link the `TechEngine::<module>` alias, never the real name.** The real target name is
  IDE-/build-output-facing only; consumers (`apps/*`, other modules' `DEPS`) use the alias,
  so it stays the ADR-008 §2 invariant. Set in `cmake/techengine_module.cmake` (pascal-cases
  the module dir name).

## Open — decide when they first bite

| Item | Current | Recommendation |
| ---- | ------- | -------------- |
| Utility interface target | `te_warnings` (+ `TechEngine::warnings`) | Keep `te_` as the **internal build-only** prefix — visually separates shippable module libs (`TechEngine*`) from interface/test targets. |
| Per-module test exes (ADR-008 §6) | not built yet; ADR shows `te_<module>_tests` | Follow the same rule → `te_*_tests` (build-only) *or* `TechEngine<Module>Tests` if we want uniformity. |
| Leaf exes | `editor`, `runtime` | v1 used `TechEngineEditor` / `TechEngineRuntime*` — prefix them to match if we want one scheme. |
| `TechEngine::` alias case | lowercase `TechEngine::core` | Left lowercase = zero churn; Pascal `TechEngine::Core` reads cleaner if we reformat later. |
| Constants / `enum class` values | undecided | Pick when the first real enum/constant lands (candidates: `kPascalCase`, `PascalCase`). |

## Related

- [[ADR-008 — v2 build & testing baseline]] · [[B3 — Build & Testing Notes]] — the build side.
- [[ADR-006 — v2 core architecture & module layout]] — module graph the names describe.
- `CLAUDE.md` → "Code conventions" (the short version AI loads every session).
