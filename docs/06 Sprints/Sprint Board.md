---

kanban-plugin: board

---

## 📥 Backlog

- [ ] Full backlog → [[Backlog]]


## 📋 To Do — Week of Jul 20–26 (bridge → Sprint 02 on Sun 26; B2 + B4 deferred → [[Backlog]])

- [ ] Sun 26 — Sprint 01 demo + retro + plan Sprint 02 → `/sprint-plan`


## 🔨 In Progress — Story C (ground: first buildable slice)

- [ ] GitHub repo & CI enforcement — *once `ci.yml` is green*: branch protection on `master` + [[ADR-008 — v2 build & testing baseline]] §9 required checks + repo visibility (Fri/Sun · Miguel)
- [ ] Backlog grooming for Sprint 02 (Fri · moderate)


## 👀 Review / Demo



## ✅ Done — Story A (deep v1 audit · Jul 19)

- [x] A1 — Core & ECS (F1 live workaround, service-locator, no job system, everything-a-System)
- [x] A2 — Resources & filesystem (shared_ptr overuse, god-class, editor-only FS)
- [x] A3 — Renderer (god-object + RenderResources blackboard; UI actually decomposed)
- [x] A4 — Physics/audio/script/server (scripting SDK broken F11, core→editor include F12, server stub)
- [x] A5 — Editor/build (editor in the frame loop F14, no picking, no test-bed, GLOB/multi-config)
- [x] A6 — Synthesized → [[v1 Code Audit]] deepened + salvage verdicts filled
- [ ] Miguel: read the deepened [[v1 Code Audit]] (F1–F35) + [[Lessons from v1 (reference prototype)]] verdicts; correct any calls before B1


## ✅ Done

- [x] B5 — AI collaborators set up (3 agents + docs); CLAUDE.md/prompt refresh → B4
- [x] Command polish — `/adr`, `/weekly-review`, `/feature-breakdown` wired to agents + scrum-master + ADR immutability (Sun 19)
- [x] B3 — v2 build + testing baseline ADR → [[ADR-008 — v2 build & testing baseline]] (Accepted)
- [x] B1b — Networking & ECS replication foundation ADR → [[ADR-007 — v2 networking & ECS replication foundation]] (Accepted)
- [x] B1 — v2 core & module-layout ADR → [[ADR-006 — v2 core architecture & module layout]] (Accepted)
- [x] B0 — v2 tech stack & toolchain ADR → [[ADR-005 — v2 tech stack & toolchain]] (Accepted)
- [x] C1 — run git flow (tag v1-reference, start v2) *[Miguel]*
- [x] Obsidian vault + process
- [x] AI operating model (CLAUDE.md, slash commands, operating guide)
- [x] Direction: fresh start (v2) — [[ADR-004 — Fresh start (v2) with v1 as reference]]
- [x] Breadth-first scan (first pass) — [[v1 Code Audit]]; deep audit = Story A
- [ ] Build scaffold (ADR-008 checklist) — **built + locally green** (Jul 20): Win/MSVC Debug+Release, `ctest` 3/3 (base ×2 + SDK-smoke), clang-format clean, ASan flag confirmed on our targets only. Slices: skeleton → deps (FetchContent, pins in `cmake/deps.cmake`) → CI + clang-format/tidy + `te_sdk_smoke`. **Open:** `ci.yml` never run → CI-green tracked by the "GitHub repo & CI enforcement" To Do card; clang-tidy verified on Linux leg only (VS tidy segfaults local); glad2 deferred until `platform` opens a GL context. Uncommitted → Miguel branches + commits.
- [x] Vault cleanup: dropped stale v1 system/overview notes
- [x] Vault reset for fresh start (Jul 19): archived v1 ADRs 001–003, blanked premature v2 verdicts/backlog, collapsed timeline to audit-now




%% kanban:settings
```
{"kanban-plugin":"board","list-collapse":[null]}
```
%%