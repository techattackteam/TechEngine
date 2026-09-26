---
description: Ground a card I have started - freshness check, design note, open defects, then scaffold (Dev) or propose (planning)
argument-hint: "[card ID, e.g. S3-T14; omit and I'll take it from the current branch]"
---

Act as my technical lead grounding a card I have just started. Card: $ARGUMENTS

With no ID, take it from the current branch name (`<card ID>/<slug>`).

This is `CLAUDE.md` rule 2 made mechanical. The rule's failure mode is anchoring to an
artifact that the engine has already moved past, and the whole point of running this before
the work is that the anchoring happens silently otherwise.

## Gates: I do these, you check them

Starting a card is my call, so the branch and the board move are mine. Check all three before
anything else. If one fails, say which and stop. Do not fix it for me.

1. **Order.** Read the card in the active sprint note under `docs/06 Sprints/`. If its
   predecessor has not merged (the story chains on [[Sprint Board]] read `T11 → T12 → T13`,
   and the sprint note's *Ordering* names the hard sequences), say which card comes first.
2. **Board.** The card sits in **In Progress** on [[Sprint Board]].
3. **Branch.** `git -C C:/dev/TechEngine fetch origin`, then the current branch must be named
   `<card ID>/<slug>`, and `git -C C:/dev/TechEngine log --oneline origin/master..HEAD` must list
   nothing but this card's own commits. Another card's commits there mean the branch was cut
   from a merged branch, which replays that PR as a conflict against itself (PRs #8-#10,
   `CLAUDE.md` rule 9).

## Gather

1. **The card's own text.** Its `done:` clauses are the acceptance, and they are the thing
   you check the tree against below.
2. **Freshness, first, because it decides how much to trust everything after it.**
   Read the [[Dashboard]]'s `**Reconciled against:** engine <sha> (date)` line, then:
   `git -C C:/dev/TechEngine log --oneline <sha>..origin/master`
   That is the last engine commit a drift check **actually ran against** (ADR-012 §6). If the
   engine is ahead, every design note is **suspect** and you say so in the brief rather than
   citing one as current. **Distance is a signal, not proof**: a note may be perfectly fine
   at forty commits or wrong at two, and the stamp cannot tell you which. Judgement still
   applies; the count only says how hard to look.
3. **The system's design note** in `docs/04 Design Docs/`. Start here, never at the ADRs. Its
   *Decided* rows index the ADR sections, so follow a link only when the **rationale** is what
   you actually need.
4. **[[Known Issues]]**: the open defects on this system. One of them is often the card's
   subject, and more often the thing the card is about to walk into.
5. **The code the card touches**, at `origin/master`. This is where the checking happens.

## The brief

It exists to tell me what would **change how I build this card**. I can read the card myself,
so a summary of it is worth nothing.

**Earns a place:**
- **A `done:` clause with no referent in the tree.** The clause names a type, a file or a
  function that does not exist, or that exists and means something else. This has now happened
  twice (S3-B1's three clauses, S3-T13's `EngineContext`), so check every clause against the
  code rather than reading them as a description.
- **A design note claim the code contradicts**, with the `file:line` that shows it. Cite
  nothing you have not opened.
- **An open [[Known Issues]] entry** the card will touch, by ID.
- **A decision this card needs that no artifact makes.** Say the gap exists. On a Dev card,
  that stops the scaffold below; on a planning card, it is what the proposal answers.
- **What it unblocks**, and whether it closes its story.

**Does not:**
- A restatement of the card, or of the design note.
- Rationale copied from an ADR. Link the `§`.

If the ground is clean, keep the brief to a few lines: what the card must satisfy, that the
artifacts match the tree, and go.

## Then, by card kind

### Dev card (`T`): scaffold it

Write the smallest scaffold that unblocks me: new files, headers, declarations, signatures,
CMake wiring, and test cases named after the `done:` clauses. Leave the logic to me. Bodies stay
empty or stubbed, and a `TODO(<card ID>)` marks each place I fill in.

- Read `CONVENTIONS.md` and the files you are adding to first, and match them (rule 0).
- The shape comes from the design note and the card. If the brief found a decision no
  artifact makes, **do not scaffold around it**: a guessed signature looks like ground truth
  afterwards. Report the gap and stop.
- Do not compile it. Hand it over saying plainly that it is unverified.
- In the response, list the files and say what each stub is waiting for.

### Planning card (`D` or `P`): propose a solution

Propose how to settle the card's question, grounded in what you gathered. Give at most two
options, the main difference between them, and your pick with the reason. Say where the
decision would be recorded (design note, ADR amendment, new ADR), and flag any load-bearing
choice that needs its own ADR ([[Planning Workflow - Artifact Gate]]).

Stop at the proposal. Write nothing to the vault until I agree on the direction.

## Guardrails

- **Never cut the branch or move the card.** Those are the gates, and they are mine.
- **Never advance the `Reconciled against` stamp.** You are reading it, not answering it. It
  moves only as the output of `/weekly-review`'s real drift check (ADR-012 §6).
- **Report drift, do not sweep it.** If a note is wrong, say so. You may fix the specific rows
  this card depends on if I ask, but a broad reconciliation is `/vault-clean` and
  `/weekly-review`, and doing it here hides the drift from the check that is supposed to
  record it.
- **Do not amend an Accepted ADR while grounding a card.** A design note disagreeing with
  its ADR is drift to **report**. Filing the amendment is part of the card's work, if it is
  anyone's ([[ADR Index]] § *Amending an Accepted ADR*).
- **No engine logic.** A scaffold is structure. Anything that decides behaviour is mine.
