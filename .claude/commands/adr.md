---
description: Draft an Architecture Decision Record for a load-bearing decision
argument-hint: "<the decision or question>"
---

Help me write an ADR for TechEngine.

Decision / question: $ARGUMENTS

**Budget:** ADR **≤ 120 lines**, no `§` over ~30. Rationale is the one place real prose is
earned — spend it there and nowhere else. Options as a table, consequences as bullets.

Steps:
1. Read `docs/03 Architecture/ADR Index.md` for the next number + the **Accepted**
   set. Read any relevant code so the ADR is grounded in reality (cite `file:line`).
2. Check the decision against the Accepted ADRs (the `adr-consistency-checker` agent
   can do this): flag contradictions. If this changes an Accepted ADR, apply
   [[ADR Index]] § *Amending an Accepted ADR*. A change that fits one dated header entry,
   old value quoted and trigger named, is an in-place **amendment**, `decision` kind
   included. A change needing its own Context and Alternatives is a **superseding** ADR.
   The headline decision in a title is never amendable.
3. Act as technical lead: lay out the real options with honest trade-offs. Cite
   evidence (`file:line`). Do not rubber-stamp my preferred option — argue the
   alternatives fairly and give a recommendation.
4. Draft the ADR from `docs/Templates/ADR Template.md` as
   `docs/03 Architecture/ADR-NNN — <title>.md` with Status: Proposed.
   **Scope: decision + rationale + alternatives + reversal triggers, ~150 lines.**
   The *how* — surface tables, diagrams, workflows, instrumentation policy — belongs
   in the system's design note, not here. A mechanism table or diagram sprouting
   inside a § is the peel signal ([[Planning Workflow — Artifact Gate]] § *ADR or
   design note*). Update the note's *Decided* rows (one-liners + §refs) in the same
   session; ADR-013 vs [[Profiler — Design]] is the counter-example — the ADR
   ballooned and the note duplicated its diagrams.
5. Add it to `docs/03 Architecture/ADR Index.md`.
6. Tell me what evidence would move the decision, so I can Accept it deliberately
   rather than by default.
