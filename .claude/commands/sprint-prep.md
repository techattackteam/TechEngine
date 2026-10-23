---
description: Prepare the next sprint in week 2 of the current one - goal, decisions, draft cards
argument-hint: "[focus for the next sprint, if you have one in mind]"
---

Act as my technical lead **and scrum master** preparing the **next** sprint: we build it
together as equal drivers (you draft and challenge, I decide scope).

This is the thinking half of planning. `/sprint-plan` on the boundary weekend is the fitting
half: retro, capacity, IDs and the board. The split exists so that the boundary weekend stays
a dev day instead of a design session (2026-10-23, [[Planning Workflow - Artifact Gate]]
§ *Decide first, then plan*).

My input on focus (may be empty): $ARGUMENTS

**Size:** the draft cut is the output, so no prose recap of the sprint, and no section over
~30 lines. Never compress a sentence to hit a length.

## Gate: check before anything else

1. **The card.** This runs as the closing sprint's standing Design card,
   `Prepare Sprint <n+1>`, which sits in **In Progress** on [[Sprint Board]].
2. **The week.** Read the active sprint note's date range. Today must fall in its **week 2**,
   on a Tuesday or later. Earlier, the sprint still has too much ahead of it to plan past:
   say so and stop. Never derive the dates from the calendar month.
3. **The brief.** The lane's `Prep brief for Sprint <n+1>` card should have closed. If it has
   not, say so and continue: the brief saves reading, and its absence blocks nothing.

If gate 1 or 2 fails, say which and stop. Do not fix it for me.

## Steps

1. **Start from the brief**, in its run report under `docs/07 Journal/autoruns/`. Check every
   claim you use against the design note or the code before relying on it.
2. **Source the work from design notes**: `docs/06 Sprints/Planning Workflow - Artifact Gate.md`
   § *Where plans come from* is the rule; this step executes it. Read
   `docs/02 Roadmap/Roadmap.md` (the next goal is the first unfinished one on the ladder),
   `docs/01 Product/Capability Map.md` for what its systems must support, the current quarter
   note and the active sprint. Then, for each system in play, **start at its design note in
   `docs/04 Design Docs/`** and work the two sources in order:
   - **Decided ∧ unbuilt ∧ has a consumer now**: the delta between the note's *Decided*
     rows and the code. This is the queue; these become **Dev** tasks.
   - **An open question that blocks one of those**: becomes a decision to settle in step 4.
     An open question with nothing waiting on it earns **no card**.
   Follow an ADR link into `docs/03 Architecture/` only when the **rationale** carries the
   argument (CLAUDE.md rule 2). **Never size a card off an ADR body alone**: an Accepted
   ADR can hold a partially-superseded clause ([[ADR Index]] → *Partial supersessions*); the
   design note's *Decided* rows are the reconciled view.
   - **Coverage check.** For each **system** in play, no design note → **say it out loud**
     and make drafting the note a Design card, ordered before the dev work it grounds.
     Process/meta ADRs (004, 009, 012) have no system and never trigger this.
   - **`docs/03 Architecture/Known Issues.md`**: one question: *does any `D<n>` block or
     touch what we're planning?* Blocks → a Dev card ordered before it. Touches the same
     files → no card, it rides along in that PR. Neither → it stays. A `D<n>` whose condition
     has actually **fired** is no longer latent: draft a **Bug** card for it.
   - **Known bugs → `B` cards**: any unfixed `B` card in the closing sprint, and Known Issue
     promotions above. They are ordinary cards and displace nothing.
   - **`docs/06 Sprints/Backlog.md` is read LAST**, and only to ask *"has any trigger
     fired?"* It holds no decisions, so nothing can be planned from it.
3. **Propose ONE headline goal** for the next sprint. A Roadmap goal is about a month, so a
   sprint usually delivers its first or second half; say which. Justify it against the
   Capability Map and the affected systems' design notes. Push back if I'm over-scoping.
   - **Brief check.** For each system whose ADR the goal needs, is there a brief in
     `docs/01 Product/Capabilities/`? No → a Design card to write it, ordered before the ADR
     card ([[Planning Workflow - Artifact Gate]] § *Coverage check: a system with no capability
     brief*). The card runs `/brief <system>`: I decide its content; you interview me.
   - **Showcase card.** When the sprint ends a Roadmap goal, draft the card that records and
     publishes its showcase ([[Channel]] § *How a video fits the plan*), and run `/showcase`
     with me so the card has its outline.
   - **Off-map work.** A candidate the Capability Map does not list goes through `/triage`
     before it becomes a draft card.
4. **Settle the decisions the goal needs**, by the artifact gate, per item:
   - **Light** (a short note, the call is essentially clear): decide it with me now and
     write it into the design note in this session.
   - **Heavy** (an ADR, or a note gated on a spike): name it. If the closing sprint still has
     a slot, I may run `/adr` before the boundary. Otherwise it becomes a **day-1 Design
     card** of the next sprint, ordered before every card it unblocks, and the story under it
     stays unsized (§ *Don't size past an open decision*).
5. **Draft the cards**, Epic → Story → Task, as equal drivers. Every task fits one 2-6h
   session with a clear done-condition.
   - **Kind, priority + weight on every draft**: kind as a letter without a number (`T` Dev,
     `D` Design, `B` Bug, `P` Process), because `/sprint-plan` assigns the IDs. Then
     `P1/P2/P3` and a weight (🟢 Deep / 🟠 Moderate / 🟡 Light / 🤖 Auto / 🤖 Claude).
     Design cards come before the story they unblock; Process cards are the first cut.
   - **The 🤖 Auto pass.** Read the lane's status on the [[Dashboard]] and in
     [[Autonomous Lane - Routine Prompt]] § *Routine configuration*; if it is paused, say
     so. Then look for Auto work actively: Process and research cards, behaviour-preserving
     refactors and small fully specified changes named in design notes or [[Known Issues]],
     test scaffolding for the Dev cards, and fired [[Backlog]] entries. Run each through
     § *The 🤖 Auto gate*. **A 🟡 Light card goes to the Auto lane by default** when it
     passes; my attended cards are 🟢 and 🟠.
   - **The 🤖 Claude pass.** Propose candidates by § *The 🤖 Claude gate*: work I would
     rather not write (plumbing, file handling, glue), and contained features the roadmap or
     a design note already names. I pick. Tag the story `🤖 Claude`; its cards inherit it.
     When a feature still needs design-note decisions, its first card is a 🤖 Claude Design
     card that takes them and cuts the rest of the story.
   - **Keep every lane card off the files of my cards.** A lane card that collides stops at
     run time.
   - **Do not fit to capacity here.** `/sprint-plan` does that once the closing sprint's
     carry-over is known. Give a rough count by weight, so over-scope shows early.
6. **Write the draft cut into the closing sprint note**, under `#### Sprint <n+1>'s cut`
   inside this card's story, as numbered drafts. S8-D2's *Sprint 09's cut* in
   [[2026-10 Sprint 08 - Content Path]] is the model. Name the proposed goal and any day-1
   Design card above the list. Mark which drafts come from fired [[Backlog]] entries; the
   entries themselves stay until `/sprint-plan` pulls them.
   **The cut records no decision itself.** Each decision lives in its design note (light) or
   ADR (heavy), and the cut links the `§` that each draft is cut against, so the cards and the
   decisions keep one home each.
7. **Close the card** with `/card-close`. It is a vault-only Design card, so it closes on its
   vault commit.

## Guardrails

- **Planning only.** No implementation, no new sprint note, and no board change beyond this
  card. Those are `/sprint-plan`'s.
- **Never advance the `Reconciled against` stamp.** That is `/sprint-plan`'s drift check.
- **Never apply a heavy decision here.** An ADR is drafted with `/adr` and accepted by me.
