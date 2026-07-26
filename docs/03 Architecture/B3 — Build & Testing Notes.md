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
- **★ SDK smoke CI target** → compile a sample script against `TechEngine::sdk` **alone**; a
  private type leaking into the SDK fails CI (the F11 acid test, automated).
- **Test-bed = sample project(s)** loaded by `runtime`/`editor` (content), not a
  separate exe; `app` exposes a headless/sim-only mode (exercised by `tests`).

## CI operational notes (observed on the first real runs, 2026-07-24)

**Why the `push` → `master` backstop stays — it warms the shared ccache.** The obvious
argument for deleting it is redundancy: with "require branches up to date" on
([[ADR-009 — Branching strategy & merge rules]] §2), a squash-merge produces the tree the PR
already tested, so the backstop re-tests it. The reason to keep it anyway is **GitHub Actions
cache scoping**: a cache saved on a branch is visible to that branch and its children, but a
cache saved on the **default branch is visible to every branch**. PR runs restore only from
their own branch or `master`, so the backstop run is what keeps the cache every future PR
pulls from fresh. Delete it and each PR starts from whatever stale `master` cache was last
written — directly against ADR-008 §9's "CI time rides entirely on caching working".
(Backstop guarantees are ADR-009 §4; this is the *practical* reason, which that ADR doesn't
state.)

**clang-tidy is verified on the Linux leg only.** The MSVC-side tidy integration segfaults
locally, so the Linux/Clang leg is the only place the gate has actually been exercised. Treat a
clean local Windows build as *no evidence* about tidy — the Linux leg is the authority.

**Phantom `CI / matrix.name` check on push runs — cosmetic, don't chase it.** The sanitizer
job is `pull_request`-only (`ci.yml` `if: github.event_name == 'pull_request'`, ADR-008 §9
amended + ADR-009 §4). When a job is skipped by a **job-level `if:`** the matrix never
expands, so `name: ${{ matrix.name }}` has no matrix context and GitHub renders the literal
expression — **one** skipped check called `matrix.name` instead of three named ones. On a PR
the job runs, the matrix expands, and the real contexts (`win ASan`, `linux UBSan`,
`linux TSan`) report normally — which is where required checks are evaluated, so branch
rulesets are unaffected. Clean fix if it ever matters: split sanitizers into their own
`pull_request`-only workflow; costs ~40 lines of duplicated setup for a cosmetic win.

## Scaffold checklist

Moved — this note fed the ADR, and the ADR is where the checklist landed:
[[ADR-008 — v2 build & testing baseline]] → *Scaffold checklist*, **closed 2026-07-24**
(all green on CI, both legs). Kept in one place so it can't rot in two.
