/# 2026-07 · Sprint 01 — Foundation Planning (v2)

- **Quarter:** [[2026-Q3]]
- **Dates:** July 2026 (review + plan on the last Sunday)
- **Epic:** Foundation & Direction
- **Decision behind it:** [[ADR-004 — Fresh start (v2) with v1 as reference]]

## 🎯 Sprint goal

> Plan the fresh engine (v2) on paper before writing it: mine v1 for lessons,
> decide the target architecture and what to port, set up the git flow, and
> define the first buildable slice. **No v2 engine code until the plan exists.**

Planning-heavy by design — cheapest place to prevent the "fresh start that never
ships." **Audit-first:** the deep v1 audit (Story A) gates the v2 ADRs (Story B) —
don't design v2 until the real pain is known. Miguel drives; AI leads analysis,
drafts ADRs, scaffolds on request (`CLAUDE.md`).

> **Timeline collapsed (Jul 19):** the deep audit is pulled forward to today so
> planning can start right after, instead of spreading across the month. Order is
> unchanged — **A (audit) → B (design v2 + set up AI) → C (ground)** — only the
> pace. Nothing in B/C is decided until A lands.

## Deep-audit method (Story A)

Per system: **read the code, don't skim.** For each, log with `file:line` + severity:
- **Pain points** — friction, awkward APIs, things that fought you.
- **Mistakes** — design/structure decisions that aged badly.
- **Bugs / latent bugs** — incl. things that "work by luck" (e.g. F1).
- **Failure modes** — where it breaks under load / edge cases / multi-world.
- **Coupling** — hidden deps, cross-module reach-in.

Output → deepen [[v1 Code Audit]] + verdict (port / redesign / drop) in
[[Lessons from v1 (reference prototype)]].

## Stories & tasks

### Story A — Deep v1 audit *(gate for B)*
- [x] **A1** Core & ECS — archetypes, IDs (F1 blast radius), storage, systems/registry, events, serialization
- [x] **A2** Resources & asset pipeline — ResourceSystem, caches/loaders, mesh/model/texture/material/scene/shader, on-disk formats
- [x] **A3** Renderer — render graph + every pass, GPU resources, shaders
- [x] **A4** Physics / audio / script — real vs stub, integration, ScriptsCompiler flow
- [x] **A5** Editor/tooling + build — panels, project mgmt, scripting, CMake/build system
- [x] **A6** Synthesize — finalize deepened [[v1 Code Audit]] + salvage table

### Story B — Design v2 + set up AI *(from audit)*
- [ ] **B1** v2 module layout & core architecture ADR
- [ ] **B2** v2 renderer-approach ADR
- [ ] **B3** v2 build + testing baseline ADR (CMakePresets, test framework, layout)
- [ ] **B4** v2 code conventions (naming, layout, headers, style) — write once the module layout is decided
- [ ] **B5** Set up AI collaborators — subagents/agents + refresh `CLAUDE.md`/prompts for v2

### Story C — Set up the ground
- [ ] **C1** Run git flow from ADR-004 (tag `v1-reference`, start `v2`) — *Miguel*
- [ ] **C2** Define first v2 vertical slice (smallest thing that builds+runs) + draft Sprint 02 → update [[2026-Q3]], [[Roadmap]]

## Definition of Done

- [ ] Deep audit complete; [[Lessons from v1 (reference prototype)]] salvage verdicts filled **from it**.
- [ ] v2 architecture, renderer, build/testing ADRs **Accepted**.
- [ ] v2 code conventions written; AI collaborators (agents + `CLAUDE.md`) set up for v2.
- [ ] `v1-reference` tag exists; `v2` is the working mainline.
- [ ] Sprint 02 has a concrete headline goal (the first slice).

## Capacity note

Deep days Mon/Thu/Fri/Sun; Tue/Wed light. Heavy audits + ADR decisions → deep
days. Timeline is collapsed (see the callout above): the audit runs **today
(Jul 19)** rather than spread across the month, so respect the sustainable-pace
rhythm — if the deep audit runs long, stop and continue on the next deep day
rather than forcing all subsystems in one sitting.

## Sprint review (fill at end)

- Lessons captured:
- Architecture decided:
- First slice defined:

→ Retrospective goes in [[07 Journal]].
