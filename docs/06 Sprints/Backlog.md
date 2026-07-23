# 🗃️ Backlog

Everything wanted but not in the active sprint. Groom at sprint planning. Pull
from here into [[Sprint Board]]. At grooming, run each item through the
[[Planning Workflow — Artifact Gate]] — ADR / design note / neither, don't reflex-mint.

**Hierarchy:** `## module` (per [[ADR-006 — v2 core architecture & module layout]] §1)
→ `### utilities | systems` → `#### <item>`. **utilities** = passive helpers you
*call* (Logger, Profiler, assert, clock, math, allocators — ADR-006 §5 helper/utility);
**systems** = major engine subsystems (ECS, resources, serialization, physics, renderer,
netcode). Note: this "systems" bucket is an org convenience, **not** the ECS `ISystem`
taxonomy (ADR-006 §5). Empty headings are kept on purpose — they show where future work
lands and flag coverage gaps.

> 🧹 **Reset for the fresh start (Jul 19).** v1-codebase work (finish render-graph
> migration, editor stability pass, template-contract cleanup, etc.) is **dropped** —
> v1 is a frozen reference ([[ADR-004 — Fresh start (v2) with v1 as reference]]), no
> feature work on it. v2 **foundation begins now** (Sprint 02, base utilities first);
> everything else stays parked until designed. **Each item carries its own trigger** —
> read that, not a blanket "parked."

- 🎯 **C2 — first v2 vertical slice (Sprint 03 anchor)** *(milestone, cross-module).* First
  end-to-end slice built on the ADR-008 scaffold. The renderer/render-graph ADR
  (client → systems) and **B4** conventions (etc → infra) are both **born here**,
  evidence-driven, with v1 passes as reference. **Re-sequenced to Sprint 03** (see the
  Sprint 02 lock below) — the slice builds on the base foundation Sprint 02 lays.
  **Trigger:** Sprint 03 planning (end Aug).

> 🔒 **Sprint 02 direction — locked (Jul 23, pre-planning).** Sprint 02 is a **narrow
> horizontal base**, not the vertical slice: **Diagnostics (Logger + Assert) + Clock**,
> unit-tested, pulled by the **first sliver of the app loop (± window)** as the real
> consumer. Pressure-test: the rest of `base` (Profiler, memory tracking, Pool, SlotMap,
> ring buffer, containers) has **no Sprint-02 consumer** → building it now = speculation
> ([[Planning Workflow — Artifact Gate]]). **C2 vertical slice → Sprint 03.** Consequence:
> Q3 DoD demo lands at quarter's edge (Sep) — accepted. Scope details (window in/out, loop
> depth, fixed-timestep now/later; first task = the **Diagnostics ADR**) finalized Sun 26 Jul.

## base
Pure foundation, below the OS (ADR-006 §1): math, logging, assert, clock,
containers/allocators. No OS, no ECS.

### utilities

#### Logger
**Structured logging over spdlog — helper(utility) in `base` (ADR-006 §5/§6).** `source_location`
capture (F20), `do{}while(0)` (F10); math formatters live with math (§6). **Full design + level
rules → [[Logger — Design]].** In brief: `fmt` in header / **spdlog hidden in one `.cpp`** (type-erased
`format_args`); **self-registering channels** tagged by module; structured `LogRecord` → editor + file
sinks + flush-on-crash hook (crash/minidump in `platform`); per-level macros `TE_LOGGER_INFO/…`,
positional `{0}` args; wall-clock + frame-# timestamps. **Load-bearing bits need their own ADR.**
**Trigger:** Sprint 02 — foundation.

#### Profiler
**Scoped CPU + GPU instrumentation — helper(service) in `base` (ADR-006 §5).** RAII scopes,
`string_view` names, no `shared_ptr` registry (fixes F19); ~free when compiled out. The
"profiling hook" CLAUDE.md requires before any optimization; gates the Job-system ADR. GPU
zones = GL 4.5 timestamp queries per render-graph pass (absorbs the old
`client → per-pass GPU timing + overlay` item).
**Direction — decided, needs its own ADR:** thin `base` façade → **Tracy** backend. Editor embeds
Tracy **`Worker` headless** (no Tracy ImGui → dodges the two-context clash) and draws a **native,
simple, dockable panel** over Worker's data; a "deep dive" button dumps a `.tracy` snapshot and
launches the **Tracy desktop app** (native viewer — not a browser). **Open Qs for the ADR:** app
in-proc vs separate runtime process (loopback topology — an ADR-006 composition question); single
live consumer → deep dive = snapshot-to-file, not a 2nd live feed; pin Tracy version (wire-protocol
lock); socket **off** in shipping builds. **Trigger:** **not Sprint 02** — no hot path to measure
yet (pressure-test, [[Planning Workflow — Artifact Gate]]). Lands just-in-time before the **first
perf pass** (renderer/ECS, Sprint 03+); CLAUDE.md's "profiling hook before optimization" presupposes
an optimization pass, which doesn't exist until then. Also gates the Job-system ADR.

#### Assert
**Four tiers (`ASSERT`/`VERIFY`/`CHECK`/`ENSURE`), one hookable handler — helper(utility) in `base`
(ADR-006 §6).** `ASSERT` debug-only compiled out · `VERIFY` always-evaluates, debug-only abort ·
`CHECK` always-on fatal · `ENSURE` always-on non-fatal (log + continue, report-once). Fail → **Critical**
via [[Logger — Design|Logger]] + flush + controlled abort; never a silent `__debugbreak` in release (F10);
debugger break only if attached (`platform` handler); failure path `[[unlikely]]`/cold. **No external lib.**
**Full design + usage rules → [[Assert — Design]].** Combined **Diagnostics ADR** with the Logger.
**Trigger:** Sprint 02 — rides with Logger.

#### Clock / time
**Read-only time facade in `EngineContext` (`const Clock&`, ADR-006 §5) — loop writes, systems read.**
Monotonic (`std::chrono::steady_clock`) + wall-clock (`system_clock`) sources **in `base`, no `platform`
seam** (steady_clock is std + QPC-backed; add a raw platform timer only if the Profiler needs it — measure
first). Exposes the per-frame values the loop publishes: `dt`/`unscaledDt` (fixed sim step, scaled/real),
integer **`tick`** (netcode master quantity, ADR-007 — integer-counted for determinism, constant `dt`),
`alpha` (render interpolation), `frame` (Profiler/Logger correlation), `totalTime`, `timeScale`
(pause / slow-mo). Monotonic for durations; wall-clock only for stamps. **Timestep *policy* (fixed sim /
variable present / interpolate) lives in the loop → `app → Loop timestep policy`, not here.**
**Trigger:** Sprint 02 — foundation (needed by Profiler + loop).

#### Math
**Thin glm layer in `base` (ADR-005, ADR-006 §6) — alias, don't wrap.** `using Vec3 = glm::vec3` etc. +
engine helpers + **`fmt::formatter` specializations** (formatters live with math, not the Logger — §6). No
custom wrapper layer — glm is stable/ubiquitous, a wrapper is friction with no payoff. **No deterministic /
fixed-point math:** ADR-007 is server-authoritative + client-prediction/reconciliation (state-sync), which
tolerates float divergence — lockstep bit-determinism isn't needed. Pure functions → clean unit-test surface
(ADR-008). **Trigger:** foundation — pull as C2 needs it.

#### Allocators
**Simplified (deep-day cut): a Pool primitive + a reused-buffer convention — everything else deferred
behind a named trigger.** Custom allocators are ~10% of memory management here; the heavy lifting is ECS
columns + handles + ownership discipline (see **Ownership policy** + **Memory-management note** in `etc`).
- **Pool** — fixed-size free-list, O(1), no fragmentation. Kills v1 **F28** (per-event heap alloc). Lands
  with its first consumers (events/particles, `core`).
- **Reused / retained buffers** — the *default* for per-frame homogeneous data (draw cmds, culling, packet
  buf): `std::vector` + `clear()`/refill, no per-frame alloc after warm-up → convention for `CONVENTIONS.md` (B4).
- 🕓 **Frame allocator — deferred** to the job-system era (heterogeneous temporaries + per-thread worker
  scratch). ADR-007's "no per-frame heap alloc" met by reused buffers meanwhile; the `EngineContext` slot
  (ADR-006 §5) stays unpopulated until then.
- ❌ **Scoped arena — dropped** (intra-scene churn breaks a bump arena); revive only as niche asset-import
  scratch if baking needs it. ❌ **Global allocator (rpmalloc/mimalloc) — dropped**; `new`/`malloc` until a
  profile says otherwise.
**Trigger:** Pool with events/particles; reused-buffer convention now.

#### Memory tracking
**Tag allocations by subsystem + track high-water / budgets / leaks — land EARLY, feeds the Profiler.**
Cheap instrumentation that makes "measure before optimizing" (CLAUDE.md) real *before* any exotic allocator
is justified. Emits into the **Profiler** (base → utilities) — Tracy has first-class memory events/plots, so
the Profiler panel doubles as the memory dashboard. Pools + third-party hooks (Jolt `TempAllocator` /
`JPH::Allocate`) route through it. **Trigger:** Sprint 02 — foundation, with the Profiler.

#### Containers
**`std` where it's fine; hand-roll only for a measured hot-path need — in `base` (ADR-006 §1).**
Known near-term (real consumers already): **SlotMap / HandleMap** (index + generation — the
handles-not-pointers backbone for resources / ECS / GPU objects; stable, O(1), generation catches
use-after-free) · **ring buffer** (Logger editor sink, Profiler, net snapshot rings). Pull-when-needed:
`small_vector` (SBO), `fixed_vector` (heapless bounded), flat_map/flat_set (sorted-vector). The handle
**usage policy** lives with **Ownership policy** (`etc → Infra`) — base owns the *tool*, not the rule.
**Trigger:** SlotMap + ring buffer with their first consumers; the rest as hot paths appear.

### systems
- _(none — `base` is utility-only by design)_

## platform
The OS seam (ADR-006 §1): window, input, file I/O, hi-res timer, dynamic-lib load,
sockets *(later)*.

### utilities
- _(no parked items — facilities land as C2 / netcode need them; hi-res timer backs base's Clock)_

### systems
- _(none)_

## core
Simulation, server-capable (ADR-006 §1): ECS, events, resources, serialization,
physics (Jolt), job-system.

### utilities
- _(none yet — UUID / type-identity rides with Serialization / Resources, not a standalone util)_

### systems

#### ECS scheduling — DECIDED
~~System scheduling/ordering~~ — decided in [[ADR-007 — v2 networking & ECS replication foundation]] §6 (phases + declared-access conflict DAG). *(breadcrumb — don't re-add)*

#### Job-system / task-graph — needs its own ADR
The *thread-pool execution* of the schedule ADR-007 §6 designs: work-stealing pool, task-graph
dispatch, ECS parallel iteration, integration with Jolt's internal pool (fixes F15 — v1's 3
ad-hoc threading models). ADR-007 already defines its **input** (the built plan = the task graph;
`ISystem` + `SystemAccess` unchanged), so this is execution, not interface. Land the base
Profiler first (CLAUDE.md perf rule). **Execution view + open questions →
[[Task Graph — Execution Flow]]** (draft).

#### Resources — hot-reload / eviction (candidate ADR)
Hot-reload + eviction policy for the CPU resource cache. Depends on the UUID/cache model ported
from v1 (F7, F13, F31).

#### Serialization — custom binary format, needs its own ADR
Assets + scenes format (layout, versioning, endianness, type identity, trait/registration seam so
reflection is adoptable later). On the critical path — no asset/scene persistence until it lands.
Deferred out of [[ADR-005 — v2 tech stack & toolchain]]; depends on B1 (type identity, fixes v1 F1).

#### Physics (Jolt)
- _(no parked items — Jolt integration rides in with C2 / the sim slice)_

## client
Presentation, client-side (ADR-006 §1): GL 4.5 device seam, render graph + passes,
window/input binding, GPU upload, audio playback.

### utilities
- _(none)_

### systems

#### Renderer / render graph — needs v2 renderer ADR first (was B2)
Write when BUILDING the renderer, not before. Approach already decided across ADR-005/006/007:
OpenGL 4.5 behind a device seam, renderer is a `Present`-phase System (replaceable),
`device/graph/passes` folders in `client`, render graph uses **declared read/write resource deps**
(kills v1's `RenderResources` blackboard, F22). What remains = detailed pass DAG + forward-vs-deferred
+ device interface + which v1 passes to port — implementation-stage, evidence-driven. **Trigger:**
the first vertical slice (C2), with v1 passes as reference. Rendering is verified by demo scenes +
captures, not up-front design (CLAUDE.md).

Post-C2 rendering features (deferred, Q4 direction):
- Atmospheric scattering (sky LUT, aerial perspective)
- Volumetric fog (froxel scattering)
- Volumetric shadows / god rays through froxel path
- ~~Per-pass GPU timing + frame profiler overlay~~ → moved to `base → Profiler` (GPU zones + editor panel)
- Auto-exposure + bloom

#### Audio (miniaudio)
- _(no parked items — playback lands when a slice needs sound)_

## app
Loop + composition root (ADR-006 §1): game loop (simulate | present), engine lifecycle.

### systems

#### Loop timestep policy — needs a design decision (app-loop ADR / design note)
**Fixed timestep for `simulate`, variable for `present`, interpolate render with `alpha`** ("fix your
timestep"). Effectively mandated by ADR-007 (server tick = master clock) + Jolt stability. The loop owns
the accumulator and **publishes `dt`/`tick`/`alpha`/`frame` into the Clock facade** (`base → Clock`,
read-only to systems). Open: fixed rate (60? 128?), spiral-of-death clamp, sim-ahead-of-render for
prediction (ADR-007 time-sync). **Trigger:** the C2 loop / first sim slice.

## net

### systems

#### Netcode transport — needs its own ADR
Candidates: **ENet** (light, low-dep) · **GameNetworkingSockets** (batteries-included, heavy deps) ·
**yojimbo/netcode.io** (security-first). Decision deferred until the server module is real (v1
dropped it, F2/F34, [[ADR-004 — Fresh start (v2) with v1 as reference]]). Engine talks to it through
a thin transport seam (reliable/unreliable send, connect/poll) — flag for B1 so the netcode module
has a place to sit.

## server (future module)

### systems
- _(no parked items — `runtime-server` exe + role-gated sim land with the netcode transport above)_

## scripting / `te_sdk` (future module)

### systems

#### Native C++ script DLL SDK & hot-reload — needs its own ADR
User game code = a native C++ DLL loaded by the engine ([[ADR-006 — v2 core architecture & module layout]]
§3: `te_sdk` INTERFACE + façade types; fixes v1 F9/F11; SDK-smoke CI guards it,
[[ADR-008 — v2 build & testing baseline]] §7). **Boundary cleanliness = MIDDLE** (decided): abstract
interfaces + handles/POD + explicit ownership — *not* raw STL by value, *not* a full C ABI. Knock-on to
pin in the ADR: boundary cleanliness ↔ script-DLL **debug config** — a middle boundary lets a user build
their DLL in **Debug** (full stepping) against the shipped **RelWithDebInfo** editor; a dirty boundary
would force the DLL to match the host's CRT/`_ITERATOR_DEBUG_LEVEL`. ADR must decide: exact façade shape,
ownership rules, hot-reload mechanism, ABI/version stability. **Trigger:** when scripting becomes real
(post first vertical slice).

## editor & tooling (exe)
Not a module — the `editor` exe (ADR-006 §1); owns the asset pipeline. Flat list, no utilities/systems split.

- Frame capture / debug-visualization tools

## etc — cross-cutting (non-module)

### Infra / process
- **B4 — v2 code conventions + root `CONVENTIONS.md` (descoped from Sprint 01) — write
  when coding starts.** Conventions before any v2 code = speculative ("match the
  surrounding code" is vacuous on greenfield). Task = collect the scattered rules
  (CLAUDE.md naming; [[ADR-006 — v2 core architecture & module layout]] §6 log/assert;
  [[ADR-008 — v2 build & testing baseline]] §8 link-what-you-use) + fill gaps (include
  order, file skeletons, const-correctness) into **one** root `CONVENTIONS.md` — judgment
  rules only; link `.clang-format`/`.clang-tidy` for the mechanical subset. CLAUDE.md
  "Code conventions" then shrinks to a pointer. One house style (Claude matches it — no
  separate AI style). **Trigger:** first module / C2 slice. Also unblocks B5's
  deferred CLAUDE.md/prompt refresh.
- README at repo root (public-facing)
- **GitHub repo & CI enforcement setup (repo admin, not code) — Miguel.** CI is
  *designed* ([[ADR-008 — v2 build & testing baseline]] §9) and its `ci.yml` is a B3
  scaffold item; this task is the **GitHub-side** config that makes the design bite:
  branch protection on `master` with the §9 **required checks**; decide repo
  **visibility** (public → unlimited free Actions minutes, kills the §9 cost trigger ·
  private → ~2,000-min cap, Windows 2×); confirm `techattackteam` org Actions
  minutes/billing. **Trigger:** right after B3's `ci.yml` first goes green (need the real
  check names to require them). Repo + remote already exist.
- Recorded-demo workflow (capture + store)
- **Ownership / smart-pointer policy (fixes v1 F13) — `CONVENTIONS.md` (B4) or a short ADR.** Default =
  value + **handle** (index + generation); `unique_ptr` = single heap ownership; raw ptr/ref = non-owning
  (never deletes); `shared_ptr` only for genuine shared + unclear lifetime (rare). Touches every module.
  **Trigger:** B4 / first real ownership decisions.
- **Memory-management design note (engine-wide reference) — future.** The map: lifetime tiers · per-module
  memory (ECS columns/handles · resource cache · GPU/VRAM · render-graph target aliasing · net rings) ·
  handles-not-pointers · visibility (tracking → Profiler). Spans base/core/client. **Trigger:** after
  ECS + resources + renderer are real.

### AI tooling (skills)
- **Skill `te-module` — new-module scaffolder.** Stamp a new engine module per
  [[ADR-006 — v2 core architecture & module layout]] / [[ADR-008 — v2 build & testing baseline]]:
  `include/TechEngine/<m>` + `src` split, `techengine_module()` call, colocated Catch2
  test exe, SDK-exposure decision, deps wiring; bundles the CMake snippet templates.
  **Trigger:** right after the build scaffold exists (this week) so it references real files.
- **Skill `te-review` — engine review rubric.** Checks a diff against the ADR structural
  invariants (F3 private-header boundary; link-what-you-use [[ADR-008 — v2 build & testing baseline]] §8;
  no export macros; `EngineContext`≠locator [[ADR-006 — v2 core architecture & module layout]] §4;
  System taxonomy §5; assert tiers §6) + code conventions. Complements built-in
  `/code-review`. **Trigger:** after **B4** conventions land (needs the full rulebook).

## Ideas (unsorted)
- _drop raw ideas here; triage later_
