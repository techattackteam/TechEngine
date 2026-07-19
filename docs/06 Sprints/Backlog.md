# 🗃️ Backlog

Everything wanted but not in the active sprint. Groom at sprint planning. Pull
from here into [[Sprint Board]]. Group by epic.

> 🧹 **Reset for the fresh start (Jul 19).** v1-codebase work (finish
> render-graph migration, editor stability pass, template-contract cleanup, etc.)
> is **dropped** — v1 is a frozen reference ([[ADR-004 — Fresh start (v2) with v1 as reference]]),
> we don't do feature work on it. Everything below is **parked until v2 is
> designed** (i.e. after the deep audit + foundation ADRs). Don't pull any of it
> into an active sprint before then.

## Parked for v2 — revisit after audit + design

### Rendering (Q4 direction — needs v2 renderer ADR first)
- Atmospheric scattering (sky LUT, aerial perspective)
- Volumetric fog (froxel scattering)
- Volumetric shadows / god rays through froxel path
- Per-pass GPU timing + frame profiler overlay
- Auto-exposure + bloom

### Core / ECS
- System scheduling/ordering — decide + document
- Resource hot-reload / eviction policy (candidate ADR)

### Tooling / Editor
- Frame capture / debug-visualization tools

### Process / Infra
- README at repo root (public-facing)
- CI: build + basic smoke test
- Recorded-demo workflow (capture + store)

## Ideas (unsorted)
- _drop raw ideas here; triage later_
