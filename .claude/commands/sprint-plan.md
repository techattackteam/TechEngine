---
description: Plan the next 2-week sprint (boundary-weekend ritual)
argument-hint: "[sprint theme or focus, if you have one in mind]"
---

Act as my technical lead **and scrum master** for TechEngine sprint planning: we build the
plan together as equal drivers (you draft and challenge, I decide scope).

Sprints are **2 weeks**, each week **Sat → Fri**. Today (the planning weekend) is **day 1 of
the new sprint**, not the tail of the old one, and it is still a dev day; keep this session
tight. Set the new sprint's date range from that rule: start = this Saturday, end = the
Friday 2 weeks out. **Never derive sprint dates from the calendar month.**

**This is the fitting half of planning.** The thinking half ran in week 2 of the closing sprint
as `/sprint-prep`: the goal, the decisions and the draft cards (2026-10-23,
[[Planning Workflow - Artifact Gate]] § *Decide first, then plan*). This session reviews the
sprint that ended, then turns the draft into the new sprint. It does not design.

**This command absorbs `/weekly-review` on a boundary weekend; do not run both** (decided
2026-07-26). The retro covers the same ground at sprint scope, and running both wrote two
journal entries and updated the dashboard twice before I got to code. The weekly review's
stale-artifact check is *not* dropped; it's step 3 below.

My input on focus (may be empty): $ARGUMENTS

**Size:** keep the retro short, with no section over ~30 lines. The sprint note is the
template's skeleton; each task carries its title, done-condition and tags, and may wrap. No
prose recap of work already recorded elsewhere; link it. Do not fill sections merely to make
a note look complete, and never compress a sentence to hit a length.

Steps:
1. **Load the draft cut.** Read the closing sprint note's `#### Sprint <n+1>'s cut`, which
   the `Prepare Sprint <n+1>` card wrote, and the prep brief it started from. Then check what
   moved since the prep ran: cards that merged or rolled over after it, a bug that arrived,
   a [[Known Issues]] entry that fired. Adjust the draft for those and say what changed.
   **If no prep ran** (the card was dropped, or its slot was lost), run `/sprint-prep`'s
   steps 2 to 6 here first, and record it in the retro: a boundary that has to design is
   the failure this split exists to prevent.
2. If the current sprint is ending, first help me capture a short retrospective
   using `docs/Templates/Retrospective Template.md` into `docs/07 Journal/`. Cover the
   **final week** in it too (`git log --since="7 days ago" --oneline`), no separate weekly
   review runs this weekend, including the honest sustainability check: did the sprint
   respect the cadence (deep/moderate/light/off)? energy, job+engine+karting balance,
   anything to cut. **Report the kind mix as numbers**: how many Dev / Design / Bug /
   Process cards closed. Bug load is unplanned work the sprint absorbed, and Process load is
   how much of it wasn't the goal; both are invisible unless counted.
   **Report the Auto lane's sprint too**, from the run reports in `docs/07 Journal/autoruns/`:
   the cards it closed, the PRs it opened and whether they merged, the papers it screened,
   and every run that stopped, with why. A lane that ran and produced nothing is a finding.
   For the code sweeps, report the modules swept, the lines removed, the findings filed, and
   each module's line count against its previous sweep, so growth is visible.
   **Report each 🤖 Claude feature**: whether it shipped, its PRs, the decisions it took and
   how many I reversed in review, and any card it stopped on. Reversals are the measure of
   whether its gate is drawn in the right place.
   **Report the prep**: which day it ran, and how many of its drafts survived step 1 unchanged.
3. **Stale-artifact check** (inherited from `/weekly-review`, which does not run today).
   For features touched this sprint, spot-check their ADR / design note against what
   actually got built: did implementation diverge from the documented end-state? A stale
   artifact is poisoned ground truth: Claude anchors to the wrong thing (CLAUDE.md rule 2).
   **Also check hub drift:** for touched systems, the design note's *Decided* one-liners vs
   the ADRs they index, the hub pattern reads the note *first*, so a drifted one-liner
   silently overrides the ADR ([[Planning Workflow - Artifact Gate]]). List drift and offer
   to reconcile; if none, say so.
   Then **advance the reconciliation stamp** on [[Dashboard]]:
   `**Reconciled against:** engine <sha> (date)`, `<sha>` = current `origin/master`,
   **only if this step actually ran to completion**. Skipped, partial, or reconciling
   deferred → leave it and say why. A formality stamp is worse than none: it turns
   "unknown freshness" into a false "checked" that CLAUDE.md rule 2 then trusts
   (ADR-012 §6).
4. **Confirm the headline goal** the prep proposed. Re-open it only if the last days of the
   closing sprint changed the picture, and push back if I'm over-scoping.
5. **Fit the draft to capacity**, as equal drivers: I decide what drops.
   - **Count my slots by the real weekly rhythm** (deep Mon/Thu + one weekend day; moderate
     Fri; light Tue; relaxed Wed; the other weekend day off), the Dashboard calendar.
     Weekend days are a swappable pair, so don't hard-assign Sat/Sun. **Deep days are not
     equal capacity:** Mon/Thu are after the day job → ~1 🟢 each; the weekend deep day is a
     full day → **2-3 🟢**. My attended cards are 🟢 and 🟠; my 🟡 slots go to reviewing the
     lanes' PRs, at ~15 minutes per 🤖 Auto PR and 30-45 per 🤖 Claude PR.
   - **The lanes do not raise my capacity**, and they have their own limit: one PR per fire,
     shared by 🤖 Auto and 🤖 Claude cards, ~5 per sprint week. Report-only and vault-only
     Auto cards are free. Check the lane's status on the [[Dashboard]] and in
     [[Autonomous Lane - Routine Prompt]] § *Routine configuration*; if it is paused, say so
     and ask me whether to plan lane cards anyway.
   - **Assign IDs**: the kind lives in the card ID (`S3-T4` Dev · `S3-D1` Design · `S3-B1`
     Bug · `S3-P2` Process), then the draft's `P1/P2/P3` and weight, as `· P1 · 🟢 Deep`.
     Day-1 Design cards come first, and the stories they unblock stay unsized.
   - **Process cards are the first thing cut** if capacity tightens; call the mix out loud
     when a sprint is more than about a third Process.
   - **Bugs enter two ways, and they behave differently.** Known *at planning* → an ordinary
     card, sized in with everything else, nothing displaced. Arriving *mid-sprint* → still
     taken that sprint, but it **displaces**: name what it pushes out (lowest-priority
     Process first) rather than stacking it on top, since piling unplanned work onto a full
     sprint is the refill reflex the Sprint 01 retro flagged. Record the mid-sprint kind the
     way T13-T15 were: a dated scope-change note in the sprint file.
6. **Plant the two standing cards** in the new sprint, so the next prep is on the board from
   day 1:
   - `S<n>-P<k> · Prep brief for Sprint <n+1> · P2 · 🤖 Auto`, report-only. done: a run report
     lists the *Decided* rows with no code behind them, the open questions blocking them, the
     fired [[Backlog]] triggers and candidate 🤖 Claude features, and decides nothing. **Not
     before the Monday of week 2**: a fire before that leaves it in To Do.
   - `S<n>-D<k> · Prepare Sprint <n+1> · P1 · 🟠 Moderate · 2-3h`. done: `/sprint-prep`
     wrote the next sprint's goal and draft cut into this sprint note. Runs in week 2, Tuesday
     to Friday, after the brief when it exists.
7. Create the sprint note from `docs/Templates/Sprint Template.md` and update the roadmap
   and the dashboard, including the Dashboard's **Next ceremony** line, which
   `/weekly-review` would normally roll.
8. **Cut every pulled item out of `docs/06 Sprints/Backlog.md`**: pulling is a *move, not a
   copy*. As each task is written, delete its entry. Nothing needs carrying across: entries
   are one line and hold no decisions, so there is nothing to rescue. If one *does* hold a
   decision, that's the finding: the entry outgrew the file and its content belongs in the
   design note or ADR, not in a longer backlog entry.
9. **Reset `docs/06 Sprints/Sprint Board.md` for the new sprint**: the board shows *live
   state*, never history. It carries exactly **one** Done column, for the sprint in flight.
   Do this only after the retro (step 2) has mined it:
   - Fold the closing sprint's Done cards into that sprint note's `## Sprint review`
     ("what shipped" + demo/artifact), condensed to a line each. The sprint note is the
     permanent record; the column is not.
   - **Carry the unfinished before emptying anything.** Every card left in To Do, In Progress
     or Review / Demo is either **re-planned into the new sprint note** (new sprint's ID
     prefix, old ID noted) or **consciously dropped and said out loud**. Emptying a column is
     not a decision about the work in it. **An unfixed `B` card is never dropped**: a bug
     doesn't stop being real because a sprint ended; re-plan it, or if it turns out to be
     latent-and-silent after all, demote it to a `D<n>` in [[Known Issues]]. Anything dropped
     without a home is the same gap the last bullet of this step already names.
   - **Empty** Done, In Progress, and Review / Demo; retitle Done for the new sprint.
   - Rebuild **To Do** from the new sprint note's tasks, tags included (`· P1 · 🟢 Deep`).
   - Backlog column stays a bare pointer to [[Backlog]]; cards never accumulate there.
   Never open a second Done column to keep old cards around, and **never add a column for a
   task kind**; kind lives in the card ID (a column would be a second home for the same
   fact, and it hides that a Design card is sprint-bound critical-path work). **The same
   applies to the 🤖 Auto and 🤖 Claude lanes: no column for either.** Lane is a weight, it
   already shows in the card's tag, and a column would be that second home. If a card in a
   closing column has no home in a sprint note, that's a gap: say so and place it before
   cutting.
10. This is planning only; no implementation.
