---
description: Decompose a feature into Epic → Story → Task, session-sized
argument-hint: "<feature>"
---

Break down this feature for TechEngine: $ARGUMENTS

Steps:
1. Check `docs/05 Research/` and relevant ADRs — flag if research or an ADR is
   needed BEFORE any implementation, and don't hand-wave those prerequisites.
2. Read the relevant code/system docs so the breakdown is grounded.
3. Produce Epic → Stories → Tasks. Each **task** must fit one 2–6h coding
   session and have an explicit done-condition. Note dependencies and ordering.
4. Call out the riskiest/most-uncertain task and suggest a spike to de-risk it.
5. Offer to drop the stories/tasks into `docs/06 Sprints/Backlog.md` (do not add
   to the active sprint unless I say so).
6. Planning only — no implementation.
