# 🗺️ Roadmap

High-level and slow-moving. Quarters set direction; [[Sprint Board|sprints]]
execute. Review at each month's last-weekend planning session.

## Cadence

- **Quarter** → large milestones (below).
- **Month** → one sprint with a single headline goal.
- **Week** → review on the weekend; deep implementation Mon/Thu + one weekend day, moderate Fri (see Dashboard rhythm).
- **Last weekend of month** → sprint review + retro + next-sprint planning.

## Quarters

### 2026 Q3 (Jul–Sep) — *Foundation & Direction* → [[2026-Q3]]
- ✅ Direction locked: **fresh start (v2)** — [[ADR-004 — Fresh start (v2) with v1 as reference]].
- Deep-audit v1, design the v2 foundation (ADRs), stand up a first buildable slice.
- Establish the process itself (this vault, ADRs, sprint rhythm).

> 🚧 **Q4 / Q1 below are gated on v2 being designed** (foundation ADRs + first
> slice). They set aspirational direction only — no concrete features are
> committed until the v2 renderer/architecture ADRs land. Revisit at Q3-end planning.

### 2026 Q4 (Oct–Dec) — *Signature Rendering* (tentative, gated on v2 design)
- Atmospheric scattering (sky LUTs, aerial perspective).
- Volumetric fog.
- Volumetric shadows / god rays through the froxel path.
- First real demo scene + documentation pass.

### 2027 Q1 — *Polish & Prove* (tentative, gated on v2 design)
- Editor/tooling hardening around the new renderer.
- Performance pass, profiling, capture tools.
- A demo that stands on its own.

## Milestone list

| Milestone | Target | Status |
|-----------|--------|--------|
| Vault + process live | Jul 2026 | 🟢 done — full ceremony loop run Jul 25 |
| Rewrite/refactor decision locked | Jul 2026 | 🟢 done (ADR-004) |
| Deep v1 audit complete | Jul 2026 | 🟢 done |
| v2 foundation ADRs Accepted | Jul 2026 | 🟢 done |
| v2 build scaffold green on CI | Jul 2026 | 🟢 done |
| `base` foundation (Logger/Assert/Clock + loop) | Aug 2026 | 🟡 Sprint 02 |
| First v2 slice builds + runs | Q3 2026 (Sep — Sprint 03) | ⚪ not started |
| Atmospheric scattering | Q4 2026 (gated) | ⚪ |
| First demo | Q4 2026 / Q1 2027 | ⚪ |

_Legend: 🟢 done · 🟡 in progress · ⚪ not started · 🔴 blocked_
