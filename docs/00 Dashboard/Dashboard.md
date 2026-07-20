# 🎛️ TechEngine Dashboard

> The one page you open first. Keep it short. Update it during the weekly review.

## Now

| | |
|---|---|
| **Quarter** | 2026 Q3 (Jul–Sep) |
| **Sprint** | [[2026-07 Sprint 01 — Foundation Planning (v2)]] |
| **Sprint goal** | Plan the fresh engine (v2) on paper: mine v1 for lessons, decide architecture + what to port, set up git flow, define the first slice. No v2 code until the plan exists. |
| **Current focus** | 🔨 **Build scaffold** — first v2 skeleton compiling green on CI ([[ADR-008 — v2 build & testing baseline]] checklist). Foundation ADRs 005–008 Accepted; `v1-reference` tagged, `v2` mainline. |
| **Top blocker** | _none logged_ |
| **Next milestone** | v2 skeleton builds green on CI → Sprint 02 planned (Sun 26 Jul) |
| **Direction** | Fresh start ([[ADR-004 — Fresh start (v2) with v1 as reference]]); v1 = reference prototype |

## 🗓️ Rhythm

**Sprints:** monthly, one headline goal (*may change later*). **Weekly review** every
**Sunday** (`/weekly-review`). **Sprint demo + retro + next-sprint planning** on the
**last Sunday of the month** (`/sprint-plan`).
→ **Next ceremony:** **Sun 26 Jul 2026** — Sprint 01 demo + retro → plan Sprint 02 (Aug).

**Weekly rhythm** — energy is *planned*, not aspirational; protect the light/off days.

| Day | Mode      | Typical work                                  |
| --- | --------- | --------------------------------------------- |
| Mon | 🟢 Deep   | Implementation (core loop)                    |
| Tue | 🟡 Light  | Docs, reading, small fixes, ADR drafting      |
| Wed | ⚪ Relaxed | Recovery — optional light planning, else rest |
| Thu | 🟢 Deep   | Implementation                                |
| Fri | 🟠 Moderate | Lighter implementation — finish/refactor, prep next week |
| Sat | 🔴 Off    | Karting — no engine work                      |
| Sun | 🟢 Deep   | Implementation + weekly review                |

## 🧭 Plan (collapsed — audit-first)

Timeline compressed into the Jul 19–26 window: audit → plan → ground, back-to-back.
Each phase gated the next.

| # | Phase | What | State |
|---|-------|------|-------|
| 1 | **Deep v1 audit** | Read (not skim) each subsystem → deepen [[v1 Code Audit]] | ✅ done (Jul 19) |
| 2 | **Plan v2 + set up AI** | Foundation ADRs 005–008 (stack, architecture, networking, build/test) · AI agents (renderer + conventions → [[Backlog]]) | ✅ done |
| 3 | **Ground** | Git flow ✅ · build scaffold (green on CI) · first slice + draft Sprint 02 (Jul 26) | 🔨 active |

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
- [ ] Renderer · job-system · serialization · netcode transport · scripting SDK → deferred, write when coding starts ([[Backlog]])

## Health check (update weekly)

- Build: 🟢 / 🟡 / 🔴 — _note_
- Momentum: _one line on how the week felt_
- Sustainability: _energy / burnout watch (job + engine + karting)_
