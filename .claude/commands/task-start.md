---
description: Cut a fresh task branch from origin/master and open the card
argument-hint: "[task ID, e.g. S2-T4 — omit to pick from the board]"
---

Start a TechEngine task: pick the card, cut the branch, open it on the board.

Task: $ARGUMENTS

## 1. Pick the card

Read `docs/06 Sprints/Sprint Board.md`. If a task ID was given, find that card. If not,
list the **To Do** cards (ID, title, priority, effort) and ask which one — one task per
session (CLAUDE.md rule 5), so pick exactly one.

If the ID isn't on the board, stop and say so — don't invent a card.

## 2. Preflight — do not skip

Run these and **stop on the first failure**, reporting what's wrong instead of working
around it:

- `git status --porcelain` — must be clean. If it isn't, list the files and ask whether
  to stash, commit, or abort. Never switch branches over a dirty tree.
- `git fetch origin --prune`
- `git branch --list <branch>` — must be empty. A branch by that name already exists means
  the task was started before; ask rather than clobbering it.

## 3. Cut the branch

Name it `<card ID>/<slug>` — the card ID verbatim so it maps 1:1 onto the board and stays
unique across sprints, plus a 2–3 word kebab slug from the title (drop priority, effort,
wikilinks, punctuation). `S2-T4 — Assert: four tiers + single handler` → `S2-T4/assert-tiers`.

```
git checkout -b <branch> origin/master
```

**`origin/master`, always — never local `master`, never the previous task's branch.** The
repo squash-merges onto a linear history, so a merged branch's commits never appear on
master; building on one replays the whole PR as a conflict against itself. This line is the
reason this command exists.

## 4. Open the card

Move the card from **📋 To Do** to **🔨 In Progress** on the board, keeping the line intact.
Commit that file alone as the branch's first commit:

```
git commit -m "<card ID>: start — <title>"
```

Nothing else goes in this commit. Don't touch the Done sections, the sprint note, or the
Dashboard — those are the weekend ceremony's to write (`/weekly-review`), and every branch
that edits them is a merge conflict against every other branch that does.

## 5. Hand off

Report in three lines: branch name, the card's one-line goal, and the design artifact that
governs it — the system's note in `docs/04 Design Docs/`, or the ADR if there's no note yet
(CLAUDE.md rule 2). If the card is load-bearing and has **neither**, say so: that's the
artifact gate ([[Planning Workflow — Artifact Gate]]) failing, and it's worth a `/adr` or a
design note before code.

Then stop. **Do not start implementing** — Miguel writes the engine. This command sets up
the workspace and hands him the wheel.
