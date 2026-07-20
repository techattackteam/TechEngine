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

- **C2 — first v2 vertical slice (Sprint 02 anchor).** First end-to-end slice built on
  the ADR-008 scaffold. The renderer/render-graph ADR (Rendering below) and **B4**
  conventions (Process/Infra below) are both **born here**, evidence-driven, with v1
  passes as reference. **Trigger:** Sprint 02 planning (Sun 26 Jul).

### Rendering (Q4 direction — needs v2 renderer ADR first)
- **Renderer / render-graph ADR (was B2) — write when BUILDING the renderer, not before.**
  Approach already decided across ADR-005/006/007: OpenGL 4.5 behind a device seam,
  renderer is a `Present`-phase System (replaceable), `device/graph/passes` folders in
  `client`, render graph uses **declared read/write resource deps** (kills v1's
  `RenderResources` blackboard, F22). What remains = detailed pass DAG + forward-vs-deferred
  + device interface + which v1 passes to port — implementation-stage, evidence-driven.
  **Trigger:** the first vertical slice (C2), with v1 passes as reference. Rendering is
  verified by demo scenes + captures, not up-front design (CLAUDE.md).
- Atmospheric scattering (sky LUT, aerial perspective)
- Volumetric fog (froxel scattering)
- Volumetric shadows / god rays through froxel path
- Per-pass GPU timing + frame profiler overlay
- Auto-exposure + bloom

### Core / ECS
- ~~System scheduling/ordering~~ — decided in [[ADR-007 — v2 networking & ECS replication foundation]] §6 (phases + declared-access conflict DAG)
- **Job-system / task-graph — needs its own ADR.** The *thread-pool execution* of the
  schedule ADR-007 §6 designs: work-stealing pool, task-graph dispatch, ECS parallel
  iteration, integration with Jolt's internal pool (fixes F15 — v1's 3 ad-hoc threading
  models). ADR-007 already defines its **input** (the built plan = the task graph;
  `ISystem` + `SystemAccess` unchanged), so this is execution, not interface. Land a
  profiling hook first (CLAUDE.md perf rule).
- Resource hot-reload / eviction policy (candidate ADR)
- **Custom binary serialization — needs its own ADR.** Assets + scenes format
  (layout, versioning, endianness, type identity, trait/registration seam so
  reflection is adoptable later). On the critical path — no asset/scene persistence
  until it lands. Deferred out of [[ADR-005 — v2 tech stack & toolchain]]; depends
  on B1 (type identity, fixes v1 F1).

### Networking / Server
- **Netcode transport — needs its own ADR.** Candidates: **ENet** (light, low-dep) ·
  **GameNetworkingSockets** (batteries-included, heavy deps) · **yojimbo/netcode.io**
  (security-first). Decision deferred until the server module is real (v1 dropped it,
  F2/F34, [[ADR-004 — Fresh start (v2) with v1 as reference]]). Engine talks to it
  through a thin transport seam (reliable/unreliable send, connect/poll) — flag for
  B1 so the netcode module has a place to sit.

### Scripting
- **Native C++ script DLL SDK & hot-reload — needs its own ADR.** User game code = a
  native C++ DLL loaded by the engine ([[ADR-006 — v2 core architecture & module layout]]
  §3: `te_sdk` INTERFACE + façade types; fixes v1 F9/F11; SDK-smoke CI guards it,
  [[ADR-008 — v2 build & testing baseline]] §7).
  **Boundary cleanliness = MIDDLE** (decided): abstract interfaces + handles/POD +
  explicit ownership — *not* raw STL by value, *not* a full C ABI.
  Knock-on to pin in the ADR: boundary cleanliness ↔ script-DLL **debug config** — a
  middle boundary lets a user build their DLL in **Debug** (full stepping) against the
  shipped **RelWithDebInfo** editor; a dirty boundary would force the DLL to match the
  host's CRT/`_ITERATOR_DEBUG_LEVEL`. ADR must decide: exact façade shape, ownership
  rules, hot-reload mechanism, ABI/version stability. **Trigger:** when scripting becomes
  real (post first vertical slice).

### Tooling / Editor
- Frame capture / debug-visualization tools

### AI tooling
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

### Process / Infra
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

## Ideas (unsorted)
- _drop raw ideas here; triage later_
