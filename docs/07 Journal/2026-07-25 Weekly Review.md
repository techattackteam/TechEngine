---
type: weekly-review
date: 2026-07-25
---

# Weekly Review — 2026-07-25

Week of Jul 20–25 · Sprint [[2026-07 Sprint 01 — Foundation Planning (v2)]] (final week).
**Sprint boundary weekend** — demo + retro + Sprint 02 planning go to `/sprint-plan`.

## ✅ Completed this week

| What | When | Evidence |
|---|---|---|
| **Build scaffold, whole thing** — `cmake/` helpers, presets, 5 lib skeletons, `apps/{runtime,editor}`, `te_base_tests`, `sdk/` + `te_sdk_smoke`, `.clang-format`/`.clang-tidy` | Mon 20 | `f19bb8dd` |
| **CI green on both legs** — Ninja Multi-Config swap (ccache is a compiler *launcher*, VS gen ignores it), ccache, `_deps` cache, sanitizers PR-only | Mon 20 – Tue 21 | `929f0e9a`→`85a3c6a8`, ADR-008 amendments 07-20/07-21 |
| **GitHub repo + `master` ruleset Active** — PR required, up-to-date branch, linear history, empty bypass list, 8 required checks | Fri 24 | [[Sprint Board]] · [[B3 — Build & Testing Notes]] |
| **Backlog populated** (+322 lines), Utilities design docs ([[Logger — Design]], [[Assert — Design]]) | Thu 23 | `50c0d820` |
| **[[ADR-010 — User authoring model (Systems & Scripts)]]** worked through — **decided: stays Proposed**, gated on the task-graph ADR | Thu–Fri | `acc4fcbf`, `004f87f9` |
| **[[Game Loop — Frame Flow]]** design note — decided `FrameContext` owns sim time, `Clock` is time source + diagnostic counter | Fri 24 | `004f87f9` |
| **Vocabulary amendments** — `World`→`Scene`, "scheduler" retired; dated header amendments on ADR-006/-007 (bodies untouched — immutability held) | Fri 24 | ADR-006/-007 headers |
| **Process hardening** — [[Planning Workflow — Artifact Gate]], weekend-anchored ceremonies, `04 Systems` → `04 Design Docs` | Thu–Sat | working tree (uncommitted) |

Sprint DoD line *"build skeleton compiles green on CI"* — **done**. That was the week's
whole plan, and it landed Monday.

## 🚧 In progress

- **C2 — first v2 vertical slice** not yet defined → the Sprint 02 headline goal, set at
  tomorrow's `/sprint-plan`. Last open sprint DoD item.
- **Uncommitted vault work** — 14 files on `feat/improving-vault` (dashboard, board,
  charter, artifact gate, templates, CLAUDE.md). Needs a PR through the new ruleset.
- **Board hygiene** — Story A still carries an unchecked *"read the deepened audit before
  B1"* item though B1 shipped; Review/Demo still holds Fri's grooming card, done Thu.
  Sweep at `/sprint-plan`.

## ⛔ Blockers

None hard. Three watch items:

- **CI minutes now cost real money** — ~2k/mo budget, Windows billed 2×, ≈22 billed min
  per merged change *today* and it only grows. Trigger response is ADR-008 §9
  (sanitizers → nightly). Watch Settings → Billing → Actions.
- **clang-tidy verified on the Linux leg only** — VS tidy segfaults locally, so Windows
  tidy is unproven.
- **glad2 deferred** until `platform` opens a GL context.

## 📐 Artifact drift

Four findings. Three are **live poison** — a session reading them acts on the wrong facts.

> **All four reconciled same day (2026-07-25).** Living-doc edits only — no Accepted
> *decision* changed, so no superseding ADR was needed; the docs simply described a repo
> that no longer exists. Resolution noted under each finding.

**1. `CLAUDE.md` still describes v1** *(highest impact — loaded every session, every model)*
- `CLAUDE.md:13-15` — "MSVC multi-config, **Visual Studio generator**", targets
  `engine/{app,client,core,server}`, `runtime/{editor,runtimes}`. Reality: **Ninja
  Multi-Config** via presets, `engine/{base,platform,core,client,app}` + `apps/` + `sdk/`.
- `CLAUDE.md:16-17` — "mid render-graph migration" — that's the v1 prototype.
- `CLAUDE.md:23-32` — `cmake -S . -B cmake-build-debug` — wrong; it's `--preset windows`.
- `CLAUDE.md:36` — "no test suite yet" — `te_base_tests` + `te_sdk_smoke` run 3/3 under CTest.
- Known-deferred as *"CLAUDE.md refresh → B4"*, but B4 now exists and the file has gone
  from **incomplete to actively wrong**. Rule 2 says ground answers in the artifact — this
  artifact grounds them in the dead prototype.
- ✅ **Fixed** — layout/state bullets rewritten to the real v2 tree, build block is now
  preset-based (`cmake --preset windows`), Testing section describes the live Catch2+CTest
  setup. Added a CI-minute caution (don't push to watch CI).

**2. `04 Systems` → `04 Design Docs` rename left 4 dangling pointers**
- `CLAUDE.md:93` (rule 2's "start at the design note in `docs/04 Systems/`" — the path
  doesn't exist), `.claude/commands/arch-review.md:9` + `:21`, `docs/README.md:31`,
  and `docs/04 Design Docs/README.md:1` still titled *"04 Systems — v2"*.
- ✅ **Fixed** — all four repointed; a repo-wide grep for `04 Systems` now returns only
  this journal entry.

**3. [[B4 — Code Conventions]] contradicts the CI that is gating merges**
- B4's formatting header: *"`.clang-format` would be ignored today"* and CI format-gating
  is *"a separate decision"* not yet taken.
- Reality: `.clang-format` is in the repo root and `ci.yml:32-40` runs
  `clang-format --dry-run --Werror` as a **required check on `master`**. A session trusting
  B4 would think formatting is ungated and get bounced by the ruleset.
- **Second contradiction found while fixing:** B4 claimed a **380-col** right margin
  ("effectively no hard wrap"); `.clang-format` sets **`ColumnLimit: 100`** — and it's the
  one that gates merges.
- ✅ **Fixed** — `.clang-format` named as the formatting source of truth (prose now
  *describes* it), column limit corrected to 100, plus the `IncludeBlocks: Preserve` /
  `AllowShortFunctionsOnASingleLine: None` rules that were undocumented. Left one **open
  local item**: if CLion still has `EnableClangFormatSupport=false`, turn it on so the IDE
  matches CI — can't verify an IDE setting from here.

**4. Two scaffold checklists, both stale, neither is the single home**
- ADR-008 §scaffold: 4 boxes unchecked (`te_base_tests`, `sdk/` + smoke,
  `.clang-format`/`.clang-tidy`, `ci.yml`) — all four are **built and green**.
- [[B3 — Build & Testing Notes]] carries a duplicate 7-item checklist, **100% unchecked**.
- Violates one-home-per-fact. Checklist state is *status*, not a decision — it belongs on
  the board, not inside an Accepted ADR.
- ✅ **Fixed** — ADR-008's list ticked and **closed 2026-07-24** as frozen history, with a
  pointer that live status lives on [[Sprint Board]] / [[B3 — Build & Testing Notes]];
  B3's duplicate replaced by a link. One home.

**Hub drift (design-note *Decided* one-liners vs the ADRs they index): none found.**
[[Game Loop — Frame Flow]] and [[Task Graph — Execution Flow]] cite ADR §s per row, use
post-amendment vocabulary (`Scene`, `Schedule`/task graph/executor), and mark ADR-010 rows
*(Proposed)*. ADR-007's body still reads `World&` — that's the documented immutability
pattern (dated header amendment, body frozen), not drift.

## 🎯 Objective for next week

**Sprint boundary — the real objective is set at `/sprint-plan` (Sprint 02 headline goal).**
Provisional: **define and start the first v2 vertical slice (C2)** — first week of writing
engine code rather than deciding it.

Two small things to clear first, both cheap:
1. Land the uncommitted vault branch through the new ruleset (first real PR through it).
2. Reconcile findings 1–3 above **before** any coding session reads them.

## 🔋 Sustainability check

- **Energy:** high, and that is the thing to watch. The week's entire plan finished on
  **Monday** — scaffold pt1 *and* pt2 in one deep day instead of Mon+Thu.
- **Cadence:** mostly respected. Wed 22 fully off ✅. Tue 21 light ✅. Two sessions ran past
  midnight (Sun→Mon `00:25`, Mon→Tue `00:27`) — engine work sits in a 19:00–00:30 window
  after the day job, so a "deep day" is really a 5-hour night. Watch the midnight edge.
- **The honest problem:** finishing early was converted into **more work, not rest**. Thu's
  deep slot was free (pt2 already done) and got refilled with unplanned work; Fri was
  scheduled 🟠 *moderate* and delivered **four** substantial items (CI enforcement, ADR-010
  session, game loop design note, vocabulary amendments). That is a moderate day doing a
  deep day's volume. Nothing here is off-mission — but the week never had slack, because
  slack got spent the moment it appeared.
- **Job + engine + karting:** balance held; no engine work Sat–Sun daytime, Wed protected.
- **To cut:** nothing from scope — cut the *refill reflex*. Next time a plan finishes early,
  bank the day. Concretely: if Sprint 02's first week finishes ahead, **Thu stays empty.**
- **Scope creep:** mild and mostly justified (design work, deferred items). One genuine
  detour: the vault restructure (`04 Systems` → `04 Design Docs`, artifact gate, template
  edits) was unplanned and is what produced finding #2. Restructures should be a planned
  card, not a Friday-night side quest.

→ Sprint 01 demo + retro + Sprint 02 planning: `/sprint-plan` (this weekend).
