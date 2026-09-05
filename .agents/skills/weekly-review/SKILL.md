---
name: weekly-review
description: "TechEngine: Run the weekend weekly review and write it into the vault. Use when the user requests this workflow."
---

# weekly-review

Input: `[optional notes about the week]`. Use the invocation and current conversation to resolve it.
Repository paths below are relative to the engine root; vault wikilinks resolve under `docs/`.
Follow `AGENTS.md`, including existing user authorization, build ownership, and commit rules.

Act as my technical lead for the TechEngine weekly review (run on whichever weekend
day I work — Sat or Sun).

**GATE — check before doing anything else.** Read the active sprint note's end date. If it
is the Friday just gone, this weekend is a **sprint boundary**: stop and tell me to run
`$sprint-plan` instead, which absorbs this skill (decided 2026-07-26). Never derive that
date from the calendar month. Otherwise, continue.

Before writing, check whether a review already exists for either day of this weekend.
If it does, report that and do not create a duplicate.

Context to gather first:
- Read the current sprint in `docs/06 Sprints/` and the `docs/06 Sprints/Sprint Board.md`.
- Run `git log --since="7 days ago" --oneline` to see what actually shipped.
- Read the most recent review in `docs/07 Journal/`.

Notes from me about the week: the input from the user's request

Keep the review short enough to read at a glance. Use a short bullet per meaningful
outcome or blocker, with links instead of recaps. Write the sustainability check in
a few plain sentences. Preserve the drift result and any verification limits.

Then:
1. Draft this week's review using `docs/Templates/Weekly Review Template.md`:
   completed, in-progress, blockers, next-week objective, and an honest sustainability
   check — did the week respect the cadence (deep/moderate/light/off)? energy,
   job+engine+karting balance, anything to cut.
2. Save it as `docs/07 Journal/YYYY-MM-DD Weekly Review.md` (today's date).
3. **Stale-artifact check.** For features touched this week (from `git log` + the
   sprint board), spot-check their ADR / design note against what actually got built:
   did implementation diverge from the documented end-state? A stale artifact is
   poisoned ground truth — Codex will anchor to the wrong thing (AGENTS.md rule 2).
   **Also check hub drift:** for touched systems, the design note's *Decided*
   one-liners vs the ADRs they index — the hub pattern reads the note *first*, so a
   drifted one-liner silently overrides the ADR ([[Planning Workflow — Artifact Gate]]).
   List any drift under "Artifact drift"; offer to reconcile (update the living design
   note; apply [[ADR Index]] § *Amending an Accepted ADR* to decide whether an amendment
   or superseding ADR is owed if an Accepted decision changed). See
   [[Planning Workflow — Artifact Gate]]. If none, say so.
4. Update `docs/00 Dashboard/Dashboard.md`: current focus, blockers, health, and
   **roll the `Next ceremony` line**.
   **Advance the reconciliation stamp** — `**Reconciled against:** engine <sha> (date)`,
   `<sha>` = current `origin/master` — **only if step 3 actually ran**. If it was skipped,
   partial, or you deferred the reconciling, leave the stamp where it is and say why.
   A stamp advanced as a formality is worse than no stamp: it converts "unknown freshness"
   into a false "checked" that AGENTS.md rule 2 will then trust (ADR-012 §6). Advancing it
   is the *output* of the check, never a step in updating the Dashboard.
5. Keep it short and honest. Flag scope creep or burnout risk if you see it.
6. Do NOT start any implementation work — this is a planning ritual.
