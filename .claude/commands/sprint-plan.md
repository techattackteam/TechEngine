---
description: Plan the next monthly sprint (last-Sunday-of-month ritual)
argument-hint: "[sprint theme or focus, if you have one in mind]"
---

Act as my technical lead for TechEngine monthly sprint planning.

My input on focus (may be empty): $ARGUMENTS

Steps:
1. Read `docs/02 Roadmap/Roadmap.md` and the current quarter note, the active
   sprint, `docs/06 Sprints/Backlog.md`, and any relevant ADRs in
   `docs/03 Architecture/`.
2. If the current sprint is ending, first help me capture a short retrospective
   using `docs/Templates/Retrospective Template.md` into `docs/07 Journal/`.
3. Propose ONE headline goal for the next sprint. Justify it against the roadmap
   and the latest ADR decisions. Push back if I'm over-scoping.
4. Break it down Epic → Story → Task. Every task must fit one 2–6h coding
   session with a clear done-condition. Size the sprint to my real capacity
   (deep days Mon/Thu/Fri/Sun; Tue/Wed light).
5. Create the sprint note from `docs/Templates/Sprint Template.md` and update
   `docs/06 Sprints/Sprint Board.md`, the roadmap, and the dashboard.
6. This is planning only — no implementation.
