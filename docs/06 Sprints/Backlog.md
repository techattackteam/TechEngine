# 🗃️ Backlog

Everything wanted but not in the active sprint. Groom at sprint planning. Pull
from here into [[Sprint Board]]. At grooming, run each item through the
[[Planning Workflow — Artifact Gate]] — ADR / design note / neither, don't reflex-mint.

**Staging area, not an archive.** An item lives here only while it's *parked* — wanted,
but not yet scheduled. Pulling it into a sprint is a **move, not a copy**: the entry is
**cut** at planning, as the card is created. Only two exits:

| Exit | When | Action |
|---|---|---|
| Pulled into a sprint | `/sprint-plan`, as the task is written | **cut** the entry; its thinking moves to the sprint note / design note / ADR |
| Settled without building | the ADR that decides it | collapse to a **one-line tombstone** + ADR link |
| _(still parked)_ | — | stays — must carry a `**Trigger:**` |

**There is no `✅ Scheduled` state.** An entry annotated as scheduled is a copy that
should have been a move: the same item now described in two places, free to drift. Cut it.
Because the pull happens at planning, an item can never still be sitting here when it
ships — "done" is not a backlog state either.

Cut only once the thinking has somewhere else to live. If an entry is the *only* home for
its design content, that content **moves** into the sprint note, design note, or ADR as
part of the pull — never dropped on the floor. That hand-off is what the artifact gate
([[Planning Workflow — Artifact Gate]]) is already deciding at exactly that moment.
Tombstones exist only to stop a settled question being re-proposed
(`#### ECS scheduling — DECIDED` is the pattern) — built work needs no gravestone.

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

## base
Pure foundation, below the OS (ADR-006 §1): math, logging, assert, clock,
containers/allocators. No OS, no ECS.

### utilities

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

#### FrameAllocator — untracked, architecturally mandated
`EngineContext` carries `FrameAllocator&` (ADR-006 §4) and ADR-007 §2's "no per-tick heap snapshot"
promise rides on it (encode columns → wire buffer through it). **Out of near-term scope** — not in
Sprint 02's base slice — but it is not optional in the architecture, and nothing tracked it until now.
Open: reset granularity (per frame vs per tick, given `FixedUpdate` runs ×N — [[Game Loop — Frame Flow]]).
**Trigger:** replication / the first netcode slice.

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

#### glad2 GL loader — deferred, wired but unused
Fetched in `cmake/deps.cmake` and deliberately **not** built into anything: nothing opens a GL
context yet, and a loader with no context is dead weight on every build. **Trigger:** `platform`
opening its first GL 4.5 context (ADR-005) — the C2 slice.

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

**Also owes three things now** (recorded so the dependency is visible — *not* a schedule):
1. **Terminal-slot mechanism** — "after everything" in a phase; ADR-007 §6's `.after<A>()` is
   pairwise and can't express it ([[ADR-010 — User authoring model (Systems & Scripts)]] §4).
2. **Script execution ordering key** — must be stable, or reconciliation replay diverges
   (ADR-010 negatives).
3. **`ScriptSystem` declares no access** (ADR-010 §5) — the terminal slot is its whole
   scheduling contract; the graph builder has to accept an entry with an empty access set.

**Unscheduled, deliberately.** Its consumers don't exist yet (no ECS, no loop), and
[[ADR-010 — User authoring model (Systems & Scripts)]] stays **Proposed** until it lands —
that's a dependency, not a deadline. **Trigger:** the ECS + app-loop slice being real, not a
sprint date.

#### Resources — hot-reload / eviction (candidate ADR)
Hot-reload + eviction policy for the CPU resource cache. Depends on the UUID/cache model ported
from v1 (F7, F13, F31). **Trigger:** the resource cache being real — that port landing.

#### Serialization — custom binary format, needs its own ADR
Assets + scenes format (layout, versioning, endianness, type identity, trait/registration seam so
reflection is adoptable later). On the critical path — no asset/scene persistence until it lands.
Deferred out of [[ADR-005 — v2 tech stack & toolchain]]; depends on B1 (type identity, fixes v1 F1).
**Trigger:** the first asset or scene that must survive a restart.

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

#### Loop timestep policy — design note drafted
Timestep, phase cadence and the accumulator are **decided** in ADR-007 §5/§6 (fixed 60 Hz sim,
`alpha`-interpolated present, dt clamp) — assembled view + open questions →
**[[Game Loop — Frame Flow]]** (draft). Time ownership **decided** there (2026-07-24):
`FrameContext` holds sim time, `Clock` is the source + diagnostic frame counter. Still open:
`FrameAllocator` reset granularity; where net send/receive sit. **Trigger:** the C2 loop /
first sim slice.

## net

### systems

#### Per-component-type byte counters in the encoder — visibility, not a rail
Attribute outgoing bytes **per component type per second**, surfaced in a debug overlay. The delta
encoder already iterates per column (ADR-007 §2), so this is close to free. Rationale: `IsReplicated<T>`
is a bare compile-time bool — no size ceiling, no rate, no budget — so a user can opt a 300-byte
component into replication on 2k entities and the only symptom is "netcode feels bad". This converts
that into a named component. **The engine's answer to replication cost is a profiler, not a limit**
(CLAUDE.md perf rule: land the measurement before the optimization). **Trigger:** first real
replication slice.

#### Debug assert on raw entity indices arriving over the wire
POD/trivially-copyable is enforced for replicated components, but it does **not** catch a user
component holding a raw slotmap index (`uint32 targetIndex`) instead of a `NetId`. It memcpys fine and
is garbage on the receiving client — ADR-007 §1: the local index is never transmitted and is
per-process. Silent cross-machine corruption, no crash, only visible in multiplayer playtest. A
receiving-side `TE_ASSERT` (debug-only, ADR-006 §6) turns it into an ordinary, diagnosable foot-gun.
Considered and dropped as over-engineering: a `NetRef` wrapper type forcing it at compile time.
**Trigger:** first replicated user component.

#### Netcode transport — needs its own ADR
Candidates: **ENet** (light, low-dep) · **GameNetworkingSockets** (batteries-included, heavy deps) ·
**yojimbo/netcode.io** (security-first). Decision deferred until the server module is real (v1
dropped it, F2/F34, [[ADR-004 — Fresh start (v2) with v1 as reference]]). Engine talks to it through
a thin transport seam (reliable/unreliable send, connect/poll) — flag for B1 so the netcode module
has a place to sit. **Trigger:** the `server` module becoming real / the first replication slice.

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
- **Ratify the `CONVENTIONS.md` *Open* rows** — the file is live (S2-T10) and CLAUDE.md is
  already reduced to a pointer (2026-07-26). What's left is judgment calls, not writing: three
  rows are **⚠️ provisional** (enum-value casing, `TechEngine::detail`, the `g_` prefix) and
  need Miguel's ratification rather than Claude's reading of existing code; the rest
  (ownership, const-correctness, error handling) are marked *decide when they first bite*.
  **Trigger:** each row's own first real case — not a scheduled pass.
- **Code coverage in CI — catch *unreachable* code, not just failing code.** Motivated by a
  concrete miss: S2-T2 shipped a truncation bug in `flattenRecord` with `ctest` **100% green**,
  because all 9 logger cases install a capture sink via `setLogSink` — the seam used to
  *observe* logging deletes the default path, so `spdlogSink`/`flattenRecord` executed **zero
  times**. A passing suite said nothing; a coverage report would have shown those functions at
  **0%** immediately. This is the class of bug tests structurally cannot catch, so it needs a
  different instrument.
  - **How:** Linux/Clang leg only — `-fprofile-instr-generate -fcoverage-mapping` +
    `llvm-profdata`/`llvm-cov`. MSVC has no equivalent without OpenCppCoverage (heavier, and
    the Linux leg already runs the same deterministic suite). **Reuse the existing
    `linux-debug` job** with a coverage build type rather than adding a matrix leg — ADR-008 §9's
    CI-minute budget is live (~2k/mo, Windows 2×), and a new leg is the expensive way to do this.
  - **Start as a report, not a gate.** Print per-file coverage in the job summary and let it be
    read; a hard threshold on a 2-module codebase mostly generates noise and ratchet-gaming.
    Promote to a required check (or a "no *new* uncovered function" diff gate) once the surface
    is real — same escalation shape ADR-008 §9 uses for sanitizers.
  - **The signal that matters is 0%-coverage functions**, not the aggregate percentage. A
    codebase at 85% with a dead sink path is worse than one at 60% with everything reachable.
  - **Trigger:** after S2-T3 lands the real sink set (the first place this pays for itself), or
    sooner if another green-but-unreached bug appears. Related: S2-T3's test-reachability
    done-criterion is the *local* fix; this is the systemic one.
- **Mark third-party include dirs `SYSTEM` (`/external:I`) — build hygiene, found in S2-T2.**
  ADR-008 §5 keeps `/WX` off third-party *targets*, but their **headers compiled into our TUs
  still get our flags**: instantiating spdlog's bundled-fmt format checker from `Log.cpp` broke
  the build on `C4459` inside `spdlog/fmt/bundled/base.h`. Dodged in T2 by not using spdlog's
  format API (which ADR-011 §1 forbids anyway) — but **glm is the same shape** and will bite the
  first time a glm template warns. Fix: `SYSTEM` on fetched include dirs (`deps.cmake` already
  does it for miniaudio) or MSVC `/external:W0`. **Trigger:** next third-party header warning —
  or pre-emptively, it's cheap.
- **`TE_LOG_ACTIVE_LEVEL` fails *open* — found in S2-T2 review.** `Log.hpp` defaults the gate to
  **Trace** when the define is absent, so a TU that includes the header without linking
  `TechEngine::base` compiles Trace into Release — and `logDispatch`'s backstop can't catch it
  (that `.cpp` was compiled with `base`'s own value). Fix: default off `NDEBUG` so a missed link
  is quiet, not loud. **Trigger:** first target that includes `Log.hpp` without linking `base` —
  `te_sdk` is the likely one (ADR-011 §10).
- **Diagnostics init belongs in `app`, not a leaf exe — found in S2-T2 review.**
  `apps/editor/src/main.cpp` calls `initLogging()` (plus demo log lines, no `shutdownLogging()`),
  putting composition in the exe instead of ADR-006 §4's single wiring point. **Trigger:** S2-T7
  opens `run()` — move init there and drop the editor scratch.
- README at repo root (public-facing)
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
- **Skill `te-review` — engine review rubric.** Checks a diff against the ADR structural
  invariants (F3 private-header boundary; link-what-you-use [[ADR-008 — v2 build & testing baseline]] §8;
  no export macros; `EngineContext`≠locator [[ADR-006 — v2 core architecture & module layout]] §4;
  System taxonomy §5; assert tiers §6) + code conventions. Complements built-in
  `/code-review`. **Trigger:** after **B4** conventions land (needs the full rulebook).

## Ideas (unsorted)
- _drop raw ideas here; triage later_
