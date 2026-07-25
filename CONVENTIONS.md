# CONVENTIONS.md — TechEngine code conventions

The house style. Read by human AND AI, every session. One style — Claude matches it; there is
no separate "AI style".

**Rule 0:** match the surrounding file over any line here. If a file is the outlier, fix the
file — don't fork the convention.

> **Status: in progress (S2-T10, Sprint 02).** Written *while* `base` is being built so rules
> land the day they're decided instead of being reconstructed at sprint end. Sections marked
> **⚠️ provisional** are Claude's reading of existing code, not Miguel's ratified call — see
> *Open* at the bottom.

## Mechanical vs judgment

| Layer | Home | Enforcement |
|---|---|---|
| **Formatting** — indent, braces, column limit, pointer alignment | `.clang-format` | CI-enforced (`--dry-run --Werror`), **required check** on `master` |
| **Static analysis** — bugprone, performance, const-correctness | `.clang-tidy` | CI-enforced on the Linux/Clang leg |
| **Judgment** — naming, linkage, comments, includes, ownership | **this file** | Review |

`.clang-format` and `.clang-tidy` **win on any conflict** with prose here. This file does not
restate them; it covers what a formatter cannot decide.

## Naming

| Element | Style | Example |
|---|---|---|
| Namespace | one flat `TechEngine` | `namespace TechEngine {` |
| Types (class/struct/enum/alias) | `PascalCase` | `RenderGraph`, `LogRecord` |
| Functions / methods | `camelCase` | `baseVersion()`, `setMinLevel()` |
| Members | `m_camelCase` | `m_frameCount` |
| Locals / params | `camelCase` | `passIndex` |
| Files (paired `.hpp`/`.cpp`) | `PascalCase`, match primary type | `Log.hpp` / `Log.cpp` |
| Include path | `<TechEngine/<module>/File.hpp>` | `<TechEngine/base/Log.hpp>` |
| **Constants** | `SCREAMING_SNAKE_CASE` | `TRUNCATION_MARKER`, `MESSAGE_CAPACITY`, `FIXED_DT` |
| **Macros** | `SCREAMING_SNAKE_CASE`, **`TE_` prefix** | `TE_LOGGER_INFO`, `TE_LOG_ACTIVE_LEVEL` |
| **`enum class` values** ⚠️ provisional | `PascalCase` | `Level::Info`, `Level::Critical` |
| **File-scope mutable state** ⚠️ provisional | `g_camelCase` + `static` | `g_minLevel` |

- **No `te_` / snake_case prefixes on C++ identifiers.** `baseVersion`, not `te_base_version`.
- Include path is always `TechEngine/<module>/` — basename-collision-proof (ADR-008 §1).
- **`TE_` is what separates a macro from a constant** — both are `SCREAMING_SNAKE`, so the
  prefix carries the distinction. A macro has no namespace; the prefix *is* its namespace.
  Never put `TE_` on a constant.
- **Constants refine the `kPascalCase` spelling that appears in the ADRs** (`kFixedDt` in
  ADR-007 §5, [[Game Loop — Frame Flow]]). Those are *illustrative*, not decisions — the same
  precedent by which `TechEngine<Module>` refines ADR-008 §2's `te_<module>`. Accepted ADRs are
  not edited; read `kFixedDt` there as `FIXED_DT`.
- **Gotcha:** all-caps identifiers are macro territory to the preprocessor, so a constant can be
  clobbered by a third-party or OS macro of the same name (Windows headers define `ERROR`,
  `DELETE`, `MAX_PATH`, …). Avoid bare single-word names that read like OS macros; prefer a
  qualifying word (`MESSAGE_CAPACITY`, not `CAPACITY`).

## Internal linkage — `static`, not `namespace {}`

> **Anonymous namespaces are a last resort.** Reach for `static` at file scope. AI-written C++
> defaults to wrapping half a `.cpp` in `namespace { … }`; we don't.

| Hiding a… | Use | Why |
|---|---|---|
| free function | `static` | States the linkage in one word, at the declaration |
| file-scope variable / constant | `static` | Same — and it stays greppable |
| **type** | plain declaration in the `.cpp` | `static` doesn't apply to types; a `.cpp`-local type is already unreachable from other TUs via the include graph |

**Why not `namespace {}` by default:** it indents a whole file for no semantic gain; it buries
the linkage fact at the top of a block instead of on the entity (`static void foo` tells you at
the definition, `void foo` 200 lines inside a block does not); and it invites dumping
*everything* internal into one bucket, which reads as a second file inside the file.

**The one case it earns:** a `.cpp`-local **type** whose name could plausibly collide with a
*different* type of the same name in another TU. Same-name-different-type at external linkage is
a real ODR violation — silent, surfacing as a link-time or runtime mystery. If that risk is
real, wrap **just that type** and leave the functions `static`. Prefer a more specific type name
first (`LogFormatBuffer`, not `FormatBuffer`) — that removes the risk without the namespace.

## Comments

> **Default: no comment.** Not "comment *why* not *what*" — that bar was already in force and
> still produced narrated code (the S2-T2 Logger scaffold, 2026-07-25). Higher bar: a comment
> must survive *"would a competent reader be wrong without this?"* If no, delete it.
> **Scaffolding is not a licence to narrate.**

**The only three things that earn a comment:**

| Keep | Example |
|---|---|
| A **gotcha** that will bite — non-obvious lifetime, a footgun the compiler allows | `message` points into the dispatch call's buffer; a sink that outlives the call must copy |
| A **`TODO(S2-Tn)`** marking deliberately unfinished work | `TODO(S2-T3): per-channel level array replaces this global` |
| A bare **`§ref`** to the decision | `// ADR-011 §1` |

**Delete on sight:**

- **Section-divider banners** — `// --- the seam ---`. Use structure, not ASCII furniture.
- **Rationale copied from the vault.** The ADR/design note is the home for *why*; a `§ref` is
  the whole comment. Copied rationale drifts from its source.
- **File / class / function preambles** explaining purpose. The name and the header do that.
- **Per-line or per-include narration** — never annotate `#include`s (`// core -> base`).
- **Anything restating the code**, including "why this obvious idiom".

Prefer a self-explanatory name over a comment explaining a bad one. A `§ref` beats a paragraph;
no comment beats a `§ref` that adds nothing.

## Privacy that the compiler can't enforce

Module boundaries are structural (`PUBLIC include/` vs `PRIVATE src/`, ADR-006 §3) — but two
things escape that: **macros have no access control**, and `detail::` is a convention, not a
boundary. Both live in public headers by necessity (a macro must expand in the consumer's TU; a
template must be visible). So:

- **Name it private:** `TE_<AREA>_PRIVATE_<VERB>` for plumbing macros (`TE_LOG_PRIVATE_EMIT`),
  `detail::` for functions the macros call.
- **Say what to call instead**, at the definition — a `§ref` isn't enough here, because the
  reader is about to make a mistake the compiler will accept.
- **Guard it in CI.** `ci.yml` → *No reaching into private plumbing* greps for each private
  pattern outside its owning module and fails the build. Add a `check` line when you add a
  private symbol; a rule with no red check rots (ADR-008 §9), and this is the same move as the
  `te_sdk_smoke` leak gate (ADR-008 §7).

**Why it matters beyond tidiness:** `TE_LOG_PRIVATE_EMIT` bypasses the compile-time level gate,
so a direct call ships `Trace` logging into a Release build — the macro is not merely internal,
it is *wrong* to call. That is the test to apply: if reaching past the public API silently breaks
a guarantee, it needs a guard, not a comment.

## Includes — ours first, always

**Our headers come before any external or standard header.** No exceptions, including in tests.

```cpp
#include <TechEngine/base/Log.hpp>   // 1. the paired header, first, alone
                                     //
#include <TechEngine/base/Clock.hpp> // 2. other TechEngine headers
                                     //
#include <spdlog/spdlog.h>           // 3. third-party (incl. catch2)
                                     //
#include <array>                     // 4. standard library
#include <atomic>
```

**Why ours first — it is not cosmetic.** An external or std header included above ours will
satisfy symbols our header forgot to include, so the omission compiles here and breaks at the
next call site. Ours-first makes the header prove it is **self-sufficient**, which is what keeps
the number of includes down: each header carries exactly what it needs, and no more.

- Blank line between groups; alphabetical **within** a group.
- **Tests obey the same rule** — the unit under test comes before `<catch2/…>`, not after.
- Angle brackets throughout — every path resolves through a target's include dirs, so there are
  no `"relative"` includes.

**Enforced, not remembered.** `.clang-format` has `IncludeBlocks: Regroup` +
`IncludeCategories`, so clang-format *moves* includes into these groups — CLion applies it
(it reads `.clang-format`; no IDE setting to configure) and the CI format gate blocks a merge
that violates it. `IncludeIsMainRegex: '(Tests)?$'` is what makes `LogTests.cpp` treat
`Log.hpp` as its main header so it sorts to the top.

## Headers

- `#pragma once`, never include guards.
- **Forward-declare in headers; include in the `.cpp`.** A public header pulling a heavy
  transitive include is a cost every consumer pays.
- **No third-party type in a module's public header** unless the ADR says so — that's the
  façade rule, and it's what keeps backends swappable (ADR-011 §1 is the worked example).
- A module's public header must not expose a *lower* module's private type (ADR-006 §3).

## CMake

- **Module libraries → real target `TechEngine<Module>`** (`TechEngineBase`, `TechEngineCore`).
  Refines the illustrative `te_<module>` spelling in ADR-008 §2.
- **Link the `TechEngine::<module>` alias, never the real name** — the real name is
  IDE/build-output-facing only.
- Module `CMakeLists.txt` files stay **declarative**: sources and deps. Anything with an `if()`
  or a loop goes in a `cmake/*.cmake` helper (ADR-008 §1).
- **Explicit source lists, never `GLOB`** (ADR-008 §2, kills v1's F6).
- **Visibility:** `PUBLIC` iff the dep appears in the target's *public headers*, else `PRIVATE`
  (ADR-008 §8). Link what you use, directly.
- Generator expressions over `if()` for per-config values — Windows is multi-config, so the
  config is unknown at configure time.

## Open — decide when they first bite

Flag these as they come up; each becomes a rule above.

| Item | Current | Notes |
|---|---|---|
| `enum class` value casing | ⚠️ `PascalCase` (provisional) | First real enum landed in S2-T2 (`Level`). Artifacts already write `Level::Info`. **Needs ratifying.** |
| Nested impl namespace | `TechEngine::detail` in use | Cuts against "one flat `TechEngine`". Macros need a non-curated target to call. **Needs ratifying.** |
| File-scope state prefix | ⚠️ `g_camelCase` (provisional) | `g_minLevel`. Open whether `g_` survives now that constants are `SCREAMING_SNAKE` and `static` already marks linkage. |
| **Ownership / smart pointers** | undecided | Fixes v1 F13. Proposed default: value + **handle** (index + generation); `unique_ptr` = single heap ownership; raw ptr/ref = non-owning, never deletes; `shared_ptr` only for genuine shared + unclear lifetime. Trigger: first real ownership decision. → [[Backlog]] |
| const-correctness | `misc-const-correctness` on in `.clang-tidy` | Mechanical for locals. Open: params/methods by hand? |
| Error handling | undecided | Exceptions vs `std::expected` vs error codes. Note `logDispatch` catches to keep diagnostics from killing logic. Trigger: first fallible API. |
| Utility interface target | `te_warnings` | Keep `te_` as the internal build-only prefix — separates shippable libs (`TechEngine*`) from interface/test targets. |
| Per-module test exes | `TechEngine<Module>Tests` | Built that way in `cmake/techengine_test.cmake`; ADR-008 §6 showed `te_<module>_tests`. Uniformity vs the build-only prefix above. |
| Leaf exes | `editor`, `runtime` | v1 used `TechEngineEditor`; prefix to match if we want one scheme. |
| `TechEngine::` alias case | lowercase `TechEngine::core` | Lowercase = zero churn; Pascal reads cleaner. |

## Related

- [[B4 — Code Conventions]] — the vault-side note; now a pointer here + history.
- [[ADR-008 — v2 build & testing baseline]] §1 §2 §8 · [[ADR-006 — v2 core architecture & module layout]] §3
- `.clang-format` · `.clang-tidy` — the mechanical subset.
- `CLAUDE.md` → "Code conventions" — the short subset AI loads every session.
