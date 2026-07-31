---
description: Run the weekend weekly review and write it into the vault
argument-hint: "[optional notes about the week]"
---

Act as my technical lead for the TechEngine weekly review (run on whichever weekend
day I work — Sat or Sun).

**GATE — check before doing anything else.** Read the active sprint note's end date. If it
is the Friday just gone, this weekend is a **sprint boundary**: stop and tell me to run
`/sprint-plan` instead, which absorbs this command (decided 2026-07-26). Never derive that
date from the calendar month. Otherwise, continue.

Context to gather first:
- Read the current sprint in `docs/06 Sprints/` and the `docs/06 Sprints/Sprint Board.md`.
- Run `git log --since="7 days ago" --oneline` to see what actually shipped.
- Read the most recent review in `docs/07 Journal/`.

Notes from me about the week: $ARGUMENTS

**Budget:** the review note is **≤ 60 lines**, no section over ~15. Bullets, not prose —
one line per shipped item, one line per blocker. The sustainability check is 3–4 lines of
honest assessment, not an essay.

Then:
1. Draft this week's review using `docs/Templates/Weekly Review Template.md`:
   completed, in-progress, blockers, next-week objective, and an honest sustainability
   check — did the week respect the cadence (deep/moderate/light/off)? energy,
   job+engine+karting balance, anything to cut.
2. Save it as `docs/07 Journal/YYYY-MM-DD Weekly Review.md` (today's date).
3. **Stale-artifact check.** For features touched this week (from `git log` + the
   sprint board), spot-check their ADR / design note against what actually got built:
   did implementation diverge from the documented end-state? A stale artifact is
   poisoned ground truth — Claude will anchor to the wrong thing (CLAUDE.md rule 2).
   **Also check hub drift:** for touched systems, the design note's *Decided*
   one-liners vs the ADRs they index — the hub pattern reads the note *first*, so a
   drifted one-liner silently overrides the ADR ([[Planning Workflow — Artifact Gate]]).
   List any drift under "Artifact drift"; offer to reconcile (update the living design
   note; a *superseding* ADR if an Accepted decision changed). See
   [[Planning Workflow — Artifact Gate]]. If none, say so.
4. Update `docs/00 Dashboard/Dashboard.md`: current focus, blockers, health, and
   **roll the `Next ceremony` line**.
   **Advance the reconciliation stamp** — `**Reconciled against:** engine <sha> (date)`,
   `<sha>` = current `origin/master` — **only if step 3 actually ran**. If it was skipped,
   partial, or you deferred the reconciling, leave the stamp where it is and say why.
   A stamp advanced as a formality is worse than no stamp: it converts "unknown freshness"
   into a false "checked" that CLAUDE.md rule 2 will then trust (ADR-012 §6). Advancing it
   is the *output* of the check, never a step in updating the Dashboard.
5. If I already ran the review on the other day of this weekend, say so instead of writing
   a second entry. (The sprint-boundary case is handled by the GATE above — on those
   weekends this command doesn't run at all.)
6. Keep it short and honest. Flag scope creep or burnout risk if you see it.
7. Do NOT start any implementation work — this is a planning ritual.
