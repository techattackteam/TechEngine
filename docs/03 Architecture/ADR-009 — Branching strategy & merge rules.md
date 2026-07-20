# ADR-009 — Branching strategy & merge rules

- **Status:** Proposed
- **Date:** 2026-07
- **Deciders:** Miguel (Lead Engineer), with AI as technical lead
- **Related:** [[ADR-008 — v2 build & testing baseline]] §9 (CI enforcement — this fills
  its branching-model gap) · [[ADR-004 — Fresh start (v2) with v1 as reference]] (the
  `v1-reference` tag) · CLAUDE.md "How to work here" rule 8 (branch off `master`, never
  commit directly)
- **Task:** defines the branch topology, protection, and merge mechanics that ADR-008 §9
  assumed but never specified.

## Context

ADR-008 §9 fixed the CI **trigger** (`pull_request` → `master`, `push` → `master`
backstop, required checks block the merge) but not the **branch topology**: how many
long-lived branches exist, how they're protected, and how work lands. CLAUDE.md rule 8
says "branch off `master`; never commit directly to `master`" — a habit, not yet a
structural rule. This ADR makes the model explicit and enforced.

Forces:
- **Solo dev, day-job cadence, sustainable pace** (CLAUDE.md). Process must reduce
  mistakes without a team to run ceremonies. Ceremony with no payoff is a tax.
- **`master` must stay always-compiling and shippable** — the whole point of the
  ADR-008 CI gate.
- **CI minutes are constrained.** Private repo ≈ 2,000 free min/mo, **Windows billed
  2×**, all sanitizers per-PR (ADR-008 §9). Every extra full-matrix run per unit of work
  is real cost.
- **Work is story-decomposed** (Epic → Story → Task; `/feature-breakdown`). Branches
  should map to that unit.
- **Long-term company goal** (CLAUDE.md) — but *speculative* team/release structure now
  is the same premature generality ADR-005/006 refuse ("no bespoke OS-abstraction until a
  2nd *ship* platform is real"). Adopt process weight when its trigger is real.

## Decision

**GitHub Flow: one long-lived branch (`master`) + short-lived story branches.** No
permanent `develop` integration branch.

### 1. Branches

- **`master`** — the trunk and the *only* long-lived branch. Always compiles, always
  green (enforced by required checks, §2), always shippable.
- **`<type>/<slug>`** — short-lived topic branches, one per sprint-board story/task.
  Branched off `master`, merged back via PR, **deleted after merge**. Flat namespace,
  kebab-case slug:
  - `story/<slug>` — a sprint story (default)
  - `fix/<slug>` · `chore/<slug>` · `docs/<slug>` — smaller non-story work
- **Stable = tags, not a branch.** When a build ships, tag it annotated
  `vMAJOR.MINOR.PATCH` off `master` (semver; consistent with the existing `v1-reference`
  tag, ADR-004). A `release/x.y` **branch** is cut off `master` *only if* a shipped line
  needs back-patching while trunk moves on — **deferred until that's real** (see §"What
  would move this").

### 2. Branch protection — `master`

Protection is what makes §1 real; without it the rules are honour-system.

- **No direct pushes** — every change lands via PR (CLAUDE.md rule 8, now structural).
- **Required status checks** = the ADR-008 §9 set (both legs × {Debug, Release} +
  clang-format + clang-tidy + `te_sdk_smoke` + sanitizer legs). A red check blocks merge.
- **Require branch up to date with `master`** before merge, so checks run against the
  post-merge state (no "green on stale base" merges).
- **Linear history** — no merge commits on `master` (pairs with squash, §3).
- **PR required; 0 approvals** — solo, so the PR is where **CI + self-review** happen,
  not a second pair of eyes. (Flip to "require 1 approval" the day a 2nd contributor
  exists — §"What would move this".)
- **Restrict force-push and deletion** of `master`.
- **Include administrators** — protection applies to Miguel too, or it's theatre. A
  documented **break-glass** (temporarily lift protection for a genuine emergency, then
  restore) is the escape hatch, used deliberately and rarely.

### 3. Merge mechanics

- **Squash-merge** a story branch into `master`: **one story → one atomic, revertable
  commit** on trunk. Keeps `master` history readable and bisectable; local WIP commits
  stay on the branch (until it's deleted), never pollute trunk.
- **Delete the branch** after merge.
- **Merge small and continuous** — land a story the moment it's green; don't batch a
  week of stories into one big merge (bigger merge = bigger conflict + risk surface).

### 4. Reconciliation with ADR-008 §9

The §9 `push` → `master` trigger still fires: a squash-merged PR **is** a push to
`master`. So "no direct pushes" (§2) and "push backstop" (§9) coexist — the only push to
`master` is the merge itself, and the backstop run guards it. The backstop runs the
**always-on legs** (build × {Debug, Release} + `clang-format`/`clang-tidy` +
`te_sdk_smoke`); the **sanitizer trio is `pull_request`-only** (ADR-008 §9 Option A "all
per-PR", amended 2026-07-21), since a squash of an up-to-date branch (§2) yields the same
tree the PR already sanitized. This clarifies §9's *trigger*, not its required-check set —
sanitizers stay **required PR checks** (§2), they just don't re-run on the merge push.

## Consequences

**Positive**
- `master` is always green and shippable **by construction** (required checks) — a
  structural guarantee, not a discipline that can slip.
- **~Half the CI minutes** of a 3-tier model: one full-matrix gate per story, not
  story → `develop` → `master`. Directly serves the §9 cost constraint.
- Trunk history is one commit per story — bisectable, trivially revertable, matches the
  Epic → Story → Task decomposition.
- Nothing to reshape as scope grows: protections/branches are *added at real triggers*,
  not carried speculatively.

**Negative / open**
- **Solo + 0 approvals = self-review.** Correctness leans on strict CI + the habit of
  running `/code-review` before merge, not a second reviewer. Accepted for a solo repo;
  revisited when a contributor arrives.
- **No integration branch to soak overlapping features** — if two long-running branches
  ever overlap, more rebasing onto trunk. Not a solo concern today.
- **Squash drops intra-story commit granularity** from trunk (kept on the branch until
  deletion). Accepted: trunk readability > WIP archaeology.
- **"Include administrators"** means no casual direct hotfix to `master`; the break-glass
  path must be followed. Mild friction, deliberately.
- **Free-tier protection limits:** some branch-protection controls need a paid plan on
  private repos. If so, making the repo **public** removes the limit *and* the §9 CI-cost
  trigger at once — folds into the existing "repo visibility" open item (ADR-008 §9 /
  Sprint Board "GitHub repo & CI enforcement" card).

## Alternatives considered

- **Git Flow — `master` + `develop` + feature branches** — rejected (now). `develop` is
  an integration buffer for **teams** with **scheduled releases**; solo, with a per-PR CI
  gate, `master` is *already* always-green, so `develop` duplicates that guarantee and
  **doubles CI cost** (two full-matrix runs per unit of work). Same premature-generality
  the project rejects elsewhere (ADR-005/006). Adopt when its trigger is real (below).
- **Trunk-based with direct commits to `master`** (no PR) — rejected: discards the CI
  gate and CLAUDE.md rule 8; `master` could land broken.
- **`develop` as the default working branch, `master` = release-only** — rejected now:
  no release automation or external consumers yet, so `master` and `develop` would hold
  the same commits offset in time. Revisit when a `push` → `master` = deploy/release
  pipeline exists.
- **Merge-commit (no squash)** — rejected: clutters trunk with WIP + merge bubbles,
  fights the linear-history rule. **Rebase-merge** (keep individual commits, linear) is
  viable but makes trunk noisier than one-commit-per-story; revisit only if per-commit
  trunk history becomes valuable.

## What would move this decision

So it's Accepted deliberately, not by default — evidence that should change an axis:

- **A second contributor** → require ≥1 PR approval; re-evaluate an integration branch.
- **A shipped / externally-consumed stable line** → formalize release tags, and cut
  `release/x.y` branches (or introduce `develop`) so trunk can move while stable is
  back-patched.
- **`push` → `master` automation** (deploy/release on merge) → reconsider keeping
  `master` pristine behind a `develop` working branch.
- **CI-minute pressure even at one-gate-per-story** → tune ADR-008 §9 (demote some
  sanitizers to nightly), *not* the branch model.
- **Story branches routinely outliving a sprint / heavy conflicts** → shorter stories
  (`/feature-breakdown`), not a `develop` buffer.

> Add to [[ADR Index]]. Once Accepted, treat as immutable — supersede with a new ADR.
