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

**1a. User Systems bring their own component types.** Tier 1 is not "engine components
only": a user System registers new components through the **same stable-id seam** as the
engine built-ins (ADR-007 §1 — `TE_COMPONENT("Game.Health")`, content-derived id, so
registration order never perturbs it; namespaced tags make engine↔user collision a
registration-time `TE_CHECK`). A user component is therefore a **first-class ECS
citizen**, not a second tier:

- **another user System declares `Write<Health>` / `Read<Health>` on it** and gets
  conflict detection, serializing edges, and parallelism identical to `Write<Transform>`;
- it carries the same three registration facts (stable-id · serializable · replicated),
  so an opted-in POD user component replicates through the normal encoder (ADR-007 §2);
- change-tracking stamps it like any other column.

This is what makes "engine and user systems are the same kind" (ADR-006 §5) true of the
*data* as well as the code — a game can be authored entirely in user components without
touching engine ones.

**2. Script — tier 2, convenience.** A per-entity behavior object:
- **attached to an entity via a `ScriptComponent`** — an ordinary ECS component (§2a);
- authored by deriving from `Script` in the game DLL, registered with
  `TE_REGISTER_SCRIPT(T)` — self-registers a factory (stable name → create) at DLL load,
  the same self-registering idiom used for Logger channels;
- **lifecycle mirrors Systems** — `onFixedUpdate` (authoritative tick) and `onUpdate`
  (per frame), plus `onStart` / `onDestroy`.

**2a. Binding is a `ScriptComponent`.** A script attaches through an ordinary ECS
component. The rejected alternative is v1's shape — a side table on `ScriptSystem` keyed
by `Entity`, no component at all (Alternatives).

Why the ECS-native binding:
- **Lifetime is free.** Entity destroy takes the component with it and archetype moves are
  the ECS's problem — no second source of truth to hand-sync, no destroy hook the ECS owes,
  no script outliving its entity.
- **Scripts are ordinary data** — queryable, inspectable and serializable like any other
  component, so the editor and serializer need no bespoke "which scripts are on this
  entity" path.
- **Script reach stays bounded statically.** The engine knows which archetypes carry a
  `ScriptComponent`; with a registry it would not, and script access would be a purely
  runtime fact rather than a schedule fact.

Cost accepted: a script is a **polymorphic, heap-resident object with per-instance state** —
the opposite of what a POD SoA column is for. Storing one behind a component means either an
indirection or a non-POD column, both of which cut against the grain ADR-007 §2 and F13
exist to protect, and hot-reload identity gets fiddly on top. v1's script *ergonomics* were
the half that worked; the broken half was delivery (F9/F11), already fixed by the curated
`te_sdk` + its CI leak-gate (ADR-008 §7).

**Not decided here** (→ scripting ADR): whether `ScriptComponent` holds the instance
**inline or as a handle into a `ScriptSystem`-owned pool**, and hot-reload survival.
**Explicitly not the reason for this choice:** it does *not* fix the change-tracking cost of
§5 — that is an independent axis, see §5a.

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
to a **terminal slot** in its phase. This is a new task-graph concept: ADR-007 §6's
`.after<A>()` is *pairwise* and cannot express "after everything."

**5. Scripts do not declare access — and neither does `ScriptSystem`.** Not a broad
all-component set: **nothing at all.**

A terminal-slot entry (§4) is by definition serialized after every other entry in its
phase, so the slot alone fully determines its ordering — a declared set would say nothing
the pin has not already said. Scripts do not run in parallel with each other either. So the
set has **no scheduling consumer**, and §5a moves its only other consumer, replication, to
the façade. With no consumer left, declaring it is ceremony that costs correctness (§5a).

This is a property of the **terminal slot**, not a privilege of `ScriptSystem`: any entry
pinned there — including a user's own script runner (§4) — gets the same deal, so §4's "no
privileged path" holds.

**5a. The façade is the *sole* change-tracking path for scripts.** ADR-007 §2 stamps
`changeTick` from **declared** writes. §5 declares nothing, so nothing is stamped — and a
script's write to a replicated component would never reach the delta encoder and would
silently stop replicating. Something must stamp, and it must be precise.

§8 routes *every* script component access through the curated `te_sdk` façade, which makes
the façade the natural choke point: it stamps `changeTick` at the real write.

| Consumer | Where scripts get it |
|---|---|
| **Task graph** (conflict / order) | the terminal slot (§4) — no declared set needed |
| **Replication** (`changeTick`) | the **actual** writes, observed at the façade |

Tier 1 keeps coarse declared-write stamping with **zero inner-loop cost** (ADR-007 §2
unchanged); tier 2 pays one stamp on top of a virtual call it already pays for (the "façade
call cost" negative below). Two authoring tiers, two tracking granularities — and the
precise one lands on the tier that already accepted the overhead. Splitting consumers is in
ADR-007 §6's grain: it already lists the access set's four consumers separately.

**Why not keep a broad declaration as a backstop:** it would mark every replicated column
dirty every tick, the delta encoder would find `changeTick > lastAckedTick` for all of them,
and delta compression would collapse into a **full snapshot per tick**. Not a user error —
the author wrote a script and opted a component into replication, both correct — and no way
for them to opt out. A backstop that always fires is not a backstop.

**No ADR-007 supersede needed:** §2 fixes the *query* ("changed since tick N") as
representation-independent and reserves room to change how the answer is produced. This
changes **who stamps**, not the API.

**6. No new structural-change mechanism.** Script spawn/despawn/add/remove queue into
ADR-007 §6's existing command buffer, applied at the phase barrier.

**7. Vocabulary: `Scene`, never `World`.** The runtime ECS container is `Scene`
everywhere, including the SDK façade (`IScene`). ADR-006 §2/§5 and ADR-007 §6 named the
same concept `World`; both carry a **dated positioning amendment (2026-07-24)** covering
this plus the "scheduler" retirement (naming only, no decision changed).

**8. Scripts reach the engine only through curated `te_sdk` façade types** (POD handles
+ abstract interfaces). The detailed façade and query/view shape **defer to the scripting
ADR** — they depend on an ECS that does not exist yet.

## Consequences

**Positive**
- Two genuine on-ramps — data-oriented Systems for bulk/perf work, Scripts for
  per-entity gameplay — without a second runtime or an embedded VM.
- **Tier 1 is complete for users**: own Systems *and* own component types (§1a), so a game
  is authorable without engine components — and user components cost the engine nothing
  extra, they reuse the stable-id seam already required for replication.
- The runner is a schedule entry, so it is disableable/replaceable like any engine
  default; a user could ship their own script runner.
- Scripts inherit determinism *for free*: `onFixedUpdate` sits inside the authoritative
  tick, so simulation-affecting script logic is replication-safe by construction.
- Reuses the existing command buffer and stable-id seam — no new machinery.
- Recovers the v1 ergonomics that worked, on a CI-guarded SDK boundary that fixes F9/F11.
- **Delta compression survives tier 2** (§5/§5a): the terminal slot makes a declared access
  set unnecessary, so there is no broad declaration to over-stamp with, and scripts get
  precise change-tracking off a façade call they already pay for. The parallelism ceiling
  stays a *scheduling* cost and never becomes a *bandwidth* cost.

**Negative / open**
- **Parallelism ceiling.** `ScriptSystem` sits in a terminal slot, so it runs *after* —
  never alongside — every other system in its phase, and scripts do not run in parallel
  with each other. Script-heavy scenes will bottleneck here. The ceiling is now caused by
  the **slot**, not by a writes-everything declaration (§5); the effect is the same.
  Upgrade path: let scripts opt into declared access and leave the terminal slot
  (converging toward tier 1) — deliberately not mandated now.
- **No access validation inside scripts.** ADR-007 §6's debug check `actual ⊆ declared`
  has nothing to check against — §5 declares nothing. Partly recovered by §5a: the façade
  observes the *actual* touched set, so it can be **reported** for tooling and debug; there
  is simply no declaration to hold it to.
- **Façade call cost.** Type-erased/virtual access is fine per-entity, unsuitable for
  bulk iteration. Users doing bulk work in a script will write slow code — a real
  documentation and ergonomics risk.
- **Terminal-slot ordering** must be added to the task graph (see §4).
- **Two models to teach**, with a risk that users default to scripts for everything.
- **The instance does not fit the column** (§2a). A polymorphic, heap-resident script
  behind a POD SoA component needs an indirection or a non-POD column; which one — inline
  vs a handle into a `ScriptSystem`-owned pool — plus **hot-reload survival**, remain
  undecided → scripting ADR.
- **Script execution order must be stable, not incidental.** Reconciliation replays unacked
  commands (ADR-007 negatives), so an order differing between the original tick and the
  replay diverges it → rubber-banding on lossy connections. Archetype iteration order
  shifts as entities move archetypes, so this needs an explicit stable key rather than
  whatever order iteration happens to produce. Arbitrary-but-stable is sufficient; v1 could
  ignore this, v2 has prediction. Ordering key → scripting ADR.
- **§5a is the *only* stamping path for scripts — no backstop.** Under a broad declaration
  an escaped write would still have been stamped (over-broadly, by accident); with §5
  declaring nothing, a script write that bypasses the façade silently stops replicating.
  What keeps this true is structural rather than disciplinary — scripts can only *see*
  façade types (§8), enforced by ADR-008 §7's CI leak-gate — but it is the assumption the
  scripting ADR must not weaken.

## Alternatives considered

- **No `ScriptComponent` — script instances in a side table owned by `ScriptSystem`** (the
  v1 shape: a script was registered and simply ran, holding its own entity reference) —
  *Rejected (§2a).* For it: every ECS column stays POD, no polymorphism or indirection in
  storage, and it is the least machinery to build. Against, decisively: lifetime becomes a
  second source of truth hand-synced to entity destroy (a script outliving its entity is
  the failure mode); the editor and serializer need a bespoke "which scripts are on this
  entity" path; and script reach can no longer be bounded statically. Note it was **not**
  rejected to fix §5a's bandwidth issue — storage and declared access are independent axes,
  and swapping storage fixes nothing there.
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
