---
description: Close out a task branch — annotate the card, capture strays, remind me to open the PR
argument-hint: "[what landed / anything I want recorded]"
---

Close out the task on the current branch. Counterpart to `/task-start`.

Notes from me: $ARGUMENTS

## 1. Identify the task

Derive the card ID from the branch name (`S2-T6/clock` → **S2-T6**). If the branch doesn't
carry an ID, ask which card this is rather than guessing. Read that card on
`docs/06 Sprints/Sprint Board.md` and its task line in the sprint note.

## 2. Check the done-condition — honestly

The sprint note gives each task an explicit done-condition. State whether it's met, using
`git diff origin/master...HEAD --stat` for what actually changed.

**Verification is Miguel's** (CLAUDE.md "Build & run"). Do not claim built, green, or tested.
Ask what he ran and record his answer; if he didn't build it, the card says so. A partially
met condition is recorded as partial — never rounded up.

## 3. Annotate and move the card

Move the card to **👀 Review / Demo** and annotate it in the house format — date, then the
outcome in one or two lines:

```
- [x] **S2-T6** — Clock implementation · P1 · 🟠 Moderate — **Jul 26** → PR #NN.
	  <what shipped, the decision it honoured (ADR-011 §3), anything deferred to a later task>
```

This annotation is the retrospective's raw material — a card that just says "done" makes the
retro guesswork. Record what shipped, the `§ref` it implements, and anything punted.

Done is set when the PR **merges**, not now — the next ceremony sweeps Review → Done.

## 4. Capture strays

Anything noticed mid-task and deliberately not chased (CLAUDE.md rule 5) goes to
`docs/06 Sprints/Backlog.md` as a parked item **with a `**Trigger:**`** — the backlog's own
rule. Bugs found but not fixed go there too, not into the diff.

If the work revealed that a design note or ADR is now stale, say so and offer to reconcile.
Don't quietly leave a drifted artifact behind — that's poisoned ground truth for the next
session (CLAUDE.md rule 2).

## 5. Commit the vault change

Commit **only** the board (+ backlog, if step 4 added anything):

```
git commit -m "<card ID>: wrap — <one-line outcome>"
```

Nothing else. Don't touch the Done sections or the Dashboard — ceremony territory.
**No AI attribution in the message** (CLAUDE.md rule 10).

## 6. Stop here and remind me

**Do not push and do not open the PR.** End the session with exactly this, filled in:

> **Ready for PR.** Branch `<branch>` — <n> commits, <what it contains>.
> Unverified by Claude: <what Miguel still needs to build/run, or "Miguel confirmed: <result>">.
>
> ```
> git push
> ```
> Upstream was set by `/task-start`, so this is a bare push. Then open the PR against
> `master` on GitHub.
> **On merge:** the branch is dead — squash + linear history means it can never be reused.
> Delete it and start the next task with `/task-start`.
