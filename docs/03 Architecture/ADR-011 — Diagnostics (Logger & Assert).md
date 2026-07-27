# ADR-011 — Diagnostics (Logger & Assert)

- **Status:** Accepted
- **Date:** 2026-07 (Accepted 2026-07-25)
- **Deciders:** Miguel (Lead Engineer), with AI as technical lead
- **Related:** [[ADR-005 — v2 tech stack & toolchain]] (spdlog) ·
  [[ADR-006 — v2 core architecture & module layout]] §4 §5 §6 ·
  [[ADR-008 — v2 build & testing baseline]] §7 §8 · living *how*:
  [[Logger — Design]] · [[Assert — Design]]
- **Task:** S2-T1 ([[2026-08 Sprint 02 — Base Foundation]]) — **gates S2-T2…T6.**
- **Amended 2026-07-27:** §7's no-recursion guard now aborts **only if the nested failure's
  own tier is fatal**, was "and aborts" unconditionally. No decision reversed — fatality was
  always a property of the tier (§5), and the original wording made a `TE_ENSURE` fatal purely
  by firing inside a handler, which contradicts its "log + continue" definition. Found while
  implementing S2-T4: the guard's test could not raise a nested failure without killing the
  runner.
- **Supersedes:** **ADR-006 §6's assert-tier clause only** — the `TE_VERIFY`
  semantics in "`TE_CHECK/TE_VERIFY` always-on for shipped invariants" (ADR-006
  `:254-255`). ADR-006 §6's **logging bullet**, its **`TE_ASSERT`** semantics, the
  **never-silent-`__debugbreak`** rule and the `[[unlikely]]`/cold failure path all
  **remain in force** and are restated here unchanged. ADR-006 otherwise stands
  Accepted and untouched — no edit is made to its body.

## Context

Sprint 02 writes the first v2 code that isn't a skeleton, and Diagnostics is ordered
first because it is the one piece every later module compiles against. Two things make
it ADR-worthy rather than a design-note detail:

1. **It is `base`'s public header surface.** Every module, every test, and eventually
   the script SDK includes it. Reshaping it after N modules exist is the churn ADR-008
   was written to avoid — and the *formatting backend* leaks into that surface unless a
   seam is chosen deliberately.
2. **It is an engine-wide failure contract.** Which assert tier survives shipping, and
   what happens when one fires, cannot be decided per-call-site later.

The design notes ([[Logger — Design]], [[Assert — Design]]) carry the *how* and are
already detailed. What they carry that this ADR must resolve is a set of **open
questions** parked for exactly this moment, plus one **contradiction with an Accepted
ADR** (the assert tiers), which is why this is a superseding record and not an
amendment.

**Grounding — three facts checked against the tree, not assumed:**

- `engine/base/CMakeLists.txt:4` links `spdlog::spdlog` as **PUBLIC** today. Nothing in
  `base`'s public header uses it (`include/TechEngine/base/Base.hpp` is a stub), so this
  is already out of compliance with ADR-008 §8's visibility rule.
- **spdlog's bundled fmt is not separately reachable.** The pinned spdlog v1.15.1
  (`cmake/deps.cmake:29-32`) ships fmt **11.1.3** inside its own include tree at
  `include/spdlog/fmt/bundled/`. So `fmt` in a public `base` header **forces spdlog's
  include directory PUBLIC** — "fmt in the header, spdlog private" is not achievable as
  a pair without changing how fmt is obtained. This killed the seam as originally
  sketched in [[Logger — Design]].
- spdlog v1.15.1 offers three mutually-exclusive formatting backends —
  bundled fmt (default), `SPDLOG_FMT_EXTERNAL`, `SPDLOG_USE_STD_FORMAT`
  (`spdlog/CMakeLists.txt:91-104`) — which is what opens §1's third option.

## Decision

### 1. The seam — `std::format` in the header, spdlog in exactly one `.cpp`

Public `base/log.hpp` exposes the macros, `Level`, `LogChannel` and **`std::format`** —
**no third-party header at all**. The call site type-erases its arguments into a
`std::format_args`; a **non-template** `logDispatch(...)`, defined in `log.cpp` — the
only TU in the engine that sees spdlog — formats into a stack buffer and routes the
record to the sinks. spdlog receives a **pre-formatted string** and acts purely as a
sink/router, so its own formatting backend is never on our public path and stays at its
default bundled fmt (**no `deps.cmake` change, no second dep, no version-compat
matrix**).

- **`spdlog::spdlog` becomes `LIBS_PRIVATE`** on `te_base` — correcting
  `engine/base/CMakeLists.txt:4` per ADR-008 §8. `glm::glm` stays PUBLIC (math *is* in
  the public surface).
- **Compile-time-checked format strings are preserved.** `std::format_string<Args...>`
  in the macro's template makes a bad format string a **compile error**, not a runtime
  throw.
- **Positional args `{0} {1}`** ([[Logger — Design]]); never mixed with bare `{}`.
- **Formatting allocates nothing on the happy path** — `std::vformat_to` into a
  fixed stack buffer, overflow truncates with a marker.

**Verified, not assumed** (MSVC 14.38, `/std:c++20 /W4 /WX`): the full seam — positional
args, type erasure across a non-template boundary, stack-buffer formatting — compiles
and runs clean, and `"{0}x{1} {9}"` with two arguments **fails to compile**
(`_You_see_this_error_because_arg_id_is_out_of_range`). The Linux/Clang leg is **not yet
verified** — see *Consequences* and *What would move this decision*.

Why this over putting `fmt` in the header (the [[Logger — Design]] sketch): it needs no
new dependency, and it leaves **zero third-party types in `base`'s public surface**,
which is the property that keeps the future SDK question open instead of pre-deciding it
(§8). Cost: `std::format` on C++20 is a smaller feature set than fmt (no `fmt::join`, no
ranges formatting until C++23) and carries real toolchain risk on the portability leg.
Both are accepted with a named exit — see *What would move this decision*.

### 2. Channels — module-owned, explicitly registered, no enum in `base`

`base` owns the *mechanism*, never the channel list — a `base` enum of
`render`/`net`/… would couple the leaf upward (F3/F12) and would mean **adding a module
requires editing `base`**, breaking the "no reshape" promise ADR-006 `:305-309` makes for
the reserved `net`/`server` slots.

- `registerChannel(name, moduleTag, defaultLevel)` → a **`LogChannel` handle (small
  int)**. No per-call string hashing; the per-channel runtime level is an **array index**.
- **The module tag is itself a registered handle**, symmetric with channels — *not* a
  `Module::Client` enum. This is the correction to [[Logger — Design]]'s sketch, which
  rejected an upward enum for channel names but then reintroduced one for module tags.
- **Registration is explicit and invoked from the composition root**, not a file-scope
  static initializer. Two reasons, both concrete: (a) everything is a **static lib**
  (ADR-006 §1, `cmake/techengine_module.cmake:38` — plain `add_library(... STATIC ...)`),
  so a TU whose only purpose is a file-scope registration **is stripped by the linker**
  unless force-included (`/WHOLEARCHIVE`, `--whole-archive`); (b) file-scope registration
  is static-init-order-dependent, which cuts against ADR-006 §4's single wiring point in
  `app`. Each module exposes its own registration entry point; `app` calls it.
- **Logging before registration is legal** — an unknown channel falls back to a default
  channel, and logging before the Logger is initialized falls back to **stderr** (the
  same fallback §7 gives Assert).

### 3. `LogRecord` + sink set

Sinks receive a **structured record**, never a pre-baked string, so a consumer can filter
on fields instead of re-parsing:

`LogRecord { time, frame, level, channel (+ module tag), file, function, line, message }`

- **Sinks this sprint:** console · rotating file · an **in-memory ring of the last N
  records** for crash flush. File/console flatten a record to a line; the ring keeps the
  struct.
- **Rendered line format** ([[Logger — Design]]):
  `[14:32:07.412][f 1043][client · render][renderer.cpp:88 renderScene][INFO] …`
- **The file sink is synchronous.** Async is a later change *behind the façade* — which
  is precisely what §1 buys — and nothing this sprint needs it.
- **Editor ring-buffer sink is excluded** — no editor, no consumer
  ([[Planning Workflow — Artifact Gate]]). [[Logger — Design]] is pruned to match.
- **The crash handler and minidump live in `platform`, not in a sink.** This ADR lands
  only the ring. It does **not** claim symbolicated shipped crash dumps exist: ADR-008 §3
  gives `runtime` Debug + Release only and pre-authorizes RelWithDebInfo "later" for
  exactly that. Symbolication is a later ADR-008 question.

### 4. Compile-time level gate

| Config                       | Trace   | Debug   | Info | Warn | Error | Critical |
| ---------------------------- | ------- | ------- | ---- | ---- | ----- | -------- |
| Debug                        | on      | on      | on   | on   | on    | on       |
| RelWithDebInfo (dev runtime) | **off** | on      | on   | on   | on    | on       |
| Release / Shipping           | **off** | **off** | on   | on   | on    | on       |

A single compile-time minimum level per config, overridable by a cache var. Compiled-out
levels evaluate **none** of their arguments. Level *semantics* (which level to reach for)
stay in [[Logger — Design]]'s table — that's guidance, not a frozen decision, and may
promote to root `CONVENTIONS.md` (S2-T10).

### 5. Assert — four tiers, one hookable handler

**This is the clause that supersedes ADR-006 §6.** Two axes: is `cond` evaluated in
shipping, and does failure abort in shipping.

| Macro | Eval (ship) | Abort (ship) | Use when… |
|---|---|---|---|
| `TE_ASSERT` | ❌ | ❌ | dev-only correctness check, droppable, hot path |
| `TE_VERIFY` | ✅ | ❌ (dev-only abort) | as ASSERT, but `cond` has a **side effect to keep**, or you branch on the result |
| `TE_CHECK` | ✅ | ✅ **fatal** | continuing is unsafe / UB in **any** build |
| `TE_ENSURE` | ✅ | ❌ **non-fatal** — log + continue, **report-once**; returns `bool` | recoverable-but-wrong; degrade gracefully |

- **What changed vs ADR-006 §6:** §6 said two-tier, with `TE_CHECK`/`TE_VERIFY` both
  "always-on for shipped invariants". `TE_CHECK` is preserved and sharpened (always-on,
  **fatal**); **`TE_VERIFY` is reversed** to always-*evaluate* with a **dev-only abort**;
  `TE_ENSURE` is net-new (no §6 counterpart). `TE_ASSERT` is unchanged. The two-tier model
  conflated "does the condition still run?" with "does failure still kill the process?" —
  they are independent questions, and every real use site needs one of the four
  combinations, not two.
- **`TE_ASSERT` is on in RelWithDebInfo** (closing [[Assert — Design]]'s open question):
  RelWithDebInfo is the *dev runtime* config whose entire purpose is debuggability, and
  ADR-008 §3 additionally makes it the config that hosts debuggable script DLLs.
- **`TE_ENSURE` report-once is per-call-site**, via a function-local static — no global
  rate-limiter, no allocation, no shared state.
- **Failure path is `[[unlikely]]`/cold**; the happy path pays nothing (restates §6).
- **One installable handler.** `setAssertHandler(h)` **returns the previous handler**, so
  tests scope-swap a throw/flag policy (ADR-008 §6) and `platform`/`app` install a
  debugger-aware one — the *same* seam. Install at composition-root time; it is **not**
  safe to install concurrently with a firing assert.
- **No external assert library** — macros + `std::source_location` + compiler intrinsics
  only.

### 6. Fatal path, and never a silent break

On a fatal failure: **log Critical through the Logger → flush all sinks → controlled
abort.** **Never a silent `__debugbreak()` in release** (F10; restates ADR-006 §6).

- `__debugbreak()` / `__builtin_trap()` are **compiler intrinsics, not OS calls** → legal
  in `base`. But *"is a debugger attached?"* (`IsDebuggerPresent`) is an **OS** call →
  **not** legal in `base` (ADR-005's no-OS-headers rule). So: **`base` ships the default
  handler** (log + flush + abort, no break); **`platform`/`app` install** the
  debugger-aware one.
- The platform handler's OS include lives in a platform **`.cpp`** (PRIVATE per ADR-006
  §3), never a platform public header, **and the Linux/Clang leg gets an equivalent or an
  explicit no-op** — that leg is ADR-005's OS-independence validator and a required check
  (ADR-008 §9), so a Windows-only handler that fails to compile there is a red gate.

### 7. Assert → Logger: one direction, guarded

- **Assert depends on the Logger; never the reverse.** The Logger's own internals must not
  use an assert macro that routes back through the Logger.
- **No-recursion guard** — a `thread_local` in-flight flag; a failure raised while
  already reporting goes straight to stderr, **skips the handler entirely, and then aborts
  if its own tier is fatal**. Fatality stays a property of the tier, not of where the
  failure happened — a nested `TE_ENSURE` reports and continues, exactly as an unnested one
  would. (Amended 2026-07-27; originally "and aborts", which made a non-fatal tier fatal
  purely by firing inside a handler.)
- **Pre-init fallback** — an assert that fires before the Logger is initialized writes to
  **stderr** (same fallback as §2).

### 8. Diagnostics state is process-global by design — and is not a service locator

The channel table, the per-channel level array, the installed handler and the ring sink
are **mutable process-global state in a `base` "utility"**. ADR-006 §5 defines *utility*
as "stateless/global" while still placing the Logger there (`:223`, `:233`) — the stated
rationale being safety under static linkage (F4), not statelessness. This ADR resolves
that tension explicitly rather than letting it drift:

**Diagnostics state is process-global, and that is the decision.** It is *not* the F5
service locator ADR-006 §4 exists to kill, on three specific grounds:
- it **stores no systems and no services** — §4's named failure mode is "a system
  reaching a *sibling system* through it";
- it **vends handles (small ints), never pointers** to a subsystem — there is no
  `getSystem<T>()` runtime-throw shape;
- it **hides no dependency edge** — every module already links `base` unconditionally, so
  nothing becomes less greppable.

**Guardrail:** if diagnostics state ever grows a field that is a *service or system*, that
is the regression to reject in review — it stops being diagnostics and becomes the
locator.

### 9. Frame stamp — pushed by `app`, not pulled from an ambient Clock

The `[f 1043]` stamp needs a frame number inside a **global macro**, which cannot take a
`FrameContext`. Two shapes were available; this ADR takes the **push**:

**`app` publishes the frame number into diagnostics once per frame** (a relaxed atomic
integer); `base` holds a plain counter and **holds no reference to the Clock**.

Rejected: a **pull**, where the macro reads a process-global `Clock*`. ADR-006 §4 puts
`const Clock&` in `EngineContext` with `app` as the single wiring point; an ambient global
`Clock*` creates a **second access path to an `EngineContext` service** — the exact shape
§4 exists to remove. Push keeps `base` Clock-free and leaves §4 untouched.

The `Clock` still **owns** the diagnostic counter ([[Clock — Design]] ·
[[Game Loop — Frame Flow]] 2026-07-24); `app` reads it and publishes it. The stamp remains
**correlation-only and approximate when two sims share a process** — anything needing
exactness reads `FrameContext.tick`.

> [[Clock — Design]] previously attributed the ambient-frame-counter and
> monotonic/wall-clock rules to "ADR-006 §6", which says neither. **Repointed 2026-07-25**
> — the note now cites this §.

### 10. SDK exposure — deferred, deliberately

**No logging or assert macro crosses `te_sdk` in this ADR.** The script-facing seam
defers to the scripting ADR: ADR-010 §8 (POD handles + abstract interfaces) is still only
**Proposed**, so it is not binding, and ADR-006 §3 tier 2 exposes "only façade types whose
full definitions ship" — pushing a formatting library through that boundary is its own
decision.

§1's zero-third-party public header is what keeps this cheap: the day a script wants to
log, the choice is still fully open. Note the mechanical consequence — `te_sdk` has its
own include dir (`sdk/CMakeLists.txt:6,12`) and nothing else, so the first log macro that
lands in `sdk/include/` **reddens `te_sdk_smoke`** (ADR-008 §7) until that ADR decides the
seam. That is the gate working, not a bug.

### 11. `TE_ASSUME` — out

C++23 `[[assume]]` / `__assume` as an optimizer hint is **rejected for now**: a wrong
assume is UB, there is no measured need, and CLAUDE.md forbids speculative optimization.
If it ever lands it is a **separate, opt-in** macro — **never** auto-derived from a
compiled-out `TE_ASSERT`.

## Consequences

**Positive**
- F20 (no call-site info), F10 (no log/assert strategy, silent release break), F16
  (logger-as-System) and F4 (duplicated logger globals) are all closed by construction.
- **`base`'s public header carries zero third-party types**, so the formatting backend,
  the sink implementation and the SDK question all stay reversible behind one `.cpp`.
- **No new dependency and no `deps.cmake` change** — the spdlog pin and its bundled fmt
  are untouched; only the *visibility* of the existing dep changes.
- The four tiers give every real use site an exact match, and the single handler seam is
  the same one tests use — so the failure contract is itself testable.
- Two latent build bugs are decided before they are written: linker-stripped static
  registration (§2) and a Windows-only assert handler reddening the Linux leg (§6).

**Negative / open**
- **`std::format` on C++20 is a smaller feature set than fmt** — no `fmt::join`, no ranges
  formatting before C++23, and custom formatters need `std::formatter` specializations.
  (Type formatters for math live with math regardless — ADR-006 §6.)
- **The Linux/Clang leg is unverified.** `std::format` needs libstdc++ 13+; the CI leg
  installs `clang` from `ubuntu-latest` apt (`ci.yml:52-53,71`), which satisfies that
  *today* but is not pinned. First PR run is the real test. Exit is named below.
- **`std::format`'s compile-time cost in a header every TU includes is unmeasured** —
  MSVC's `<format>` is not light. It is a *measurable* number, and §1's seam is exactly
  what makes swapping to fmt's lighter `base.h` a one-file change.
- **Process-global diagnostics state is a real exception** to ADR-006 §5's "stateless"
  utility wording, argued rather than avoided (§8). The guardrail is a review discipline,
  not a compiler check — the weakest link in this ADR.
- **Explicit registration means a module can forget to register** and silently log to the
  default channel. A missing-registration warning is a later nicety, not a Sprint-02 item.
- **Truncation on stack-buffer overflow** loses the tail of very long messages. Accepted
  for an allocation-free happy path; the marker makes it visible.

## Alternatives considered

**On the seam (§1)**

- **`fmt` in the public header + `SPDLOG_FMT_EXTERNAL=ON`** — the [[Logger — Design]]
  sketch, made buildable. Fetch standalone `fmt`, point spdlog at it, then `fmt::fmt` is
  PUBLIC on `te_base` and `spdlog::spdlog` is genuinely PRIVATE. **Viable runner-up**, and
  the named fallback: it wins on feature set and probably on compile time, and dodges the
  libstdc++ risk entirely. Rejected *for now* because it adds a second pinned dependency
  plus a spdlog↔fmt version-compatibility constraint to maintain, and puts a third-party
  template library in the surface the SDK will eventually have to cross (§10) — for
  features nothing in Sprint 02 needs.
- **`fmt` in the public header with the bundled fmt** (i.e. as originally sketched, no dep
  change) — **rejected: not achievable.** The bundled fmt lives inside spdlog's include
  tree, so this silently forces `spdlog::spdlog` PUBLIC and the "spdlog in one `.cpp`"
  half of the decision evaporates. This is the option the design note actually described.
- **`SPDLOG_USE_STD_FORMAT=ON`** — makes spdlog itself use `std::format`. Not needed: log
  dispatch hands spdlog a pre-formatted string (§1), so spdlog's internal backend is
  irrelevant and flipping it only adds a constraint.
- **Keep spdlog PUBLIC; use spdlog's own macros; no façade** — the genuinely cheapest
  path, and spdlog is unlikely ever to be replaced, so the swappability argument is
  partly theoretical. Rejected on the two costs that are *not* theoretical: spdlog headers
  land in every TU in the engine (compile time), and spdlog lands in the surface the SDK
  must cross — which pre-decides §10 by accident. The façade is one non-template function;
  that is a small price for keeping both open.

**On assert (§5)**

- **Keep ADR-006 §6's two tiers** — rejected: it conflates evaluation with abort, so
  "evaluate the side effect in shipping but don't kill the player's session" has no macro.
  That combination is common enough (`TE_VERIFY`, `TE_ENSURE`) that call sites would grow
  hand-rolled `if (!x) { log; }` variants and the single strategy would erode — which is
  F10 returning by another door.
- **An external assert library** — rejected: the whole implementation is macros +
  `source_location` + intrinsics, and a dependency here would sit *below* the Logger in
  `base`, the leaf we are most protective of.
- **Handler chosen at link time** rather than installed at runtime — rejected: tests need
  to scope-swap policy per test case (ADR-008 §6), which a link-time choice cannot do.

**On channels (§2)**

- **A `base` enum of channels and/or modules** — rejected: couples the leaf upward
  (F3/F12) and makes adding a module an edit to `base`, contradicting ADR-006 `:305-309`.
- **File-scope self-registration** (`static` initializers) — rejected on a concrete
  mechanism, not taste: static libraries strip the TU (ADR-006 §1,
  `cmake/techengine_module.cmake:38`), so channels would vanish depending on whether some
  *other* symbol in the same TU happened to be referenced.
- **String channel names at the call site** — rejected: per-call hashing on a path that
  can fire per-frame, for no gain over a handle.

**On the frame stamp (§9)**

- **Ambient global `Clock*` read by the macro** — rejected: a second access path to an
  `EngineContext` service, which is the shape ADR-006 §4 exists to remove (§9).

## What would move this decision

So it is **Accepted deliberately, not by default** — the evidence that should change an
axis, as a number or an event, not a feeling:

- **Linux leg can't do `std::format`** (libstdc++ < 13 on the pinned image, or a Clang +
  libstdc++ `std::format` defect) → switch to the runner-up: standalone `fmt` PUBLIC +
  `SPDLOG_FMT_EXTERNAL=ON`. **This is a one-file change by construction** and needs no new
  ADR; record it as a dated amendment. **Trigger: the first CI run of S2-T2.**
- **Compile-time regression from `<format>` in the public header** — measure a clean
  `windows-debug` build before/after `log.hpp` exists across all modules. A material
  regression → fmt's lighter `base.h` via the same runner-up.
- **A formatting feature is genuinely needed** (`fmt::join`, ranges, compiled format
  strings) → same switch. Wanting nicer container output is *not* that; a hot path that
  measurably needs compiled format strings is.
- **The §8 guardrail is breached** — if a service or system lands in diagnostics state,
  the "not a locator" argument is void and diagnostics owes a redesign (or the state moves
  into `EngineContext`).
- **A second sim in one process makes the approximate frame stamp actively misleading** in
  real debugging (editor hosting client + server) → per-sim diagnostics context, which is
  a bigger change than a stamp and would supersede §9.
- **Async logging becomes a measured need** (file-sink I/O showing on a frame-time
  profile) → async sink behind the unchanged façade; ordering guarantees then need
  deciding, so it earns an amendment.
- **`TE_ASSERT` in RelWithDebInfo measurably costs** on a profiled dev-runtime build →
  flip §5's call for that config only.

> Add to [[ADR Index]]. Once Accepted, treat as immutable — supersede with a new ADR.
