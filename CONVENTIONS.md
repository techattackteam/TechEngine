# CONVENTIONS.md — TechEngine code conventions

The house style. Read by human AND AI, every session. One style — Claude matches it; there is
no separate "AI style".

**Rule 0:** match the surrounding file over any line here. If a file is the outlier, fix the
file — don't fork the convention.

> Written *while* `base` is being built so rules land the day they're decided instead of being
> reconstructed at sprint end.

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
| Include path | `<TechEngine/<module>/<utility>/File.hpp>` | `<TechEngine/base/diagnostics/Log.hpp>` |
| **Constants** | `SCREAMING_SNAKE_CASE` | `TRUNCATION_MARKER`, `MESSAGE_CAPACITY`, `FIXED_DELTA_TIME` |
| **Macros** | `SCREAMING_SNAKE_CASE`, **`TE_` prefix** | `TE_LOGGER_INFO`, `TE_LOG_ACTIVE_LEVEL` |
| **`enum class` values** | `PascalCase` | `Level::Info`, `Level::Critical` |
| **File-scope mutable state** | `g_camelCase` + `static` | `g_minLevel` |

- **No `te_` / snake_case prefixes on C++ identifiers.** `baseVersion`, not `te_base_version`.
- Include path is always `TechEngine/<module>/` — basename-collision-proof (ADR-008 §1).
- **`TE_` is what separates a macro from a constant** — both are `SCREAMING_SNAKE`, so the
  prefix carries the distinction. A macro has no namespace; the prefix *is* its namespace.
  Never put `TE_` on a constant.
- **Constants refine the `kPascalCase` spelling that appears in the ADRs** (`kFixedDt` in
  ADR-007 §5, [[Game Loop — Frame Flow]]). Those are *illustrative*, not decisions — the same
  precedent by which `TechEngine<Module>` refines ADR-008 §2's `te_<module>`. An ADR is not
  amended for a spelling it never decided, so read `kFixedDt` there as `FIXED_DELTA_TIME`
  (see *Names are spelled out*).
- **Gotcha:** all-caps identifiers are macro territory to the preprocessor, so a constant can be
  clobbered by a third-party or OS macro of the same name (Windows headers define `ERROR`,
  `DELETE`, `MAX_PATH`, …). Avoid bare single-word names that read like OS macros; prefer a
  qualifying word (`MESSAGE_CAPACITY`, not `CAPACITY`).

## Names are spelled out

> **Default: full words.** `deltaTime`, not `dt`. `accumulator`, not `acc`. `previous`, not `prev`.
> Decided 2026-07-30 (S2-T7) — the loop's `dt`/`fixedDt` were the trigger.

The bar is the **reader**, not the typist: a name is read far more often than typed, and IDE
completion makes the length free. Abbreviate only when the full spelling makes a line
genuinely unwieldy — and then abbreviate the *whole* codebase's way, not that file's way.

**Not abbreviations — leave these alone:**

| Case | Example |
|---|---|
| Established acronyms, cased as words | `NetId`, `uuidOf()`, `httpClient` — ADR-007's `NetId` spelling |
| Single letters that **are** the domain notation | loop counters `i`/`j`, matrix `m`, texcoords `u`/`v`, `alpha` for the interpolation factor |
| A term of art whose expansion is worse | `alpha`, not `interpolationFactor`; `tick`, not `simulationStepIndex` |

**The ADRs and design notes write `dt` / `fixedDt`.** Read them as `deltaTime` / `fixedDeltaTime`
in code — the same refinement precedent as `kFixedDt` → `FIXED_DT` and `te_<module>` →
`TechEngine<Module>` above. An ADR is not amended for a spelling it never decided.

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
| A **`TODO(D<n>)`** pointing at a known defect — prefer this over a card ID: cards die and their IDs go stale, `D<n>` doesn't | `TODO(D1): gate defaults to Trace if this TU never links base` |

**Delete on sight:**

- **Section-divider banners** — `// --- the seam ---`. Use structure, not ASCII furniture.
- **Namespace closing comments** — `} // namespace TechEngine`. One flat namespace per file
  makes the brace unambiguous; the comment is pure furniture.
- **Rationale copied from the vault** — and the citation itself; see *Comments never cite the
  vault*.
- **File / class / function preambles** explaining purpose. The name and the header do that.
- **Per-line or per-include narration** — never annotate `#include`s (`// core -> base`).
- **Anything restating the code**, including "why this obvious idiom".

Prefer a self-explanatory name over a comment explaining a bad one.

## Comments never cite the vault

> **No `ADR-011 §2`, no `[[Logger — Design]]`, no bare `§ref` — in any comment, in any module,
> tests included.** Code and comments stand alone. Decided 2026-08-07; this reverses the earlier
> "a bare `§ref` is the whole comment" rule.

A reader who needs the ADR open to follow the line is reading a line that isn't clear enough yet.

| The ref was… | Do this |
|---|---|
| standing in for rationale | Delete it. If the *why* is load-bearing **at that line**, state the mechanical fact in the reader's own terms — no source named. |
| tacked onto a real gotcha — `// process-global (ADR-011 §8), so restore it` | Keep the gotcha, drop the parenthetical. |
| the entire comment — `// ADR-011 §1` | Delete the line. |

**Why, beyond noise:** a citation is a promise the code cannot keep. Section numbers shift, notes
get renamed, and the vault is a **separate repo** — a fresh engine clone has no `docs/` at all, so
the ref resolves to nothing for the reader most likely to need it. Traceability is git's job: the
branch name (`S3-T8/Event-Registry`) is the link from a commit back to its card.

**Not refs, and they stay:** `TODO(S3-T8)` and `TODO(D1)`. They mark unfinished work rather than
explaining finished code, and they read as IDs, not as reading assignments.

**This file, the vault, CMake and CI still cite freely** — the rule covers `.hpp` / `.cpp` /
tests.

## Privacy that the compiler can't enforce

Module boundaries are structural (`PUBLIC include/` vs `PRIVATE src/`, ADR-006 §3) — but two
things escape that: **macros have no access control**, and `internal::` is a convention, not a
boundary. Both live in public headers by necessity (a macro must expand in the consumer's TU; a
template must be visible). So:

- **Name it private:** `TE_<AREA>_PRIVATE_<VERB>` for plumbing macros (`TE_LOG_PRIVATE_EMIT`),
  `internal::` for functions the macros call.
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
#include <TechEngine/base/diagnostics/Log.hpp> // 1. the paired header, first, alone
                                               //
#include <TechEngine/base/time/Clock.hpp>      // 2. other TechEngine headers
                                               //
#include <diagnostics/FormatBuffer.hpp>        // 3. our module-private headers
                                               //
#include <spdlog/spdlog.h>                     // 4. third-party (incl. catch2)
                                               //
#include <array>                               // 5. standard library
#include <atomic>
```

**Why ours first — it is not cosmetic.** An external or std header included above ours will
satisfy symbols our header forgot to include, so the omission compiles here and breaks at the
next call site. Ours-first makes the header prove it is **self-sufficient**, which is what keeps
the number of includes down: each header carries exactly what it needs, and no more.

- Blank line between groups; alphabetical **within** a group.
- **Tests obey the same rule** — the unit under test comes before `<catch2/…>`, not after.
- Angle brackets throughout — every path resolves through a target's include dirs, so there are
  no `"relative"` includes. A module's own private headers under `src/` are reached the same
  way: `<diagnostics/FormatBuffer.hpp>`, never `"FormatBuffer.hpp"`. Both
  `techengine_module()` and `techengine_test()` put `src/` on the target's include path.

**Enforced, not remembered.** `.clang-format` has `IncludeBlocks: Regroup` +
`IncludeCategories`, so clang-format *moves* includes into these groups — CLion applies it
(it reads `.clang-format`; no IDE setting to configure) and the CI format gate blocks a merge
that violates it. `IncludeIsMainRegex: '(Tests)?$'` is what makes `LogTests.cpp` treat
`Log.hpp` as its main header so it sorts to the top.

**One hole to know about.** Group 3 and group 4 are the same shape to a regex, a lowercase
root plus a path, so `.clang-format` tells them apart by **enumerating the third-party roots**.
A new dependency that is not added to that row sorts into group 3 as if it were ours, and the
format gate stays green while it does. Adding the root is part of adding the dependency.

## Headers

- `#pragma once`, never include guards.
- **One folder per subject area** — `base/diagnostics/` (Logger, Assert, Profiler),
  `base/math/`, `base/stringid/`, `base/time/`. `src/` and `tests/` mirror it. The module's
  own header (`base/Base.hpp`) stays at the root; a header shared by two utilities lives in
  the folder of the utility that owns it, not at the root. Decided 2026-08-03 (S3-T2), when
  `base` had two `Format.hpp` files. **Relaxed 2026-08-07 (S3-T5)** from *one folder per
  design note*: the profiler is instrumentation — the same thing a reader is looking for when
  they open `diagnostics/` — and a design note is a unit of documentation, not of layout. The
  test is **what a reader would look under**, not how many notes cover it.
- **Forward-declare in headers; include in the `.cpp`.** A public header pulling a heavy
  transitive include is a cost every consumer pays.
- **No third-party type in a module's public header** unless the ADR says so — that's the
  façade rule, and it's what keeps backends swappable (ADR-011 §1 is the worked example).
- A module's public header must not expose a *lower* module's private type (ADR-006 §3).

## Attributes — no `[[nodiscard]]`

**Don't write `[[nodiscard]]`.** Not on getters, not on queries, not anywhere. Removed from
`base` on 2026-07-26 (S2-T10).

Rationale: it is noise on every declaration for a warning that fires on code nobody writes —
calling a pure getter and dropping the result is a typo, not a class of bug worth taxing the
whole API surface for. It also reads as attribute-soup next to `constexpr` / `static` /
`inline`, which are load-bearing.

- Nothing enforces it either way — `modernize-use-nodiscard` is **not** in `.clang-tidy`, so no
  gate re-adds it.
- **Revisit only for a fallible API** where the discarded value *is* the error (`std::expected`,
  a `bool` "did it work"). That is the Error-handling row in *Open* below — decide it there, for
  that shape, not as a blanket habit. Today's cases are `addLogSink` and `removeLogSink`. Both
  bools were dropped unremarked until S4-T3; `addLogSink`'s is now checked at its one production
  call site, and `removeLogSink`'s is an explicit `(void)`. Nothing but review catches the next
  one, which is the cost this rule accepts.

## Initialization — `= value`, not `{value}`

> **A variable that starts at a value spells it with `=`.** `std::uint32_t m_alignment = 0;`,
> not `m_alignment{0}`. Decided 2026-08-08. Applies to members, locals, file-scope state.

Braces stay where they do something `=` cannot:

| Case | Write |
|---|---|
| scalar / enum / pointer with a starting value | `std::uint64_t m_frame = 0;` · `Level level = Level::Trace;` |
| **value-init, no value to state** | `std::array<char, 512> out{};` — bare `{}`; `= {}` is the same brace plus a token |
| aggregate with more than one field | `const Vec3 position{1.0f, 2.0f, 3.0f};` |
| a constructor call, not a value | `std::ifstream file{path};` — `= path` won't compile for an explicit ctor |
| narrowing you want the compiler to catch | braces; `=` truncates silently |
| a constructor's member-init list | out of scope — `=` isn't available there. `: m_id{id}` stays |

`std::atomic<T> x = value;` is fine — the converting constructor is not `explicit`, so the
retrofit of `g_minLevel{Level::Trace}` and friends is mechanical.

**Why:** `= 0` reads as *starts at zero*; `{0}` reads as *constructed from a one-element list*,
which is the wrong mental model for a scalar and the wrong shape next to the cases above that
genuinely are construction. Nothing enforces this — no check in our set covers init form.

`base`, `core` and `app` were swept to match on 2026-08-08.

## Loops — post-increment in the `for` step

```cpp
for (int i = 0; i < 10; i++)
```

`i++`, not `++i`. Decided 2026-08-03. The step expression's result is discarded, so the two are
identical for scalars and for any sane iterator; `i++` is what the loop idiom reads as. Nothing
enforces it — no clang-tidy check in our set covers increment form.

Applies to the `for` step only. In an expression whose value is used, write what the expression
actually needs.

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
| ~~`enum class` value casing~~ | **`PascalCase`** — ratified Jul 30 | First real enum: `Level` (S2-T2). |
| ~~Nested impl namespace~~ | **`TechEngine::internal`** — ratified Jul 30, respelled 2026-08-27 | Macros need a non-curated target to call; the "one flat namespace" rule applies to *curated* API, not plumbing. The Jul 30 spelling was `detail`; S4-T2 respelled it. |
| ~~File-scope state prefix~~ | **`g_camelCase`** — ratified Jul 30 | `g_` earns its place: `static` marks linkage but doesn't distinguish mutable state from constants at a glance. |
| **Ownership / smart pointers** | undecided | Fixes v1 F13. Proposed default: value + **handle** (index + generation); `unique_ptr` = single heap ownership; raw ptr/ref = non-owning, never deletes; `shared_ptr` only for genuine shared + unclear lifetime. Trigger: first real ownership decision. → [[Backlog]] |
| const-correctness | `misc-const-correctness` on in `.clang-tidy` | Mechanical for locals. Open: params/methods by hand? |
| Error handling | undecided | Exceptions vs `std::expected` vs error codes. Note `logDispatch` catches to keep diagnostics from killing logic. **Trigger fired twice and the row did not move**: `addLogSink` returns a bare bool, `Reader` carries a sticky `ReadStatus` (ADR-016). Carded at S4-T3 → [[Backlog]] § *etc*; it owns the `[[nodiscard]]` revisit above. |
| ~~**Target naming scheme**~~ | **`te_` = build-only, everything shippable is Pascal** — ratified 2026-08-10 | Shipping libs `TechEngine<Module>` · build-only/interface `te_*` · tests `TechEngine<Module>Tests` · leaf exes bare (`editor`, `runtime`) · alias lowercase `TechEngine::core`. The trigger fired at S3-T13: `te_test_support` (shared test helpers, INTERFACE) took its spelling from the principle with no new decision. Prior spellings that don't match and are not adopted: ADR-008 §6 `te_<module>_tests`, v1's `TechEngineEditor`. |

## Related

- [[B4 — Code Conventions]] — the vault-side note; now a pointer here + history.
- [[ADR-008 — v2 build & testing baseline]] §1 §2 §8 · [[ADR-006 — v2 core architecture & module layout]] §3
- `.clang-format` · `.clang-tidy` — the mechanical subset.
- `CLAUDE.md` → "Code conventions" — the short subset AI loads every session.
