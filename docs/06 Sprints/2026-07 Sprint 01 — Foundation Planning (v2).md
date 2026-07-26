# 2026-07 · Sprint 01 — Foundation Planning (v2)

- **Quarter:** [[2026-Q3]]
- **Dates:** Jul 19–26 2026 — a short, special first sprint. Regular cadence starts **Sprint 02**.
  *(Planned as monthly; superseded 2026-07-26 by the **4-week Sat→Fri** rule — [[Dashboard]] → Rhythm.
  Prose below still reads "monthly": it records what was true at the time.)*
- **Epic:** Foundation & Direction
- **Decision behind it:** [[ADR-004 — Fresh start (v2) with v1 as reference]]

## 🎯 Sprint goal

> Stand up the v2 foundation: mine v1 for lessons, decide the architecture on paper,
> set up the operating model, and get a build skeleton compiling **green on CI** —
> ending with the first ceremony (Jul 26). Design before code; the plan exists before
> the engine.

Special short sprint — the timeline collapsed into the **Jul 19–26** window (audit
pulled to Jul 19, ADRs the same week). It ran audit → design → ground back-to-back
instead of across a month; the regular monthly rhythm begins at Sprint 02.

## ✅ Delivered

- **Deep v1 audit** → [[v1 Code Audit]] (F1–F35) + salvage verdicts in
  [[Lessons from v1 (reference prototype)]].
- **v2 foundation ADRs Accepted:** [[ADR-005 — v2 tech stack & toolchain]] ·
  [[ADR-006 — v2 core architecture & module layout]] ·
  [[ADR-007 — v2 networking & ECS replication foundation]] ·
  [[ADR-008 — v2 build & testing baseline]].
  *(B2 renderer + B4 conventions **deferred** → [[Backlog]], written when coding starts.)*
- **AI operating model:** technical-lead **+ scrum-master** role, 5 commands, 3 subagents,
  weekly/sprint cadence — see [[Dashboard]], [[Technical Lead Charter]].
- **Git flow (C1):** `v1-reference` tagged, `v2` is the working mainline. *(Miguel)*

## 🔨 Active — Story C: first buildable slice

Build scaffold → first CI-green skeleton runs **this week** (day plan below). Remaining:
**C2** — define the first v2 vertical slice (drafted at the Jul 26 ceremony, lands in
Sprint 02).

## 🗓️ Week of Jul 20–26 → Sprint 02 ceremony

Deep days start the **build scaffold** ([[ADR-008 — v2 build & testing baseline]]
checklist); the month's last weekend (Jul 25–26) is the first true ceremony.

| Day | Mode | Plan |
|-----|------|------|
| Mon 20 | 🟢 Deep | Scaffold pt1 — `cmake/` helpers + top `CMakeLists` + `CMakePresets` + 5 lib skeletons compiling (public/private split) |
| Tue 21 | 🟡 Light | **Free** — command polish pulled forward to Sun 19 |
| Wed 22 | ⚪ Relaxed | Rest · optional: consolidate stale memory tail |
| Thu 23 | 🟢 Deep | Scaffold pt2 — leaf exes (`apps/runtime`, `editor`) + `te_base_tests` (Catch2+CTest) + `sdk/` + `te_sdk_smoke` + `.clang-format`/`.clang-tidy` |
| Fri 24 | 🟠 Moderate | `ci.yml` (build matrix + tidy + ctest + sanitizers + SDK smoke) → green · **then GitHub repo setup** if green (branch protection + §9 required checks + visibility) · backlog grooming |
| Sat 25 / Sun 26 | 🟢 Deep + 🔴 Off | Ceremony weekend — one day: weekly review + **Sprint 01 demo + retro + plan Sprint 02** (`/sprint-plan`); other day off (karting) |

**Demo target (weekend of Jul 25–26):** v2 skeleton builds **green on CI** (both legs), tests + SDK
smoke pass. The GitHub repo task is gated on `ci.yml` first going green.

## Definition of Done

- [x] Deep audit complete; salvage verdicts filled.
- [x] v2 foundation ADRs (stack, architecture, networking, build/testing) **Accepted**.
- [x] AI collaborators set up (agents + role + commands). *(v2 `CLAUDE.md`/prompt refresh + code conventions deferred → B4 / coding.)*
- [x] `v1-reference` tagged; `v2` is mainline.
- [x] Build skeleton compiles **green on CI** (Jul 24).
- [x] Sprint 02 has a concrete headline goal — set Jul 25 ([[2026-08 Sprint 02 — Base Foundation]]).

## Capacity note

Weekly rhythm lives on the [[Dashboard]] — respect it. If a deep session runs long,
stop and continue on the next deep day rather than forcing it in one sitting.

## Sprint review — closed 2026-07-25 ✅

- **Lessons captured:** [[v1 Code Audit]] F1–F35 + salvage verdicts in
  [[Lessons from v1 (reference prototype)]].
- **Architecture decided:** ADRs 005–009 **Accepted** (stack · module layout · networking/ECS ·
  build & testing · branching); ADR-010 deliberately held **Proposed**.
- **Ground:** scaffold **green on CI** both legs, `ctest` 3/3, `master` ruleset Active.
- **First slice defined:** re-sequenced — Sprint 02 lays the **base foundation**
  ([[2026-08 Sprint 02 — Base Foundation]]); **C2 vertical slice → Sprint 03**.

→ Retrospective: [[2026-07-25 Sprint 01 Retrospective]] · Week detail: [[2026-07-25 Weekly Review]]
