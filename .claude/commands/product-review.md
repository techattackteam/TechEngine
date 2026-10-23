---
description: Review the product direction with me - Vision, Reference Game, Capability Map - against what shipped and what we learned
argument-hint: "[focus, or what the channel's viewers have been asking for]"
---

Act as my product owner's analyst for a TechEngine product review. My input (may be empty):
$ARGUMENTS

**When this runs:** at the planning session that opens a quarter, at the boundary between
Phase 1 and Phase 2, when a parked product decision's trigger comes due, or when I ask. It is
not a weekly ceremony.

**Size:** the review itself stays in chat. Only the changes I agree to are written, in place,
in the notes they belong to.

## Gather

- `docs/01 Product/`: Vision, Reference Game, Capability Map, Channel, Principles, and every
  brief in `Capabilities/`.
- `docs/02 Roadmap/Roadmap.md` and the current quarter note.
- The sprint notes and retrospectives since the last review (`docs/06 Sprints/`,
  `docs/07 Journal/`), for what shipped and what hurt.
- The *Decided* tables in the product notes, for decisions parked with a trigger.

## Steps

1. **What shipped against the map.** Which Roadmap goals closed, which showcases came out, and
   where a goal changed the system's scope on the way.
2. **What we learned.** Ask me what the channel taught us: views, comments, the requests
   viewers keep making. Add what the retros say, and the 🤖 Claude decisions I reversed.
3. **Parked decisions that are due.** Every *Decided* row or open question whose trigger has
   fired: for example the second gameplay language "after the demo", world-space precision "at
   the endless-world goal", vegetation "when G1 is planned", the briefs' re-check "when Phase 2
   starts".
4. **Propose changes**, as one table: note · change · why. One decision per row, my
   recommendation first. I accept or reject each row.
5. **Apply what I accepted**: edit the notes in place, reorder the Roadmap's goals if the map
   changed, and write the new quarter's theme when a quarter opens (a direction, not a feature
   list). Record each accepted change as a dated *Decided* row where the note has one.

## Report

What changed and in which note, and any decision I deferred, with its new trigger.

## Guardrails

- **Never rewrite the Vision or the Reference Game without my yes on that row.**
- **No design, no ADRs, no cards.** A change that needs one becomes a card for `/sprint-prep`.
- Attended only. Vault only. Do not commit unless I ask.
