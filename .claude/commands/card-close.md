---
description: Close a merged card — board Done entry, design note, Known Issues, backlog
argument-hint: "[card ID, e.g. S3-T12 — omit and I'll infer it]"
---

Act as my technical lead closing a **merged** card in TechEngine. Card: $ARGUMENTS

**GATE — check before writing anything.** A card is closed by the merge, not by the work
being finished. Confirm the card's PR is merged into `origin/master`
(`git fetch origin && git log origin/master --oneline -10`). If it is not merged, stop and
say so. Nothing below runs on unmerged work.

## Gather first

- **The card's own text**, in the active sprint note under `docs/06 Sprints/`. Its `done:`
  clauses are the acceptance you are closing against. Read them before the diff, so the diff
  is judged rather than summarised.
- **The squashed commit**: sha + PR number. The branch was named `<card ID>/<slug>`, which is
  the only link from a squashed commit back to the card (ADR-012 § *Consequences*).
- **The diff**: `git show --stat <sha>`, then read the files that carry the decisions. Not
  every file.
- **The PR itself**: `gh pr view <n> --comments`. The merged diff is the end state, so it
  cannot show what was written and then deleted, what review caught, or which findings were
  logged rather than fixed. Those are usually the best lines in the entry, and they exist
  **only** in the PR conversation.
- **The system's design note** in `docs/04 Design Docs/`. The card was written against it, so
  it is the thing the card may have landed calls against.
- **The two most recent Done entries** on [[Sprint Board]]. They are the format reference.

## The entry records deviations, not the diff

This is the whole rubric. A Done entry exists so that six months later I can see **what the
card taught**, which the git history cannot tell me.

**Earns a place in the entry:**
- A `done:` clause that had **no referent**, or turned out to mean something other than what
  it said.
- A call that landed **against the design note as written**, plus where the note now records it.
- Something written and then **deleted before it shipped**, and why.
- A review finding that changed the design. Anything **logged not fixed** gets a
  [[Known Issues]] ID and is named by that ID.
- A wrong assumption, a build break, or a process slip worth a **retro line**. Tag it as one.
- What the card **unblocks**, and whether it completes its story.

**Does not:**
- A restatement of the diff, a file list, or line counts.
- "All tests pass." CI is the gate; the entry is not where that gets claimed.
- Rationale already written in an ADR or a design note. Link it.

**If nothing surprised, the entry is three lines. Write three lines.** A padded entry is the
failure mode this rubric exists to stop, and it is worse than a short one, because it teaches
me to skim the column.

**Budget:** ~15 lines, and only a card that genuinely fought back reaches that.

## Then write, in this order

1. **[[Sprint Board]]** — move the card out of In Progress into Done, newest at the top, and
   write the entry.
2. **The sprint note** — tick the card, and any *Definition of Done* line it satisfies. A DoD
   line carries its evidence: date, sha, PR.
3. **The design note** — record the calls the card made against it, in place. Link the ADR
   section, never copy its rationale. If a decision now contradicts an Accepted ADR, that is a
   finding for me, not an edit.
4. **[[Known Issues]]** — one row per logged-not-fixed finding, using the ID the entry cites.
5. **[[Backlog]]** — anything discovered mid-card that was not the card (CLAUDE.md rule 5).
6. **Report**: what you changed, and what needs my call. Then stop.

## Guardrails

- **Never advance the Dashboard's `Reconciled against` stamp.** That is `/weekly-review`'s
  output, and only after a real drift check. A stamp advanced as a formality converts unknown
  freshness into a false "checked" (ADR-012 §6).
- **Do not amend an Accepted ADR during a close.** A card whose *work* was an amendment
  already landed it in its own PR. If closing reveals an ADR that now owes one, that is a
  finding for the report, never a close-time edit
  ([[ADR Index]] § *Amending an Accepted ADR*).
- **Do not review the code.** Findings come from the card's review, already logged in the PR.
  A fresh defect you spot while reading the diff is not a close finding, and inventing
  [[Known Issues]] rows out of new analysis makes every close an unplanned code review. That
  is `te-review`'s job. Mention it to me in the report instead.
- **Never claim a build, test or CI result you did not see.** The merged PR is the evidence.
  If something is unverified, the entry says so.
- The vault is its own repo and commits straight to its `master`. Do not commit unless I ask.

This is a bookkeeping ritual — no engine implementation.
