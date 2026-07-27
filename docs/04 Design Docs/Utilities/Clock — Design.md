# Clock — Design

> Living design doc. **Status: draft** — drafted in the 2026-07-25 planning session
> (light artifact per [[Planning Workflow — Artifact Gate]]; the decisions were already
> made, this is their hub).
> **ADR = the decision; this doc = the _how_.** No ADR is owed here — the Clock is local
> to `base`, reversible, and its shape was settled in [[Game Loop — Frame Flow]].

**Module:** `base` · **Kind:** utility (helper you *call*) · **Status:** draft
**ADRs:** [[ADR-006 — v2 core architecture & module layout]] §4 §6 ·
[[ADR-007 — v2 networking & ECS replication foundation]] §5
**Consumers:** the app loop (writes) · [[Logger — Design]] / [[Profiler — Design]] (read the frame stamp)
**Sprint:** [[2026-08 Sprint 02 — Base Foundation]] — S2-T6

## Purpose

The engine's **time source** — and deliberately nothing more. The hard part of this
design is what the Clock *refuses* to own.

## Decided

| Fact | Where |
|---|---|
| Read-only time facade in `EngineContext` as `const Clock&` — loop writes, systems read | ADR-006 §4 |
| Lives in **`base`**, no `platform` seam — `steady_clock` is std and QPC-backed | [[Backlog]] (add a raw platform timer only if the Profiler measures a need) |
| Owns: monotonic `now()`, wall-clock stamp, `totalTime`, **diagnostic** frame counter | [[Game Loop — Frame Flow]] (2026-07-24) |
| Does **not** own `dt` / `fixedDt` / `tick` / `alpha` / `role` — those live on `FrameContext` | [[Game Loop — Frame Flow]] (2026-07-24), ADR-007 §5 |
| Monotonic for **durations**; wall-clock **only** for stamps | this note (local call; no ADR owes it) |
| The frame stamp is **pushed by `app`** into diagnostics — `base` holds no `Clock` reference | [[ADR-011 — Diagnostics (Logger & Assert)]] §9 |
| `timeScale` / pause / slow-mo is **loop policy**, not a Clock knob | [[Game Loop — Frame Flow]] |

## Design

### Why sim time is not here

A process can host **more than one sim** — the editor hosts a client *and* a server (v1's
F1 trigger), and tests run several headless sims. A process-wide Clock can hold exactly one
`tick`/`alpha`, and `role` is meaningless as a global. `FrameContext` is per-call, so each
sim carries its own. Full rationale: [[Game Loop — Frame Flow]] → *Where time lives*.

### The frame counter is correlation-only

[[Logger — Design]]'s `[f 1043]` stamp and the Profiler are **global macros** and cannot take a
`FrameContext` — they need an ambient number. So the Clock keeps one, and it is **approximate when two
sims share a process**. It is never the simulation's source of truth; anything that must be exact reads
`FrameContext.tick`.

**The Clock owns the counter; it does not hand itself to the Logger.** `app` reads `frame()` and
**pushes** the value into diagnostics once per frame — an ambient global `Clock*` read from a log macro
would be a second access path to an `EngineContext` service, which ADR-006 §4 exists to remove.
Decided in [[ADR-011 — Diagnostics (Logger & Assert)]] §9.

### Surface (shape, not a spec — impl decides the details)

| Call | Returns | For |
|---|---|---|
| `now()` | monotonic `TimePoint` | durations, the loop's `dt` |
| `totalTime()` | seconds since start | ambient elapsed |
| `wallClock()` | `system_clock` stamp | log timestamps only |
| `frame()` | diagnostic counter | Logger/Profiler correlation |

The loop is the only writer: it computes `dt` from `now()` and bumps `frame()` once per
frame. Everything else takes `const Clock&`.

## Open questions (→ resolve during S2-T7 / S2-T9)

- **Testability seam.** The loop's determinism test (S2-T9) needs to drive a *fake* time
  sequence — a real `steady_clock` can't produce an exact tick count on demand.
  Two shapes: (a) keep `Clock` concrete and let the **loop** take its `dt` from an injected
  source, or (b) put a seam inside `Clock`. **Leaning (a)** — it keeps `base`'s simplest
  utility free of virtuals and puts the seam where the test actually needs it. Decide when
  writing the loop, not before.
- **Profiler-grade resolution.** Whether `steady_clock` is precise enough, or a raw
  platform timer is needed — **measure first** (CLAUDE.md perf rule), and there is no
  profiler yet to measure with. Revisit when the Profiler lands (Sprint 03+).

## References

- [[Game Loop — Frame Flow]] — where sim time lives, and why not here
- [[Logger — Design]] — the `[f N]` stamp consumer
- Code: *(none yet — lands as S2-T7)*
