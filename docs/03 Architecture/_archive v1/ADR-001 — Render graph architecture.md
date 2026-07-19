# ADR-001 — Render graph architecture

- **Status:** Accepted
- **Date:** 2026-07
- **Deciders:** Miguel (Lead Engineer)

## Context

The renderer historically issued passes as an ad-hoc call sequence, making it
hard to reorder, toggle, or reason about the frame. A growing pass count
(shadows, culling, fog, god rays, bloom, AO, auto-exposure, post) needs an
explicit, inspectable structure.

## Decision

Introduce a **render graph**: `RenderGraph` owns an ordered
`std::vector<std::unique_ptr<IRenderPass>>`. Each pass implements `IRenderPass`:

- `name()` — identity for debugging/profiling.
- `setup(RenderResources&)` — one-time allocation of targets/buffers.
- `isEnabled(FrameContext&)` — per-frame toggle.
- `execute(FrameContext&, RenderResources&)` — record the pass.

Passes communicate through a shared `RenderResources` plus a per-frame
`FrameContext` and global `RenderSettings`.

## Consequences

**Positive**
- Passes are isolated, individually testable, and reorderable.
- Frame composition is explicit and greppable (`passes/`).
- Easy to add/remove features (fog, god rays…) as passes.

**Negative / open**
- Ordering and resource lifetimes are **implicit** (insertion order + a shared
  `RenderResources`). There is no declared read/write dependency graph, so the
  system can't auto-order, alias, or validate resource usage yet.
- This is revisited in [[ADR-003 — Renderer direction (rendergraph vs rewrite)]].

## Alternatives considered

- **Keep ad-hoc sequence** — rejected: doesn't scale with pass count.
- **Full dependency-declaring frame graph** (declared resource edges, automatic
  scheduling/aliasing) — deferred: more machinery than currently justified;
  candidate for a later ADR if pain appears.
