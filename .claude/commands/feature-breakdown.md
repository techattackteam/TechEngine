---
description: Decompose a feature into Epic → Story → Task, session-sized
argument-hint: "<feature>"
---

Break down this feature for TechEngine — as scrum master, **co-create it with me as an
equal driver** (draft and challenge; I decide scope): $ARGUMENTS

Steps:
1. Start at the system's **design note** in `docs/04 Design Docs/` — its *Decided* rows are
   the reconciled view and the entry point; follow an ADR link only where a decision's
   **rationale** carries the argument (CLAUDE.md rule 2, gate note § *Where plans come
   from*). No note for a system in play → say so; that gap is itself a Design task. Then
   check `docs/05 Research/` — flag if research or an ADR is needed BEFORE any
   implementation, and don't hand-wave those prerequisites. (For a technique, the
   `engine-researcher` agent can evaluate it; for v1 prior art, `v1-reference-miner`.)
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
   (an ADR/design-note task precedes the story it unblocks). **Where the gate chose a
   heavy artifact, STOP the breakdown at that artifact's task** — the work below it stays a
   named, roughly-counted story (`~2–3 tasks · size after ADR-NNN`), never cards with
   done-conditions; cutting them belongs to the artifact task's own done-condition (gate
   note § *Don't size past an open decision*). Tag every task with
   **priority + weight** — `P1/P2/P3` and 🟢 Deep / 🟠 Moderate / 🟡 Light (day-fit) —
   per the gate note, so tasks can be picked by the day's energy. Format `· P1 · 🟢 Deep`.
5. Call out the riskiest/most-uncertain task and suggest a spike to de-risk it.
6. **If the gate chose a light design note, draft it in this same pass** from
   `docs/Templates/Design Doc Template.md` — light artifacts are *made in the session*, not
   scheduled (gate note, Artifact timing). Don't leave the decision in a backlog entry;
   that's the entry-budget tripwire in `Backlog.md`.
7. Offer to drop the stories/tasks into `docs/06 Sprints/Backlog.md` (do not add
   to the active sprint unless I say so). Backlog entries carry **want + `Trigger:` + a link
   to the artifact** — not the decision itself.
8. Planning only — no implementation.
