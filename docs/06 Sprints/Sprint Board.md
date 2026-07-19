---

kanban-plugin: board

---

## 📥 Backlog

- [ ] Full backlog → [[Backlog]]


## 📋 To Do — Story B/C (after audit — nothing decided until A lands)

- [ ] B1 — v2 core & module-layout ADR
- [ ] B2 — v2 renderer-approach ADR
- [ ] B3 — v2 build + testing baseline ADR
- [ ] C2 — first v2 vertical slice + draft Sprint 02


## 🔨 In Progress — Story B next (v2 design, unblocked by the audit)

- [ ] C1 — run git flow (tag v1-reference, start v2) *[Miguel]*
- [ ] B5 — set up AI collaborators (agents + refresh CLAUDE.md/prompts for v2)
- [ ] B4 — v2 code conventions (naming, layout, headers, style)


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

- [x] Obsidian vault + process
- [x] AI operating model (CLAUDE.md, slash commands, operating guide)
- [x] Direction: fresh start (v2) — [[ADR-004 — Fresh start (v2) with v1 as reference]]
- [x] Breadth-first scan (first pass) — [[v1 Code Audit]]; deep audit = Story A
- [x] Vault cleanup: dropped stale v1 system/overview notes
- [x] Vault reset for fresh start (Jul 19): archived v1 ADRs 001–003, blanked premature v2 verdicts/backlog, collapsed timeline to audit-now




%% kanban:settings
```
{"kanban-plugin":"board","list-collapse":[null]}
```
%%