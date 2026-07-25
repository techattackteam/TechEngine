# 2026-08 · Sprint 02 — Base Foundation

- **Quarter:** [[2026-Q3]]
- **Dates:** **Jul 27 – Aug 30 2026** (5 weeks — one-off length; Sprint 01 was a short
  special sprint, so this absorbs the Jul 27–31 week rather than orphaning it).
  Review + plan on the **Aug 29–30** weekend.
- **Epic:** v2 base foundation
- **Decisions behind it:** [[ADR-006 — v2 core architecture & module layout]] §4 §6 ·
  [[ADR-007 — v2 networking & ECS replication foundation]] §5 · [[Backlog]] Sprint 02 lock (Jul 23)

## 🎯 Sprint goal

> **`base` you can trust:** Logger, Assert and Clock — unit-tested and proven by a real
> consumer, the first sliver of the app loop. **Horizontal base, not the vertical slice.**

The first v2 code that isn't a skeleton. Scope is narrow on purpose: the rest of `base`
(Profiler, memory tracking, Pool, SlotMap, ring buffer) has **no Sprint-02 consumer**, so
building it now would be speculation ([[Planning Workflow — Artifact Gate]]). The C2
vertical slice is **Sprint 03**.

### Scope calls locked at planning (2026-07-25)

| Question | Call | Why |
|---|---|---|
| Loop depth | **Accumulator + `FrameContext`** — no phases, no `Schedule`, no ECS | ADR-007 §5 is already decided; it's the only consumer that genuinely exercises the Clock's tick/frame correlation. Phase stubs without ECS systems would be scaffolding for a consumer that doesn't exist. |
| Window | **Out — headless** | A window pulls in `platform` + GLFW + input + glad2 — that's C2's opening move a sprint early, and none of it is unit-testable. |
| Fixed timestep | **Now** (rides with the accumulator) | Falls out of the loop-depth call. |
| Process work | **In** — `CONVENTIONS.md` + `te-module` skill | Both triggers fired this sprint (first module code / scaffold exists). |

## 🚦 Artifact gate

| Item | ADR? | Design note? | Outcome |
|---|---|---|---|
| **Diagnostics** (Logger + Assert) | ✅ | already exist | Cross-module + hard to reverse — `base`'s public header surface and an engine-wide failure contract. **Heavy → task S2-T1, ordered first.** [[Logger — Design]] / [[Assert — Design]] stay the *living how*; the ADR freezes only the calls. |
| **Clock** | ❌ | ✅ | Local to `base`, reversible, decision already made 2026-07-24. **Light → drafted in this session:** [[Clock — Design]]. |
| **App loop sliver** | ❌ | ✅ exists | [[Game Loop — Frame Flow]] is its note. No new artifact. |
| **`CONVENTIONS.md`** | ❌ | ❌ | It *is* documentation, not a decision. Straight to task. |
| **`te-module` skill** | ❌ | ❌ | Tooling. Straight to task. |

> **One decision home.** ADR-011 will freeze the load-bearing Diagnostics calls; the two
> design notes get *Decided* one-liners + §refs pointing at it — **never copied rationale**
> (or the copies drift, which is exactly what this weekend's review had to clean up).

## Stories & tasks

> Each task: `· P1/P2/P3 · 🟢 Deep / 🟠 Moderate / 🟡 Light`. Pick **weight-fits-day first**,
> then priority ([[Planning Workflow — Artifact Gate]]).

### Story A — Diagnostics decided

- [x] **S2-T1** — Write the Diagnostics ADR (Logger + Assert) via `/adr` · **P1** · 🟢 Deep —
      **✅ Jul 25** → [[ADR-011 — Diagnostics (Logger & Assert)]] **Accepted**. T2–T6 unblocked.
      Two planning assumptions did **not** survive contact: the `fmt`-in-header + spdlog-private
      seam is **unbuildable** (bundled fmt lives inside spdlog's include tree) → seam is
      **`std::format`**, spdlog private, no new dep; and the frame stamp is **pushed by `app`**,
      not pulled from an ambient `Clock`. Also closed 12 parked open questions across both notes.
      done: **ADR-011 Accepted** in [[ADR Index]], freezing: spdlog hidden behind a
      type-erased façade in **one** `.cpp` · channel registration model · `LogRecord` shape +
      sink set · four assert tiers + one hookable handler · no external assert lib · release
      behaviour (no silent `__debugbreak`, F10). [[Logger — Design]] + [[Assert — Design]]
      updated to *index* it. **Blocks T2–T6.**

### Story B — Logger

- [ ] **S2-T2** — Logger core: levels, macros, `source_location`, `do{}while(0)` · **P1** · 🟢 Deep —
      done: `TE_LOGGER_INFO/WARN/...` compile in `base`; `fmt` in the header, **spdlog in one
      `.cpp`** (type-erased `format_args`); `te_base_tests` covers level filtering + positional
      args; format/tidy clean, CI green.
- [ ] **S2-T3** — Channels + `LogRecord` + console/file sinks · **P1** · 🟢 Deep —
      done: self-registering channels tagged by module; structured `LogRecord` reaches console
      + file sinks; two modules log on distinct channels in a test. *(Editor ring-buffer sink
      **excluded** — no editor, no consumer.)*

### Story C — Assert

- [ ] **S2-T4** — Four tiers + single handler · **P1** · 🟢 Deep —
      done: `ASSERT` (debug-only, compiled out) · `VERIFY` (always evaluates, debug abort) ·
      `CHECK` (always-on fatal) · `ENSURE` (always-on non-fatal, report-once); failure path
      `[[unlikely]]`/cold; tests prove VERIFY still evaluates its expression in release and
      ENSURE reports once.
- [ ] **S2-T5** — Assert → Logger integration + flush-on-fail · **P2** · 🟠 Moderate —
      done: failure logs **Critical** through the Logger, flushes, controlled abort; the
      debugger-break-if-attached path is a **documented `platform` hook left unimplemented**
      (no `platform` module this sprint). Test: `ENSURE` logs and continues; `CHECK` aborts.

### Story D — Clock

- [ ] **S2-T6** — Clock implementation · **P1** · 🟠 Moderate —
      done: `now()` / `wallClock()` / `totalTime()` / diagnostic `frame()` per
      [[Clock — Design]]; **no `platform` seam**; tests cover monotonicity + `totalTime`
      accumulation; Logger's `[f N]` stamp reads it.

### Story E — App loop sliver *(the consumer that proves the base)*

- [ ] **S2-T7** — `FrameContext` + fixed-timestep accumulator · **P1** · 🟢 Deep —
      done: `app` runs **headless**; accumulator per ADR-007 §5 with `dt` clamp; integer
      `tick`, `alpha = acc / kFixedDt`; `FrameContext { dt, fixedDt, tick, alpha, frameIndex,
      role }` published per iteration; runs N ticks and exits cleanly.
- [ ] **S2-T8** — Determinism + clamp tests · **P1** · 🟢 Deep —
      done: with an **injected/fake time source**, a fixed `dt` sequence produces an *exact*
      expected tick count; a simulated 2s stall produces clamped catch-up, **not** a spiral of
      death. *(Resolves [[Clock — Design]]'s open testability-seam question — decide seam
      placement here.)*
- [ ] **S2-T9** — End-to-end wire-up = **the sprint demo** · **P2** · 🟠 Moderate —
      done: a headless run emits per-frame log lines carrying the frame stamp + tick, visibly
      correlated; recorded as the Sprint 02 demo artifact.

### Story F — Process & tooling

- [ ] **S2-T10** — Root `CONVENTIONS.md` (B4) · **P2** · 🟠 Moderate —
      done: one root `CONVENTIONS.md` with **judgment** rules only (include order, file
      skeletons, const-correctness, ownership default), linking `.clang-format`/`.clang-tidy`
      for the mechanical subset; CLAUDE.md "Code conventions" shrinks to a pointer.
      **Re-scheduled 2026-07-25 → runs in PARALLEL with T2–T9**, not late. The "needs real
      code" trigger fired on S2-T2, and Miguel flags conventions *as he reviews* — catching
      them live beats reconstructing them at sprint end. File **opened 2026-07-25** with the
      B4 migration + the comments and internal-linkage rules; [[B4 — Code Conventions]] is now
      a pointer + decision history (rules have **one** home). Remaining: fill the *Open* rows
      as they bite, then shrink CLAUDE.md's section — **deferred to sprint end**, since
      shrinking it now would drop rules out of Claude's session context mid-sprint.
- [ ] **S2-T11** — Skill `te-module` scaffolder · **P3** · 🟡 Light —
      done: stamps `include/TechEngine/<m>` + `src` split, `techengine_module()` call,
      colocated Catch2 test exe, deps wiring — referencing the **real** scaffold files.
- [ ] **S2-T12** — Land `feat/improving-vault` through the ruleset · **P3** · 🟡 Light —
      done: PR opened, 8 required checks green, squash-merged. First real exercise of the
      protection rules.

## Definition of Done

- [x] **ADR-011 (Diagnostics) Accepted**; both design notes index it, no copied rationale.
- [ ] **Logger, Assert, Clock** live in `base`, each with Catch2 tests, **CI green both legs**.
- [ ] **Headless app loop** runs a fixed-timestep accumulator publishing `FrameContext`, with
      a **tick-exact** determinism test and a **clamp** test.
- [ ] Demo recorded: correlated frame/tick log output from a headless run.
- [ ] Root `CONVENTIONS.md` exists; CLAUDE.md's conventions section is a pointer.
- [ ] **Nothing built without a Sprint-02 consumer** — the pressure test holds (no Profiler,
      no FrameAllocator, no Pool/SlotMap/ring buffer).

## Capacity note

**This sprint is deliberately under-filled.** 12 tasks across ~5 weeks (≈15 deep slots) is
slack by design, and that is the point: Sprint 01's retro identified **refilling freed time**
as the live burnout risk, not overload.

> **The rule, in writing: finish early → the next deep day stays empty.** Slack is the
> deliverable, not a gap to fill. If the sprint runs genuinely dry, pull *nothing* — bank it,
> and let Sprint 03 (C2, the vertical slice) start rested.

Weight matches day-type — rhythm on the [[Dashboard]] (deep Mon/Thu + one weekend day ·
moderate Fri · light Tue · relaxed Wed · other weekend day off). Weekend days are a
**swappable pair** — nothing here is assigned to Sat or Sun specifically.

**Ordering constraint:** S2-T1 (the ADR) gates T2–T6. Everything else is free.

## Sprint review (fill Aug 29–30)

- What shipped:
- Demo / artifact:

→ Retrospective in [[07 Journal]].
