---
type: retrospective
sprint: Sprint 01
date: 2026-07-25
---

# Retrospective — Sprint 01 (Foundation Planning, v2)

Jul 19–26 · short special sprint ([[2026-07 Sprint 01 — Foundation Planning (v2)]]).
Week-level detail in [[2026-07-25 Weekly Review]]; this is the sprint-level view.

## 🎬 Demo — what Sprint 01 actually shows

- `cmake --preset windows` → **builds clean**, `ctest` **3/3** (base ×2 + SDK smoke).
- **CI green on both legs** (win-msvc + linux-clang, Debug + Release), format + tidy gates,
  sanitizer trio per-PR.
- `master` **ruleset Active** — PR required, linear history, empty bypass list, 8 required checks.
- The vault: 7 Accepted ADRs, 4 design notes, populated backlog, working ceremony loop.

**Sprint goal — met.** Every DoD line closed except *"Sprint 02 has a concrete headline
goal"*, which this session closes.

## 🟢 What went well

- **Audit → design → ground actually gated each other.** No v2 code was written before the
  foundation ADRs; the scaffold then landed in **one day** because the decisions were done.
- **Scaffold beat its estimate 2:1** — pt1 + pt2 planned Mon+Thu, delivered Mon.
- **ADR immutability held under real pressure.** The `World`→`Scene` + "scheduler" rename hit
  two *Accepted* ADRs and was handled by dated header amendments with frozen bodies — the
  rule survived its first genuine test instead of being quietly bent.
- **Saying "no" worked.** ADR-010 was deliberately left **Proposed** (gated on the task-graph
  ADR) rather than forced to Accepted for tidiness. Profiler and FrameAllocator were
  pressure-tested out of Sprint 02 for having no consumer. That's the artifact gate doing
  its job.
- **The hub pattern is holding** — this weekend's drift check found **zero** hub drift across
  [[Game Loop — Frame Flow]] and [[Task Graph — Execution Flow]].

## 🔴 What wasted time

- **CI toolchain thrash (Mon 20 evening, ~5 commits + 2 assisted PRs).** sccache → ccache,
  Visual Studio generator → Ninja Multi-Config, Wayland deps, `windows-2022` pinning, and a
  literal *"Test commit to check if cache is working"*. Root cause: **ccache is a compiler
  *launcher*, which the VS generator ignores** — a fact discoverable by reading, found by
  pushing. Cost: an evening of CI round-trips, on the meter now that minutes are billed.
- **The Friday-night vault restructure.** `04 Systems` → `04 Design Docs` was unplanned, and
  it silently broke 4 pointers — including CLAUDE.md rule 2's own path. Found today, fixed
  today, but it was self-inflicted.
- **`CLAUDE.md` described v1 for the entire sprint.** Wrong generator, wrong module layout,
  wrong build commands, "no test suite yet" — while sitting in every single session's context.

## 🔧 Process improvements (do next sprint)

1. **Finish early → bank the day.** The week's plan finished Monday and the freed time was
   refilled, not rested (Fri ran four substantial items on a 🟠 moderate slot). **If a sprint
   week finishes ahead, the next deep day stays empty.** This is the one process change that
   matters.
2. **Restructures are planned cards, not side quests.** Any rename/move of a vault folder or
   module gets a task with a "grep for dangling refs" done-condition.
3. **Read the tool's model before pushing to CI.** CI is now metered (~22 billed min per
   merged change). Local repro first; no "push and see" debugging.
4. **Keep the weekly drift check** — it caught all four findings, including the one nobody
   would have noticed until it misled a coding session. It earned its slot.

## 🤖 How AI helped (and where it didn't)

**Helped:** drafting and *challenging* the foundation ADRs; enforcing the immutability
pattern during the vocabulary rename; the artifact gate and backlog population; the drift
audit that found all four stale artifacts.

**Didn't:** Claude read a **stale `CLAUDE.md` every session for the whole sprint and never
flagged it** — it followed the wrong spec (VS generator, v1 module layout) without noticing
the contradiction with the repo in front of it. The lesson isn't "write better docs", it's
that **the drift check has to be a scheduled ritual**, because the failure mode is silent
and self-reinforcing: the wrong ground truth reads as confident and correct.

Also: the CI thrash was trial-and-error where a moment of reading the ccache docs would
have gone straight to Ninja.

## Carry-over to next sprint

- **C2 — first v2 vertical slice** → deliberately re-sequenced to **Sprint 03** (needs the
  base foundation Sprint 02 lays).
- **B4 / root `CONVENTIONS.md`** — trigger ("first module code") fires in Sprint 02.
- **Uncommitted vault branch** `feat/improving-vault` — needs a PR through the new ruleset.
- **Skill `te-module`** — its trigger ("scaffold exists") has fired.
