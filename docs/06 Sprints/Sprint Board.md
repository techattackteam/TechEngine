---

kanban-plugin: board

---

## 📥 Backlog

- [ ] Full backlog → [[Backlog]]


## 📋 To Do — [[2026-08 Sprint 02 — Base Foundation]] (Jul 27 – Aug 30)

- [ ] **S2-T4** — Assert: four tiers + single handler · P1 · 🟢 Deep
- [ ] **S2-T5** — Assert → Logger integration + flush-on-fail · P2 · 🟠 Moderate
- [ ] **S2-T6** — Clock implementation ([[Clock — Design]]) · P1 · 🟠 Moderate
- [ ] **S2-T7** — `FrameContext` + fixed-timestep accumulator (headless) · P1 · 🟢 Deep
- [ ] **S2-T8** — Determinism + clamp tests (injected time source) · P1 · 🟢 Deep
- [ ] **S2-T9** — End-to-end wire-up = sprint demo · P2 · 🟠 Moderate
- [ ] **S2-T11** — Skill `te-module` scaffolder · P3 · 🟡 Light
- [ ] **S2-T12** — Land `feat/improving-vault` through the ruleset · P3 · 🟡 Light


## 🔨 In Progress

- [ ] **S2-T3** — Channels + `LogRecord` + console/file sinks · P1 · 🟢 Deep — **+ sink path
	  must be test-reachable**: no code path that only runs when the default sink is installed
	  (S2-T2 shipped a bug there with the suite green)
- [ ] **S2-T10** — Root `CONVENTIONS.md` (B4) · P2 · 🟠 Moderate — **runs in parallel**, not
	  late. File opened Jul 25 (B4 migrated in; B4 → pointer + history). **Flag conventions as
	  you review** — they land here live. Remaining: fill the *Open* rows, then shrink
	  CLAUDE.md's section at sprint end.


## 👀 Review / Demo



## ✅ Done — [[2026-08 Sprint 02 — Base Foundation]]

- [x] **S2-T2** — Logger core · P1 · 🟢 Deep — **Jul 25** → PR #8 (`6f054b6b`) merged green.
	  `std::format` seam + spdlog private (ADR-011 §1), per-config compile-time gate (§4), 9 Catch2
	  cases, `.clang-format` = CLion scheme. **`std::format` survived the Linux leg** — ADR-011's
	  fmt-fallback trigger did not fire.
- [x] **S2-T1** — Diagnostics ADR (Logger + Assert) · P1 · 🟢 Deep — **Jul 25** →
	  [[ADR-011 — Diagnostics (Logger & Assert)]] Accepted; **T2–T6 unblocked**. Seam changed
	  under review: `fmt`-in-header was unbuildable → **`std::format`**, spdlog private, no new
	  dep. Frame stamp **pushed by `app`**, not pulled. Partial supersession of ADR-006 §6's
	  assert-tier clause tracked in [[ADR Index]].


## ✅ Done — Sprint 01 (closed Jul 25 · [[2026-07-25 Sprint 01 Retrospective]])

- [x] **Sprint 01 ceremony** (Jul 25) — weekly review + retro + Sprint 02 planned. Demo:
	  presets build clean, `ctest` 3/3, CI green both legs, `master` ruleset Active.
- [x] **Artifact drift reconciled** (Jul 25) — `CLAUDE.md` de-v1'd (generator, module layout,
	  build commands, testing), `04 Design Docs` refs repointed, B4 aligned to the CI format
	  gate (+ `ColumnLimit: 100` correction), scaffold checklist given one home in ADR-008.


## ✅ Done — Story A (deep v1 audit · Jul 19)

- [x] A1 — Core & ECS (F1 live workaround, service-locator, no job system, everything-a-System)
- [x] A2 — Resources & filesystem (shared_ptr overuse, god-class, editor-only FS)
- [x] A3 — Renderer (god-object + RenderResources blackboard; UI actually decomposed)
- [x] A4 — Physics/audio/script/server (scripting SDK broken F11, core→editor include F12, server stub)
- [x] A5 — Editor/build (editor in the frame loop F14, no picking, no test-bed, GLOB/multi-config)
- [x] A6 — Synthesized → [[v1 Code Audit]] deepened + salvage verdicts filled
- [x] Miguel: read the deepened [[v1 Code Audit]] (F1–F35) + [[Lessons from v1 (reference prototype)]] verdicts — closed Jul 25; B1 and the foundation ADRs shipped on them uncorrected


## ✅ Done

- [x] **GitHub repo & CI enforcement — done** (Jul 24) — `ci.yml` green on both legs; `master` ruleset **Active**: PR required (0 approvals), branch up to date, linear history, no force-push/deletion, **empty bypass list** (applies to Miguel), 8 required checks. Sanitizers confirmed PR-only on the master backstop (ADR-009 §4). **Repo stays private for now** — ADR-009 §2's protection limit never bit, but ADR-008 §9's **CI-minute budget is live** (~2k/mo, Windows 2×; ≈22 billed min per merged change today, will grow). Watch Settings → Billing → Actions; trigger response is §9's (sanitizers → nightly). Ops findings → [[B3 — Build & Testing Notes]].
- [x] **ADR-010 — decided: stays Proposed** (Jul 24) — gated on the task-graph ADR (its consumers don't exist yet). Session also settled: §1a user Systems register their own component types · §2a binding is a `ScriptComponent` (side table rejected) · §5 `ScriptSystem` declares **no** access — the terminal slot determines ordering · §5a the `te_sdk` façade is the sole `changeTick` path for scripts, which is what keeps delta compression alive.
- [x] **Game loop design note** (Jul 24) — [[Game Loop — Frame Flow]] drafted (frame diagram, `FixedUpdate` ×N, phase cadence); **decided** `FrameContext` owns sim time, `Clock` is the time source + diagnostic frame counter. Closes [[Backlog]] `app` → Loop timestep policy.
- [x] **ADR vocabulary amendments** (Jul 24) — `World` → **`Scene`** + **"scheduler"** retired (`Schedule` · task graph · `executor`). One dated amendment each on ADR-006/-007 (Accepted → header amendment, body untouched); free edits in ADR-010 §4/§7/negatives; vocabulary-debt banner dropped from [[Task Graph — Execution Flow]].
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
- [x] Build scaffold (ADR-008 checklist) — **built + green on CI** (Jul 20–24): Win/MSVC Debug+Release, `ctest` 3/3 (base ×2 + SDK-smoke), clang-format clean, ASan flag confirmed on our targets only. Slices: skeleton → deps (FetchContent, pins in `cmake/deps.cmake`) → CI + clang-format/tidy + `te_sdk_smoke`. **Open:** `ci.yml` never run → CI-green tracked by the "GitHub repo & CI enforcement" To Do card; clang-tidy verified on Linux leg only (VS tidy segfaults local); glad2 deferred until `platform` opens a GL context. Uncommitted → Miguel branches + commits.
- [x] Vault cleanup: dropped stale v1 system/overview notes
- [x] Vault reset for fresh start (Jul 19): archived v1 ADRs 001–003, blanked premature v2 verdicts/backlog, collapsed timeline to audit-now




%% kanban:settings
```
{"kanban-plugin":"board","list-collapse":[null]}
```
%%