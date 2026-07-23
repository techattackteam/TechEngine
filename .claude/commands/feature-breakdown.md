---
description: Decompose a feature into Epic → Story → Task, session-sized
argument-hint: "<feature>"
---

Break down this feature for TechEngine — as scrum master, **co-create it with me as an
equal driver** (draft and challenge; I decide scope): $ARGUMENTS

Steps:
1. Check `docs/05 Research/` and relevant ADRs — flag if research or an ADR is needed
   BEFORE any implementation, and don't hand-wave those prerequisites. (For a technique,
   the `engine-researcher` agent can evaluate it; for v1 prior art, `v1-reference-miner`.)
2. Read the relevant code/system docs so the breakdown is grounded.
3. **Artifact gate** — run each item through
   `docs/06 Sprints/Planning Workflow — Artifact Gate.md`: decide by reversibility ×
   blast radius whether it needs an ADR, a design note, or **neither** (the default).
   Rarely both. Don't reflexively mint an ADR per feature; don't skip one a
   hard-to-reverse cross-module decision needs. Mark the chosen artifact **light**
   (short note, drafted at Sunday planning) or **heavy** (ADR / needs a spike → its own
   task, ordered before the impl task it unblocks).
4. Produce Epic → Stories → Tasks together. Each **task** must fit one 2–6h coding
   session and have an explicit done-condition. Note dependencies and ordering
   (an ADR/design-note task precedes the impl task it unblocks). Tag every task with
   **priority + weight** — `P1/P2/P3` and 🟢 Deep / 🟠 Moderate / 🟡 Light (day-fit) —
   per the gate note, so tasks can be picked by the day's energy. Format `· P1 · 🟢 Deep`.
5. Call out the riskiest/most-uncertain task and suggest a spike to de-risk it.
6. Offer to drop the stories/tasks into `docs/06 Sprints/Backlog.md` (do not add
   to the active sprint unless I say so).
7. Planning only — no implementation.
