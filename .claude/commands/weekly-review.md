---
description: Run the Sunday weekly review and write it into the vault
argument-hint: "[optional notes about the week]"
---

Act as my technical lead for the TechEngine Sunday weekly review.

Context to gather first:
- Read the current sprint in `docs/06 Sprints/` and the `docs/06 Sprints/Sprint Board.md`.
- Run `git log --since="7 days ago" --oneline` to see what actually shipped.
- Read the most recent review in `docs/07 Journal/`.

Notes from me about the week: $ARGUMENTS

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
5. **If today is the month's last Sunday**, this is also the sprint boundary — hand off
   to `/sprint-plan` (demo + retro + plan the next sprint).
6. Keep it short and honest. Flag scope creep or burnout risk if you see it.
7. Do NOT start any implementation work — this is a planning ritual.
