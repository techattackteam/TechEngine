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
   completed, in-progress, blockers, next-week objective, and an honest
   sustainability check (energy, job+engine+karting balance, anything to cut).
2. Save it as `docs/07 Journal/YYYY-MM-DD Weekly Review.md` (today's date).
3. Update `docs/00 Dashboard/Dashboard.md` (current focus, blockers, health).
4. Keep it short and honest. Flag scope creep or burnout risk if you see it.
5. Do NOT start any implementation work — this is a planning ritual.
