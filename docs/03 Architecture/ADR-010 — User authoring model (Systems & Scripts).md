# ADR-010 — User authoring model (Systems & Scripts)

- **Status:** Proposed
- **Date:** 2026-07
- **Deciders:** Miguel (Lead Engineer)

## Context

User-authored **Systems** are already first-class and fully specified:
[[ADR-006 — v2 core architecture & module layout]] §5 ("engine- and user-authored are
the same kind", defaults disabled/replaced, schedule is *data*) and
[[ADR-007 — v2 networking & ECS replication foundation]] §6 (declared `SystemAccess`,
phase pipeline, conflict DAG, command buffer, `.after<A>()` escape hatch). **This ADR
adds nothing there.**

What is *not* decided: whether a **second, lower-friction authoring path** exists.
Declaring component access and thinking in queries is the right model for bulk,
data-oriented work — and a high barrier for per-entity gameplay ("when this entity is
hit, play a sound"). Forcing all gameplay through tier-1 Systems taxes the most common,
least performance-critical code.

v1 had exactly this: a macro registered a script, and a script-engine System ticked
`update` on all live scripts. The **concept was good; the delivery was broken** —
F9/F11 (SDK exposed private `src/` types), synchronous shell build froze the editor.
Salvage verdict: *keep the native decision, redesign delivery*
([[Lessons from v1 (reference prototype)]]). The delivery half is now solved: the
curated `te_sdk` INTERFACE target exists with a CI leak-gate (`sdk/CMakeLists.txt`,
`sdk/smoke/smoke.cpp`, [[ADR-008 — v2 build & testing baseline]] §7).

## Decision

**Two authoring tiers over one runtime.** No second engine, no scripting VM.

**1. System — tier 1, performance.** Unchanged from ADR-006 §5 / ADR-007 §6: declares
`SystemAccess`, scheduled in the conflict DAG, parallelizable, replaceable.

**2. Script — tier 2, convenience.** A per-entity behavior object:
- attached to an entity via a `ScriptComponent`;
- authored by deriving from `Script` in the game DLL, registered with
  `TE_REGISTER_SCRIPT(T)` — self-registers a factory (stable name → create) at DLL load,
  the same self-registering idiom used for Logger channels;
- **lifecycle mirrors Systems** — `onFixedUpdate` (authoritative tick) and `onUpdate`
  (per frame), plus `onStart` / `onDestroy`.

**3. Scripts run after all Systems, within the same phase.**

```
FixedUpdate:  [ all systems' fixedUpdate ]  →  [ all scripts' onFixedUpdate ]
Update:       [ all systems' update      ]  →  [ all scripts' onUpdate      ]
```

Because the lifecycle mirrors Systems, the author chooses determinism the same way a
System author does: simulation-affecting logic in `onFixedUpdate` (inside the
authoritative tick → replication-safe, ADR-007), presentation/glue in `onUpdate`.

**4. The runner is an ordinary System.** `ScriptSystem` is a normal schedule entry — no
privileged path (ADR-007 §6) — registered in **both** `FixedUpdate` and `Update`, pinned
to a **terminal slot** in its phase. This is a new scheduler concept: ADR-007 §6's
`.after<A>()` is *pairwise* and cannot express "after everything."

**5. Scripts do not declare access.** `ScriptSystem` declares a broad (all-component)
access set. This is the deliberate tier-2 trade — accepted, with the costs below.

**6. No new structural-change mechanism.** Script spawn/despawn/add/remove queue into
ADR-007 §6's existing command buffer, applied at the phase barrier.

**7. Vocabulary: `Scene`, never `World`.** The runtime ECS container is `Scene`
everywhere, including the SDK façade (`IScene`). ADR-006 §2/§5 and ADR-007 §6 name the
same concept `World` → both get a **dated positioning amendment** (naming only, no
decision changes).

**8. Scripts reach the engine only through curated `te_sdk` façade types** (POD handles
+ abstract interfaces). The detailed façade and query/view shape **defer to the scripting
ADR** — they depend on an ECS that does not exist yet.

## Consequences

**Positive**
- Two genuine on-ramps — data-oriented Systems for bulk/perf work, Scripts for
  per-entity gameplay — without a second runtime or an embedded VM.
- The runner is a schedule entry, so it is disableable/replaceable like any engine
  default; a user could ship their own script runner.
- Scripts inherit determinism *for free*: `onFixedUpdate` sits inside the authoritative
  tick, so simulation-affecting script logic is replication-safe by construction.
- Reuses the existing command buffer and stable-id seam — no new machinery.
- Recovers the v1 ergonomics that worked, on a CI-guarded SDK boundary that fixes F9/F11.

**Negative / open**
- **Parallelism ceiling.** A writes-everything `ScriptSystem` conflicts with every
  System, and scripts do not run in parallel with each other. Script-heavy scenes will
  bottleneck here. Upgrade path: let scripts opt into declared access (converging toward
  tier 1) — deliberately not mandated now.
- **Weakened validation.** ADR-007 §6's debug check `actual ⊆ declared` degenerates
  inside script code, since the declared set is "everything."
- **Façade call cost.** Type-erased/virtual access is fine per-entity, unsuitable for
  bulk iteration. Users doing bulk work in a script will write slow code — a real
  documentation and ergonomics risk.
- **Terminal-slot ordering** must be added to the scheduler (see §4).
- **Two models to teach**, with a risk that users default to scripts for everything.
- Script instance storage, lifetime, and hot-reload survival are **undecided** → scripting ADR.

## Alternatives considered

- **One tier (Systems only)** — one mental model, everything parallel and
  access-validated. *Rejected:* per-entity gameplay is genuinely awkward as a
  system-with-query, and script ergonomics were the part of v1 that actually worked.
  (This was the initial recommendation; the tiers are complementary, not redundant.)
- **Scripts in their own phase after `PostUpdate`** — simpler ordering, no terminal slot
  needed. *Rejected:* scripts could then never participate in the fixed tick, so any
  simulation-affecting script logic would break determinism and replication (ADR-007).
- **Embedded VM (Lua / C#)** — *Rejected:* contradicts ADR-006 §1 (the only future DLL is
  the user game module) and the ADR-005 stack; the native-C++ decision is already made
  and salvaged from v1.
- **Scripts declare access like Systems** — preserves parallelism and validation.
  *Deferred, not rejected:* it is the natural upgrade path, but mandating it now
  reintroduces precisely the friction scripts exist to remove.

> Add to [[ADR Index]]. Once Accepted, treat as immutable — supersede with a new ADR.
