---
description: Weigh a new idea or feature request against the product before anyone designs or schedules it
argument-hint: "<the idea, request or feature>"
---

Act as my product owner's analyst. A new idea has turned up: $ARGUMENTS

It might be mine, a viewer's request, or something a card or a review uncovered. **Nobody designs
or schedules it before this runs.** That is the point of the command: an idea built straight
away skips the question of whether the product needs it.

## Steps

1. **Restate it** as one user-visible capability, in one sentence: who gets what.
2. **Is it already on the map?** Search `docs/01 Product/Capability Map.md` and the briefs in
   `docs/01 Product/Capabilities/`. If it is there, point to the row and its goal, and stop.
3. **Weigh it against the product** ([[Principles]]):
   - **P1:** does TechAttack need it, and in which goal or stage?
   - **P2:** is it an engine capability, or a game rule that belongs in the TechAttack project?
   - **P5:** what does the general form cost compared with the narrow one?
4. **Size its impact in goals, not hours**: which Roadmap goal it would join, and what it would
   push out of that month.
5. **Recommend one outcome**, and let me pick:

   | Outcome | What gets written |
   |---|---|
   | **Add to goal X** | A row in the map's section, and the goal's row on the Roadmap |
   | **Later** | The map section's *Later* row |
   | **Won't, for now** | The map's *Not in the demo* row, with the reason, so it reads as a decision |
   | **Game, not engine** | A note in the [[Reference Game]], for Phase 2 |
   | **Not product at all** | A plain engine improvement: a [[Backlog]] line with a trigger |

6. **Write only the outcome I chose**, in place, dated.

## Guardrails

- **Never put it into the active sprint.** A mid-sprint addition displaces something, and that
  is `/sprint-prep`'s or my call, never this command's.
- **No design, no ADR, no cards.** A Backlog line holds a want and a trigger, never a decision.
- Attended only. Vault only. Do not commit unless I ask.
