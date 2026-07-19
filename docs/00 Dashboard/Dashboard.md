# 🎛️ TechEngine Dashboard

> The one page you open first. Keep it short. Update it during the weekly review.

## Now

| | |
|---|---|
| **Quarter** | 2026 Q3 (Jul–Sep) |
| **Sprint** | [[2026-07 Sprint 01 — Foundation Planning (v2)]] |
| **Sprint goal** | Plan the fresh engine (v2) on paper: mine v1 for lessons, decide architecture + what to port, set up git flow, define the first slice. No v2 code until the plan exists. |
| **Current focus** | ✅ **Deep v1 audit done (Jul 19)** — [[v1 Code Audit]] F1–F35 + verdicts. Next: Miguel reviews, then B1 (v2 module-layout ADR). |
| **Top blocker** | _none logged_ |
| **Next milestone** | v2 foundation ADRs Accepted + `v1-reference` tagged |
| **Direction** | Fresh start ([[ADR-004 — Fresh start (v2) with v1 as reference]]); v1 = reference prototype |

## 🧭 Plan (collapsed — audit-first)

Timeline compressed: the audit moves up to **today** so planning can start right
after. Each phase gates the next — nothing downstream is filled in ahead of it.

| # | Phase | What | State |
|---|-------|------|-------|
| 1 | **Deep v1 audit** | Read (not skim) each subsystem → deepen [[v1 Code Audit]] | ✅ done (Jul 19) |
| 2 | **Plan v2 + set up AI** | Foundation ADRs (module layout, renderer, build/test) · code conventions · agents | 🔨 next |
| 3 | **Ground** | Git flow (tag `v1-reference`, start `v2`) · first buildable slice · draft Sprint 02 | ⏳ after 2 |

_Tasks → [[Sprint Board]]. Nothing in phase 2/3 is decided until phase 1 lands._

## Quick links

- 📌 [[Roadmap]] · [[2026-Q3]]
- 🏃 [[Sprint Board]] · [[Backlog]]
- 🏛️ [[ADR Index]] · [[v1 Code Audit]] · [[Lessons from v1 (reference prototype)]]
- 🧠 [[Technical Lead Charter]] · [[Prompt Library]] · [[Working with Claude — Operating Guide]]
- 📓 Latest review: [[2026-07-19 Weekly Review]]

## Active decisions in flight

- [x] Fresh start vs continue in place → **fresh (v2)**, [[ADR-004 — Fresh start (v2) with v1 as reference]]
- [ ] v2 module layout & core architecture → ADR (_after audit_)
- [ ] v2 renderer approach → ADR (_after audit_)
- [ ] v2 build + testing baseline (presets, test framework) → ADR (_after audit_)

## Health check (update weekly)

- Build: 🟢 / 🟡 / 🔴 — _note_
- Momentum: _one line on how the week felt_
- Sustainability: _energy / burnout watch (job + engine + karting)_
