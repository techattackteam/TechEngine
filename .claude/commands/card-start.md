---
description: Ground a card before building it — freshness check, design note, open defects, branch
argument-hint: "[card ID, e.g. S3-T14 — omit and I'll take the next one on the board]"
---

Act as my technical lead grounding a card before I build it. Card: $ARGUMENTS

This is `CLAUDE.md` rule 2 made mechanical. The rule's failure mode is anchoring to an
artifact that the engine has already moved past, and the whole point of running this before
the work is that the anchoring happens silently otherwise.

**GATE — check before anything else.** Read the card in the active sprint note under
`docs/06 Sprints/`. If its predecessor has not merged (the story chains on [[Sprint Board]]
read `T11 → T12 → T13`, and the sprint note's *Ordering* names the hard sequences), stop and
say which card comes first. Grounding a card that cannot start yet is wasted.

## Gather

1. **The card's own text.** Its `done:` clauses are the acceptance, and they are the thing
   you check the tree against below.
2. **Freshness, first, because it decides how much to trust everything after it.**
   Read the [[Dashboard]]'s `**Reconciled against:** engine <sha> (date)` line, then:
   `git -C C:/dev/TechEngine fetch origin && git -C C:/dev/TechEngine log --oneline <sha>..origin/master`
   That is the last engine commit a drift check **actually ran against** (ADR-012 §6). If the
   engine is ahead, every design note is **suspect** and you say so in the brief rather than
   citing one as current. **Distance is a signal, not proof** — a note may be perfectly fine
   at forty commits or wrong at two, and the stamp cannot tell you which. Judgement still
   applies; the count only says how hard to look.
3. **The system's design note** in `docs/04 Design Docs/`. Start here, never at the ADRs. Its
   *Decided* rows index the ADR sections, so follow a link only when the **rationale** is what
   you actually need.
4. **[[Known Issues]]** — the open defects on this system. One of them is often the card's
   subject, and more often the thing the card is about to walk into.
5. **The code the card touches**, at `origin/master`. This is where the checking happens.

## What the brief is for

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
- **A decision this card needs that no artifact makes.** Say the gap exists and stop there.
  Inventing the answer on the spot is the exact failure rule 2 was written against, and it is
  worse than a blocked card because it looks like ground truth afterwards.
- **What it unblocks**, and whether it closes its story.

**Does not:**
- A restatement of the card, or of the design note.
- Rationale copied from an ADR. Link the `§`.
- An implementation plan, a design sketch, or code. **I write the engine.**

**If the ground is clean, the brief is three lines**: what the card must satisfy, that the
artifacts match the tree, and go. A padded brief trains me to skip the real finding when one
is there.

**Budget:** ~20 lines.

## Then

Offer to cut the branch, and cut it only if I say yes:

`git -C C:/dev/TechEngine fetch origin && git -C C:/dev/TechEngine switch -c <card ID>/<slug> origin/master`

From **freshly fetched `origin/master`**, never local `master` and never a previous card's
branch. The repo squash-merges, so branching off a merged branch replays its whole PR as a
conflict against itself, which is what happened to PRs #8 through #10 (`CLAUDE.md` rule 9).
The `<card ID>/` prefix is the only link from the squashed commit back to the board card.

## Guardrails

- **Never advance the `Reconciled against` stamp.** You are reading it, not answering it. It
  moves only as the output of `/weekly-review`'s real drift check (ADR-012 §6).
- **Report drift, do not sweep it.** If a note is wrong, say so. You may fix the specific rows
  this card depends on if I ask, but a broad reconciliation is `/vault-clean` and
  `/weekly-review`, and doing it here hides the drift from the check that is supposed to
  record it.
- **Never edit an Accepted ADR.**
- **Stop at the brief.** No engine implementation, no scaffolding unless I ask for it by name.
