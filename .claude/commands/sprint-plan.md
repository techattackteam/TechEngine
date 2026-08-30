---
description: Plan the next 2-week sprint (boundary-weekend ritual)
argument-hint: "[sprint theme or focus, if you have one in mind]"
---

Act as my technical lead **and scrum master** for TechEngine sprint planning — we build the
plan together as equal drivers (you draft and challenge, I decide scope).

Sprints are **2 weeks**, each week **Sat → Fri**. Today (the planning weekend) is **day 1 of
the new sprint**, not the tail of the old one, and it is still a dev day — keep this session
tight. Set the new sprint's date range from that rule: start = this Saturday, end = the
Friday 2 weeks out. **Never derive sprint dates from the calendar month.**

**This command absorbs `/weekly-review` on a boundary weekend — do not run both** (decided
2026-07-26). The retro covers the same ground at sprint scope, and running both wrote two
journal entries and updated the dashboard twice before I got to code. The weekly review's
stale-artifact check is *not* dropped — it's step 3 below.

My input on focus (may be empty): $ARGUMENTS

**Budget:** retro **≤ 80 lines**, no section over ~20. Sprint note = the template's
skeleton, **one line per task** (title, done-condition, tags). Sprint-review entries are one
line each. No prose recap of work already recorded elsewhere — link it.

Steps:
1. **Source the work from design notes** — `docs/06 Sprints/Planning Workflow — Artifact Gate.md`
   § *Where plans come from* is the rule; this step executes it. Read
   `docs/02 Roadmap/Roadmap.md`, the current quarter note and the active sprint for
   direction. Then, for each system in play, **start at its design note in
   `docs/04 Design Docs/`** and work the two sources in order:
   - **Decided ∧ unbuilt ∧ has a consumer now** — the delta between the note's *Decided*
     rows and the code. This is the queue; these become **Dev** tasks.
   - **An open question that blocks one of those** — becomes a **Design** task. An open
     question with nothing waiting on it earns **no card**.
   Follow an ADR link into `docs/03 Architecture/` only when the **rationale** carries the
   argument (CLAUDE.md rule 2). **Never size a card off an ADR body alone** — an Accepted
   ADR can hold a partially-superseded clause ([[ADR Index]] → *Partial supersessions*); the
   design note's *Decided* rows are the reconciled view.
   - **Coverage check.** For each **system** in play, no design note → **say it out loud**
     and put drafting the note in this sprint as a Design task, ordered before the dev work
     it grounds. Read the ADR directly meanwhile — that's the fallback, not the plan.
     Process/meta ADRs (004, 009, 012) have no system and never trigger this.
   - **`docs/03 Architecture/Known Issues.md`** — one question: *does any `D<n>` block or
     touch what we're planning?* Blocks → a Dev card ordered before it. Touches the same
     files → no card, it rides along in that PR. Neither → it stays; that's the list working,
     not a backlog of unfixed work. A `D<n>` whose condition has actually **fired** is no
     longer latent: promote it to a **Bug** card and delete the entry.
   - **Known bugs → `B` cards, planned in like any other work.** A bug is anything that
     misbehaves *now*. Sweep three places: any **unfixed `B` card from the closing sprint**
     (it carries — see step 8), anything the retro or the stale-artifact check surfaced as
     actually broken, and Known Issue promotions above. Bugs planned in at the boundary are
     **ordinary sized cards** — they displace nothing, because the sprint is being sized
     fresh around them. Only a bug *arriving mid-sprint* displaces (step 5).
   - **`docs/06 Sprints/Backlog.md` is read LAST**, and only to ask *"has any trigger
     fired?"* It is a parking lot of one-line wants — it holds no decisions, so nothing can
     be planned from it.
2. If the current sprint is ending, first help me capture a short retrospective
   using `docs/Templates/Retrospective Template.md` into `docs/07 Journal/`. Cover the
   **final week** in it too (`git log --since="7 days ago" --oneline`) — no separate weekly
   review runs this weekend — including the honest sustainability check: did the sprint
   respect the cadence (deep/moderate/light/off)? energy, job+engine+karting balance,
   anything to cut. **Report the kind mix as numbers** — how many Dev / Design / Bug /
   Process cards closed. Bug load is unplanned work the sprint absorbed, and Process load is
   how much of it wasn't the goal; both are invisible unless counted.
3. **Stale-artifact check** (inherited from `/weekly-review`, which does not run today).
   For features touched this sprint, spot-check their ADR / design note against what
   actually got built: did implementation diverge from the documented end-state? A stale
   artifact is poisoned ground truth — Claude anchors to the wrong thing (CLAUDE.md rule 2).
   **Also check hub drift:** for touched systems, the design note's *Decided* one-liners vs
   the ADRs they index — the hub pattern reads the note *first*, so a drifted one-liner
   silently overrides the ADR ([[Planning Workflow — Artifact Gate]]). List drift and offer
   to reconcile; if none, say so.
   Then **advance the reconciliation stamp** on [[Dashboard]] —
   `**Reconciled against:** engine <sha> (date)`, `<sha>` = current `origin/master` —
   **only if this step actually ran to completion**. Skipped, partial, or reconciling
   deferred → leave it and say why. A formality stamp is worse than none: it turns
   "unknown freshness" into a false "checked" that CLAUDE.md rule 2 then trusts
   (ADR-012 §6).
4. Propose ONE headline goal for the next sprint. Justify it against the roadmap and the
   affected systems' design notes — dropping to their ADRs where a decision's rationale
   carries the argument. Push back if I'm over-scoping.
5. **Co-create** the Epic → Story → Task breakdown with me as an equal driver — draft
   and challenge, I decide scope. Every task must fit one 2–6h coding session with a
   clear done-condition. Size the sprint to my real weekly rhythm (deep Mon/Thu + one
   weekend day; moderate Fri; light Tue; relaxed Wed; the other weekend day off) — the
   Dashboard calendar. Weekend days are a swappable pair, so don't hard-assign Sat/Sun.
   **Deep days are not equal capacity:** Mon/Thu are after the day job → ~1 🟢 each; the
   weekend deep day is a full day → **2–3 🟢**. Count slots that way, not one-per-day.
   - **Artifact gate** (`docs/06 Sprints/Planning Workflow — Artifact Gate.md`) — ADR,
     design note, or neither, by reversibility × blast radius; rarely both. Then apply
     **artifact timing**: flag every artifact-less item that warrants one — **draft the
     light ones with me now, in this session**; heavy ones (ADR / needs a spike) become
     a sprint task ordered before the impl task they unblock.
   - **Heavy-gated stories stay unsized** — where the gate demanded an ADR (or a
     spike-gated note), the breakdown **stops at that artifact's task**: the story below it
     goes into the sprint note named and roughly counted (`~2–3 tasks · size after ADR-NNN`),
     with no done-conditions, and cutting its cards is part of the artifact task's own
     done-condition. Sizing them now writes cards the ADR will overturn (S2-T2's `fmt` seam)
     — see the gate note § *Don't size past an open decision*. Do not "helpfully" fill them
     in to make the sprint note look complete.
   - **Kind, priority + weight on every task** — the **kind lives in the card ID**
     (`S3-T4` Dev · `S3-D1` Design · `S3-B1` Bug · `S3-P2` Process), then `P1/P2/P3` and a
     weight (🟢 Deep / 🟠 Moderate / 🟡 Light / 🤖 Auto) matching the day-type it fits. Tag
     format `· P1 · 🟢 Deep`. Design tasks are ordered **before** the story they unblock;
     Process tasks are the **first thing cut** if capacity tightens — call the mix out loud
     when a sprint is more than about a third Process.
   - **Fill the two lanes separately, attended first.** Size the attended lane against the
     capacity table exactly as before — the 🤖 Auto lane does **not** raise it. Then make a
     second pass over what is left and run each candidate through the artifact gate's
     § *The 🤖 Auto gate* (four questions; one no disqualifies). Process and research cards
     are the natural population, and moving them is the point: every sustainability overrun
     of Sprint 04 was process work, not engine work.
     **Cap the Auto lane at one PR-producing card per weekday**, ~5 per sprint week, because
     each costs 16.1 billed CI minutes and ~15 minutes of Miguel's review. Report-only and
     vault-only Auto cards are free and are not capped.
     Full model: [[Autonomous Lane — Design]].
   - **Bugs enter two ways, and they behave differently.** Known *at planning* (step 1) → an
     ordinary card, sized in with everything else, nothing displaced. Arriving *mid-sprint* →
     still taken that sprint, but it **displaces**: name what it pushes out (lowest-priority
     Process first) rather than stacking it on top, since piling unplanned work onto a full
     sprint is the refill reflex the Sprint 01 retro flagged. Record the mid-sprint kind the
     way T13–T15 were — a dated scope-change note in the sprint file.
6. Create the sprint note from `docs/Templates/Sprint Template.md` and update the roadmap
   and the dashboard — including the Dashboard's **Next ceremony** line, which
   `/weekly-review` would normally roll.
7. **Cut every pulled item out of `docs/06 Sprints/Backlog.md`** — pulling is a *move, not a
   copy*. As each task is written, delete its entry. Nothing needs carrying across: entries
   are one line and hold no decisions, so there is nothing to rescue. If one *does* hold a
   decision, that's the finding — the entry outgrew the file and its content belongs in the
   design note or ADR, not in a longer backlog entry.
8. **Reset `docs/06 Sprints/Sprint Board.md` for the new sprint** — the board shows *live
   state*, never history. It carries exactly **one** Done column, for the sprint in flight.
   Do this only after the retro (step 2) has mined it:
   - Fold the closing sprint's Done cards into that sprint note's `## Sprint review`
     ("what shipped" + demo/artifact), condensed to a line each. The sprint note is the
     permanent record; the column is not.
   - **Carry the unfinished before emptying anything.** Every card left in To Do, In Progress
     or Review / Demo is either **re-planned into the new sprint note** (new sprint's ID
     prefix, old ID noted) or **consciously dropped and said out loud**. Emptying a column is
     not a decision about the work in it. **An unfixed `B` card is never dropped** — a bug
     doesn't stop being real because a sprint ended; re-plan it, or if it turns out to be
     latent-and-silent after all, demote it to a `D<n>` in [[Known Issues]]. Anything dropped
     without a home is the same gap the last bullet of this step already names.
   - **Empty** Done, In Progress, and Review / Demo; retitle Done for the new sprint.
   - Rebuild **To Do** from the new sprint note's tasks, tags included (`· P1 · 🟢 Deep`).
   - Backlog column stays a bare pointer to [[Backlog]] — cards never accumulate there.
   Never open a second Done column to keep old cards around, and **never add a column for a
   task kind** — kind lives in the card ID (a column would be a second home for the same
   fact, and it hides that a Design card is sprint-bound critical-path work). **The same
   applies to the 🤖 Auto lane: no column for it either.** Lane is a weight, it already shows
   in the card's tag, and a column would be that second home. If a card in a closing column
   has no home in a sprint note, that's a gap — say so and place it before cutting.
9. This is planning only — no implementation.
