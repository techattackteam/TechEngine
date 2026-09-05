---
name: adr
description: "TechEngine: Draft an Architecture Decision Record for a load-bearing decision. Use when the user requests this workflow."
---

# adr

Input: `<the decision or question>`. Use the invocation and current conversation to resolve it.
Repository paths below are relative to the engine root; vault wikilinks resolve under `docs/`.
Follow `AGENTS.md`, including existing user authorization, build ownership, and commit rules.

Help me write an ADR for TechEngine.

Decision / question: the input from the user's request

Keep the ADR focused on the decision and its rationale, usually within 120 lines.
Use plain sentences and short sections. Compare options in a table only when that
helps; length guidance is a ceiling, not a target or a reason to compress the text.

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
   **Scope: decision + rationale + alternatives + reversal triggers, within the budget above.**
   The *how* — surface tables, diagrams, workflows, instrumentation policy — belongs
   in the system's design note, not here. A mechanism table or diagram sprouting
   inside a § is the peel signal ([[Planning Workflow — Artifact Gate]] § *ADR or
   design note*). Update the note's *Decided* rows (one-liners + §refs) in the same
   session; ADR-013 vs [[Profiler — Design]] is the counter-example — the ADR
   ballooned and the note duplicated its diagrams.
5. Add it to `docs/03 Architecture/ADR Index.md`.
6. Tell me what evidence would move the decision, so I can Accept it deliberately
   rather than by default.
