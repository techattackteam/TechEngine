---
description: Review code against CONVENTIONS.md's judgment rows and the ADR structural invariants
argument-hint: "[file, folder, or module — omit and I'll take the working tree diff]"
---

Act as my reviewer for TechEngine house rules. Target: $ARGUMENTS

Read [`CONVENTIONS.md`](CONVENTIONS.md) first. It is the source of truth and this file does not
restate it — the list below is the **checklist over it**, not a copy, so a rule that changed
there wins here.

## Scope — this is the *judgment* layer only

`CONVENTIONS.md` → *Mechanical vs judgment* splits enforcement three ways, and this command is
the third row. **Do not check what already has a red gate**, because a second opinion on a
solved problem is how a review becomes noise:

| Already enforced | By | Don't check |
|---|---|---|
| Formatting, include **order** and grouping | `.clang-format` + the CI format gate | indent, braces, column limit, `IncludeCategories` |
| bugprone / performance / const-correctness of locals | `.clang-tidy`, Linux leg | its whole check set |
| A private type leaking into the SDK | `te_sdk_smoke` (ADR-008 §7) | the leak itself |
| Reaching into private plumbing | `ci.yml` → *No reaching into private plumbing* | the greps it already runs |

**Not a bug hunt either.** Correctness, logic errors and edge cases are `/code-review`. If you
see a real defect while reading, say it once at the end under *Off-rubric*, then get back to
the rubric.

## The checklist — rules with no gate but this one

**Naming** (`CONVENTIONS.md` → *Naming*, *Names are spelled out*)
- `m_` on members, `g_` + `static` on file-scope mutable state, `SCREAMING_SNAKE_CASE`
  constants, `PascalCase` on types and `enum class` values, `camelCase` on functions and locals.
- **`TE_` marks a macro and only a macro.** A `TE_`-prefixed constant is a finding; so is an
  unprefixed macro.
- **Full words**: `deltaTime` not `dt`, `accumulator` not `acc`, `previous` not `prev`.
- No `te_` or snake_case on a C++ identifier. `te_` is build-only (*Open* → *Target naming*).

**Linkage** (*Internal linkage*)
- A `.cpp`-local free function or variable is `static`, not wrapped in `namespace {}`.
- An anonymous namespace is a finding **unless** it wraps exactly one `.cpp`-local **type**
  with a real same-name-different-type ODR risk, and a more specific type name would not have
  removed the risk.

**Comments** (*Comments*, *Comments never cite the vault*) — usually the richest row
- The bar is *"would a competent reader be wrong without this?"*. Default is **no comment**.
- Only three things earn one: a gotcha that will bite, a `TODO(S<n>-T<n>)`, a `TODO(D<n>)`.
- Delete on sight: divider banners, `} // namespace`, file/class/function preambles, per-include
  narration, anything restating the code, rationale copied from the vault.
- **Any vault citation in `.hpp`/`.cpp`/tests is a finding** — `ADR-011 §2`, `[[Log — Design]]`,
  a bare `§ref`. A fresh clone has no `docs/`. `TODO(D1)` and `TODO(S3-T8)` are **not** citations
  and stay.

**Attributes / init / loops** — the three the file says nothing enforces
- **No `[[nodiscard]]`**, anywhere.
- `= value`, not `{value}`. Braces only for bare `{}` value-init, multi-field aggregates, a real
  constructor call, deliberate narrowing checks, and member-init lists.
- `i++` in a `for` step, not `++i`.

**Headers** (*Headers*, *Privacy*)
- `#pragma once`, never guards. Forward-declare in the header, include in the `.cpp`.
- **No third-party type in a module's public header** unless an ADR says so. That is the façade
  rule, and ADR-011 §1 is the worked example.
- One folder per subject area, `src/` and `tests/` mirroring it. The test is *what would a
  reader look under*, not how many design notes cover it.
- A plumbing macro in a public header is named `TE_<AREA>_PRIVATE_<VERB>`, says what to call
  instead **at the definition**, and has a `ci.yml` check line. A private symbol with no red
  check is a finding.

**CMake** (*CMake*)
- Link the `TechEngine::<module>` alias, never the real target name.
- `PUBLIC` iff the dep appears in the target's public headers, else `PRIVATE`.
- Explicit source lists, never `GLOB`. Module lists stay declarative — an `if()` or a loop
  belongs in `cmake/*.cmake`.

**Structural invariants** (ADR-006 §1 §3)
- The DAG is acyclic and downward: `base` ← `platform` ← `core` ← `client`, `app` → `core`,
  exes are leaves. An include or a link that points *up* is the most expensive finding on this
  list, so check the direction before anything else.
- A module's public header must not expose a **lower** module's private type.

## Open rows are a different finding

`CONVENTIONS.md` → *Open* lists rules that are deliberately undecided: ownership and smart
pointers, error handling, const-correctness of params and methods.

Code that lands on one of these is **not a violation**. It is the **trigger firing**, and the
finding is that `CONVENTIONS.md` now owes a decided row. Report it separately from violations,
name which row, and say what the code actually chose. That is how *Target naming scheme* got
ratified at S3-T13.

## Output

One line per finding: `file:line` → the rule → what to change. Grouped under **Violations**,
**Open row fired**, **Off-rubric**. No paragraphs, no restating the code you are flagging.

**A clean file gets one line saying so.** Do not manufacture findings to look useful. But if a
whole pass produces nothing at all, say that plainly too, because a rubric that never fires is
one I should trim rather than keep running.

## Guardrails — these are the false positives to avoid

- **Rule 0 wins: match the surrounding file.** If a file is a consistent outlier, the finding is
  *"this file diverges"*, and my call is whether to fix the file or the convention. Never both.
- **The ADRs' spellings are illustrative, not decisions.** `kFixedDt`, `dt`, `fixedDt`,
  `te_<module>`, ADR-008 §6's `te_<module>_tests` — all refined by `CONVENTIONS.md`. Flagging
  code for matching the file instead of the ADR is backwards, and Accepted ADRs are never edited
  to match.
- **Not abbreviations:** `NetId`, `uuidOf`, `httpClient`, loop `i`/`j`, matrix `m`, texcoords
  `u`/`v`, `alpha`, `tick`. Leave them.
- **Report, don't edit.** No fixes unless I ask, and never a reformat.
