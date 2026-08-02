---
description: Plan the next 4-week sprint (boundary-weekend ritual)
argument-hint: "[sprint theme or focus, if you have one in mind]"
---

Act as my technical lead **and scrum master** for TechEngine sprint planning — we build the
plan together as equal drivers (you draft and challenge, I decide scope).

Sprints are **4 weeks**, each week **Sat → Fri**. Today (the planning weekend) is **day 1 of
the new sprint**, not the tail of the old one, and it is still a dev day — keep this session
tight. Set the new sprint's date range from that rule: start = this Saturday, end = the
Friday 4 weeks out. **Never derive sprint dates from the calendar month.**

**This command absorbs `/weekly-review` on a boundary weekend — do not run both** (decided
2026-07-26). The retro covers the same ground at sprint scope, and running both wrote two
journal entries and updated the dashboard twice before I got to code. The weekly review's
stale-artifact check is *not* dropped — it's step 3 below.

My input on focus (may be empty): $ARGUMENTS

Steps:
1. Read `docs/02 Roadmap/Roadmap.md` and the current quarter note, the active sprint, and
   `docs/06 Sprints/Backlog.md`. Then, for each system in play, **start at its design note
   in `docs/04 Design Docs/`** — the note is the hub, and its *Decided* section indexes the
   ADR decisions. Follow an ADR link into `docs/03 Architecture/` only when the **rationale**
   matters (CLAUDE.md rule 2). **No design note yet → the ADRs directly**, and say so: a
   load-bearing system planned with neither is the artifact gate failing
   ([[Planning Workflow — Artifact Gate]]), not a detail to work around.
2. If the current sprint is ending, first help me capture a short retrospective
   using `docs/Templates/Retrospective Template.md` into `docs/07 Journal/`. Cover the
   **final week** in it too (`git log --since="7 days ago" --oneline`) — no separate weekly
   review runs this weekend — including the honest sustainability check: did the sprint
   respect the cadence (deep/moderate/light/off)? energy, job+engine+karting balance,
   anything to cut.
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
   - **Priority + weight on every task** — `P1/P2/P3` and a weight (🟢 Deep / 🟠 Moderate
     / 🟡 Light) matching the day-type it fits, so I can pick by the day I'm on. Tag
     format `· P1 · 🟢 Deep`.
6. Create the sprint note from `docs/Templates/Sprint Template.md` and update the roadmap
   and the dashboard — including the Dashboard's **Next ceremony** line, which
   `/weekly-review` would normally roll.
7. **Cut every pulled item out of `docs/06 Sprints/Backlog.md`** — pulling is a *move,
   not a copy* (see that file's header). As each task is written, delete its backlog
   entry; carry any design content it held into the sprint note, design note, or ADR
   first, so nothing is dropped on the floor. Never leave a `✅ Scheduled` marker behind
   — that's the same item in two places, and the copy will drift from the sprint note.
8. **Reset `docs/06 Sprints/Sprint Board.md` for the new sprint** — the board shows *live
   state*, never history. It carries exactly **one** Done column, for the sprint in flight.
   Do this only after the retro (step 2) has mined it:
   - Fold the closing sprint's Done cards into that sprint note's `## Sprint review`
     ("what shipped" + demo/artifact), condensed to a line each. The sprint note is the
     permanent record; the column is not.
   - **Empty** Done, In Progress, and Review / Demo; retitle Done for the new sprint.
   - Rebuild **To Do** from the new sprint note's tasks, tags included (`· P1 · 🟢 Deep`).
   - Backlog column stays a bare pointer to [[Backlog]] — cards never accumulate there.
   Never open a second Done column to keep old cards around. If a card in a closing column
   has no home in a sprint note, that's a gap — say so and place it before cutting.
9. This is planning only — no implementation.
