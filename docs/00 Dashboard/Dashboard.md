# 🎛️ TechEngine Dashboard

> The one page you open first. Keep it short. Update it during the weekly review.

## Now

| | |
|---|---|
| **Quarter** | 2026 Q3 (Jul–Sep) |
| **Sprint** | [[2026-08 Sprint 02 — Base Foundation]] *(Jul 27 – Aug 30)* |
| **Sprint goal** | **`base` you can trust:** Logger, Assert and Clock — unit-tested and proven by a real consumer, the first sliver of the app loop. Horizontal base, **not** the vertical slice. |
| **Current focus** | 🟢 **S2-T1 — the Diagnostics ADR** (`/adr`), which gates the Logger/Assert/Clock work. First v2 code that isn't a skeleton. |
| **Top blocker** | _none_ — watch: CI-minute budget (≈22 billed min/merged change, ADR-008 §9) · clang-tidy unproven on Windows |
| **Next milestone** | `base` foundation done (Aug 30) → **C2 vertical slice** in Sprint 03 (Sep) |
| **Direction** | Fresh start ([[ADR-004 — Fresh start (v2) with v1 as reference]]); v1 = reference prototype |

## 🗓️ Rhythm

**Sprints:** monthly, one headline goal (*may change later*). **Weekly review** once per
**weekend** (`/weekly-review`). **Sprint demo + retro + next-sprint planning** on the
**last weekend of the month** (`/sprint-plan`).
→ **Next ceremony:** **weekend of Aug 1–2 2026** — weekly review.
*(Jul 25–26 weekend fully closed: [[2026-07-25 Weekly Review]] + [[2026-07-25 Sprint 01 Retrospective]]
+ Sprint 02 planned. Next **sprint boundary**: weekend of **Aug 29–30**.)*

**Ceremony anchor = the weekend, not a fixed day.** Run each on whichever weekend day you
work; if you work both, pick one. *Last weekend of the month* = the weekend containing the
month's last Sunday (so a month ending on a Saturday doesn't spawn a stray ceremony).

**Weekly rhythm** — energy is *planned*, not aspirational; protect the light/off days.

| Day | Mode        | Typical work                                             |
| --- | ----------- | -------------------------------------------------------- |
| Mon | 🟢 Deep     | Implementation (core loop)                               |
| Tue | 🟡 Light    | Docs, reading, small fixes, ADR drafting                 |
| Wed | ⚪ Relaxed   | Recovery — optional light planning, else rest            |
| Thu | 🟢 Deep     | Implementation                                           |
| Fri | 🟠 Moderate | Lighter implementation — finish/refactor, prep next week |
| Sat | 🔴 Off*     | Karting — no engine work                                 |
| Sun | 🟢 Deep*    | Implementation + weekly review                           |

*Weekend days are a **pair**, not fixed: default Sat off / Sun deep, but swap them or use
both when there's no karting. Guardrail: keep **≥1 rest day most weekends** — Mon–Fri
already carries 2 deep days.

## 🧭 Plan

Phase 1–3 compressed into Jul 19–26 (audit → plan → ground), each gating the next.
Build order from here: **base → slice → renderer**.

| # | Phase | What | State |
|---|-------|------|-------|
| 1 | **Deep v1 audit** | Read (not skim) each subsystem → deepen [[v1 Code Audit]] | ✅ done (Jul 19) |
| 2 | **Plan v2 + set up AI** | Foundation ADRs 005–008 · AI agents + ceremony loop | ✅ done |
| 3 | **Ground** | Git flow · build scaffold green on CI · `master` ruleset Active | ✅ done (Jul 24) |
| 4 | **Base foundation** | Logger · Assert · Clock · headless fixed-timestep loop — [[2026-08 Sprint 02 — Base Foundation]] | 🔨 active (Aug) |
| 5 | **First vertical slice (C2)** | End-to-end slice on that base; the renderer ADR is born here | ⚪ Sprint 03 (Sep) |

_Tasks → [[Sprint Board]]._

## Quick links

- 📌 [[Roadmap]] · [[2026-Q3]]
- 🏃 [[Sprint Board]] · [[Backlog]]
- 🏛️ [[ADR Index]] · [[v1 Code Audit]] · [[Lessons from v1 (reference prototype)]]
- 🧠 [[Technical Lead Charter]] · [[Working with Claude — Operating Guide]]
- 📓 Journal: [[07 Journal]]

## Active decisions

Recently locked — full set in [[ADR Index]]:

- [x] Fresh start vs continue → **fresh (v2)**, [[ADR-004 — Fresh start (v2) with v1 as reference]]
- [x] v2 stack · architecture · networking · build/testing → ADRs 005–008 Accepted ([[ADR Index]])
- [x] Branching + merge rules → [[ADR-009 — Branching strategy & merge rules]] Accepted; `master` ruleset live
- [ ] **Diagnostics (Logger + Assert) → ADR-011, due S2-T1** — gates all Sprint 02 code
- [ ] User authoring model → [[ADR-010 — User authoring model (Systems & Scripts)]] stays **Proposed**, gated on the task-graph ADR
- [ ] Renderer · job-system · serialization · netcode transport · scripting SDK → deferred, write when coding starts ([[Backlog]])

## Health check (update weekly · 2026-07-25)

- **Build:** 🟢 — CI green both legs, `ctest` 3/3, `master` ruleset Active (8 required checks).
  Caveats: clang-tidy proven on Linux only; CI-minute budget is now live.
- **Momentum:** 🟢 strong — the whole week's plan landed Monday; Thu/Fri went to design
  (ADR-010, [[Game Loop — Frame Flow]], vocabulary amendments) + process.
- **Sustainability:** 🟡 — cadence held (Wed off, Tue light), but early finish was refilled
  instead of banked and Friday ran a deep day's load on a moderate slot. Two sessions past
  midnight. **Rule for Sprint 02: finish early → the day stays empty.**
- **Artifact health:** 🟢 — all four drift findings **reconciled 2026-07-25** (`CLAUDE.md`
  de-v1'd, `04 Design Docs` refs fixed, B4 aligned to the CI format gate, scaffold
  checklist given one home). See [[2026-07-25 Weekly Review]] → *Artifact drift*.
