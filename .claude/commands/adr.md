---
description: Draft an Architecture Decision Record for a load-bearing decision
argument-hint: "<the decision or question>"
---

Help me write an ADR for TechEngine.

Decision / question: $ARGUMENTS

Steps:
1. Read `docs/03 Architecture/ADR Index.md` for the next number + the **Accepted**
   set. Read any relevant code so the ADR is grounded in reality (cite `file:line`).
2. Check the decision against the Accepted ADRs (the `adr-consistency-checker` agent
   can do this): flag contradictions. Accepted ADRs are **immutable** — if this changes
   one, write a **superseding** ADR; a dated in-place *amendment* is allowed only for
   pure positioning that changes no decision.
3. Act as technical lead: lay out the real options with honest trade-offs. Cite
   evidence (`file:line`). Do not rubber-stamp my preferred option — argue the
   alternatives fairly and give a recommendation.
4. Draft the ADR from `docs/Templates/ADR Template.md` as
   `docs/03 Architecture/ADR-NNN — <title>.md` with Status: Proposed.
5. Add it to `docs/03 Architecture/ADR Index.md`.
6. Tell me what evidence would move the decision, so I can Accept it deliberately
   rather than by default.
