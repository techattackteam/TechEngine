# 2026-08 · Sprint 02 — Base Foundation

- **Quarter:** [[2026-Q3]]
- **Dates:** **Jul 25 – Aug 28 2026** (5 weeks — **one-off transition**: the 4-week Sat→Fri
  cadence was adopted 2026-07-26 mid-sprint, and this sprint was already sized at ~5 weeks.
  Cutting it to 4 would have removed the deliberate slack, not idle time. Sprint 03 is the
  first clean cycle). Review + plan on the **Aug 29–30** weekend — which is **day 1 of
  Sprint 03**, not the tail of this one.
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

- [x] **S2-T2** — Logger core: levels, macros, `source_location`, `do{}while(0)` · **P1** · 🟢 Deep —
      **✅ Jul 25** → PR #8 (`6f054b6b`), required checks green. Shipped: `TE_LOGGER_*` over the
      **`std::format` seam**, spdlog private to `Log.cpp`, `spdlog::spdlog` → `LIBS_PRIVATE`
      (ADR-011 §1 — the `:4` visibility breach the ADR flagged is closed); compile-time gate wired
      **per config** via genex (Debug 0 · RelWithDebInfo 1 · Release 2) + `TE_LOG_ACTIVE_LEVEL`
      cache override (§4); 9 Catch2 cases; `.clang-format` aligned to the CLion scheme so IDE == CI.
      **ADR-011's `std::format` exit trigger did not fire** — the Linux/Clang legs merged green, so
      the standalone-`fmt` fallback stays parked.
      Carried into T3: default-sink path untested · `LogRecord` has no `time` field (§3) · rendered
      line is still `[f-N][file:line:func()]`, not §3's. Found in flight → [[Backlog]]: `SYSTEM`
      third-party includes · coverage in CI · gate fails open · diagnostics init belongs in `app`.
- [x] **S2-T3** — Channels + `LogRecord` + console/file sinks · **P1** · 🟢 Deep —
      **✅ Jul 25** → PR #9. Shipped: module/channel **handles** in fixed tables, registered
      explicitly from the composition root (ADR-011 §2); filtering at
      `max(process, module, channel)`; `LogRecord` gains `time` + module tag; a **sink array**
      (`addLogSink`/`removeLogSink`) replacing the single slot; one spdlog logger over console +
      rotating file (`logs/techengine.log`, 5 MB × 3) that degrades to console-only if the file
      won't open; the [[Logger — Design]] line, flattened **once** per record and shared by the
      pre-init stderr fallback. Call site picks its channel via a per-TU `TE_LOG_CHANNEL` +
      `_CH` escape.
      **Test-reachability criterion met** (the one T2's green-but-unreached bug earned):
      `flattenRecord` sits behind a `src/`-internal header the test target includes —
      `techengine_test()` now puts every module's `src/` on its test include path — and the
      capture sink **adds** instead of replacing, so no path exists that only runs when the
      default sink is installed. 9 new cases, incl. the stderr fallback (fd save/restore) and a
      buffer canary. *(Ring sink → T5; editor sink still excluded.)*
      **Residual:** `initLogging`/`spdlogSink` stay uncovered by ctest — the suite never calls
      `initLogging()` so it writes no log files; **T9's demo is what proves them.**

### Story C — Assert

- [ ] **S2-T4** — Four tiers + single handler · **P1** · 🟢 Deep —
      done: `ASSERT` (debug-only, compiled out) · `VERIFY` (always evaluates, debug abort) ·
      `CHECK` (always-on fatal) · `ENSURE` (always-on non-fatal, report-once); failure path
      `[[unlikely]]`/cold; tests prove VERIFY still evaluates its expression in release and
      ENSURE reports once.
- [ ] **S2-T5** — Assert → Logger integration + flush-on-fail · **P2** · 🟠 Moderate —
      **+ owns the in-memory ring sink** (moved from T3, 2026-07-25: this is the task whose flush
      path consumes it — ADR-011 §3).
      done: failure logs **Critical** through the Logger, flushes, controlled abort; the
      debugger-break-if-attached path is a **documented `platform` hook left unimplemented**
      (no `platform` module this sprint). Test: `ENSURE` logs and continues; `CHECK` aborts.

### Story D — Clock

- [x] **S2-T6** — Clock implementation · **P1** · 🟠 Moderate —
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
- [x] **S2-T12** — Land the accumulated vault + AI-config work through the ruleset ·
      **P3** · 🟡 Light — done: PR opened, 8 required checks green, squash-merged. First real
      exercise of the protection rules. *(Written as "land `feat/improving-vault`"; that
      branch is gone and the work accumulated uncommitted on `master` instead — mechanism
      changed, goal unchanged.)*

**Vault repo split — added mid-sprint 2026-07-27** on
[[ADR-012 — Vault repository split]] (Accepted). Urgent: the pain is *per-merge* and
*per-overlap*, so every day it waits costs another orphaned board edit or a board conflict.
T13 blocks T14 and T15.

- [ ] **S2-T13** — Vault repo cutover · **P1** · 🟠 Moderate —
      done: `docs/` is its own private GitHub repo **with its history preserved** (subtree
      split / `filter-repo`, *not* a fresh `init`), removed from the engine index;
      `.gitignore` gains `docs/` and a root `.ignore` gains `!docs/` **in the same commit**
      (ADR-012 §1 — split them and vault search silently dies); verified both ways —
      `git check-ignore` reports `docs/` ignored *and* a repo-root ripgrep search returns
      vault hits; `CLAUDE.md` + root README document the two-repo clone so a fresh machine
      works. **Atomic:** do not leave `docs/` both tracked and separately-repo'd.
- [ ] **S2-T14** — Retarget the vault-writing commands · **P2** · 🟡 Light —
      done: every `.claude/commands/*` that commits vault files targets the nested repo
      (`git -C docs …`) — `/task-start`, `/task-wrap`, `/sprint-plan`, `/weekly-review`,
      `/vault-clean`; no command attempts a `docs/` commit from the engine repo; a
      `/task-start` board move no longer appears in an engine PR.
- [ ] **S2-T15** — Reconciliation stamp · **P2** · 🟡 Light —
      done: [[Dashboard]] carries `**Reconciled against:** engine <sha> (YYYY-MM-DD)`;
      `/weekly-review` and `/sprint-plan` advance it **only after** the drift check has
      actually run (a formality stamp is worse than none — ADR-012 §6); CLAUDE.md rule 2
      says to compare it against `origin/master` and treat design notes as **suspect** while
      the engine is ahead.

## Definition of Done

- [x] **ADR-011 (Diagnostics) Accepted**; both design notes index it, no copied rationale.
- [ ] **Vault split done (ADR-012)** — `docs/` its own repo, board edits no longer touch engine
      PRs, reconciliation stamp live. *Added mid-sprint 2026-07-27; see the capacity note.*
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

**Ordering constraint:** S2-T1 (the ADR) gates T2–T6. T13 gates T14/T15. Everything else is free.

### Mid-sprint scope change — 2026-07-27

**12 → 15 tasks** (T13/T14/T15, the ADR-012 vault split), added on day 3. Recorded rather
than absorbed silently, because the note above forbids refilling freed time.

**This is not that case** — the rule targets *refilling slack when the sprint runs dry*; this
is new work arriving with a real trigger. The distinction matters, so: **zero 🟢 Deep tasks
were added.** The added weights are 🟠 + 🟡 + 🟡, which draw on moderate/light capacity (Fri /
Tue), while the protected resource — deep slots for T4/T7/T8 — is untouched. That is why T13
is **P1 but Moderate**: prioritised without displacing the sprint goal.

**Still a net increase, and the trade is on the table:** if light capacity gets tight, push
**S2-T11** (`te-module`, P3 🟡) to Sprint 03. It is the same tooling category, the lowest
priority in the sprint, and it has no consumer waiting. Take that trade before letting
anything touch the Deep slots.

## Sprint review (fill Aug 29–30)

- What shipped:
- Demo / artifact:

→ Retrospective in [[07 Journal]].
