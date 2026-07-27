---

kanban-plugin: board

---

## 📥 Backlog

- [ ] Full backlog → [[Backlog]]


## 📋 To Do — [[2026-08 Sprint 02 — Base Foundation]] (Jul 25 – Aug 28)

- [ ] **S2-T13** — Vault repo cutover ([[ADR-012 — Vault repository split]]) · P1 · 🟠 Moderate
	  — **blocks T14/T15**. `.gitignore` + root `.ignore` land in the **same** commit, history
	  preserved, fresh clone documented. Weighted Moderate on purpose: urgent, but it must not
	  eat a 🟢 Deep slot reserved for T4/T7/T8.
- [ ] **S2-T4** — Assert: four tiers + single handler · P1 · 🟢 Deep
- [ ] **S2-T7** — `FrameContext` + fixed-timestep accumulator (headless) · P1 · 🟢 Deep
- [ ] **S2-T8** — Determinism + clamp tests (injected time source) · P1 · 🟢 Deep
- [ ] **S2-T9** — End-to-end wire-up = sprint demo · P2 · 🟠 Moderate
- [ ] **S2-T14** — Retarget vault-writing commands at the nested repo · P2 · 🟡 Light — *after T13*
- [ ] **S2-T15** — Reconciliation stamp on [[Dashboard]] (ADR-012 §6) · P2 · 🟡 Light — *after T13*
- [ ] **S2-T11** — Skill `te-module` scaffolder · P3 · 🟡 Light


## 🔨 In Progress

- [ ] **S2-T5** — Assert → Logger integration + flush-on-fail · P2 · 🟠 Moderate
- [ ] **S2-T10** — Root `CONVENTIONS.md` (B4) · P2 · 🟠 Moderate — **runs in parallel**, not
	  late. File opened Jul 25 (B4 migrated in; B4 → pointer + history). **Flag conventions as
	  you review** — they land here live. CLAUDE.md's section **shrunk to a pointer + the 3
	  AI-default corrections (Jul 26)**. Remaining: ratify the *Open* rows — 3 are ⚠️ provisional
	  and need your call, not Claude's reading.


## 👀 Review / Demo



## ✅ Done — [[2026-08 Sprint 02 — Base Foundation]]

- [x] **S2-T3** — Channels + `LogRecord` + console/file sinks · P1 · 🟢 Deep — **Jul 25** → PR #9.
	  Handles + explicit registration (ADR-011 §2), filtering at max(process, module, channel),
	  sink **array** not slot, console + rotating file behind one spdlog logger, one flatten shared
	  with the stderr fallback. **Sink path is test-reachable** — the criterion T2's
	  green-but-unreached bug earned. Ring sink → T5.
- [x] **S2-T2** — Logger core · P1 · 🟢 Deep — **Jul 25** → PR #8 (`6f054b6b`) merged green.
	  `std::format` seam + spdlog private (ADR-011 §1), per-config compile-time gate (§4), 9 Catch2
	  cases, `.clang-format` = CLion scheme. **`std::format` survived the Linux leg** — ADR-011's
	  fmt-fallback trigger did not fire.
- [x] **S2-T6** — Clock implementation ([[Clock — Design]]) · P1 · 🟠 Moderate — **Jul 26** →
	  PR #10 (`2e16a067`). `now()` / `totalTime()` / `wallClock()` / `frame()` + `advanceFrame()`;
	  **plain `uint64_t` counter** — the atomic already sits on the diagnostics side (`g_frame`),
	  so the cross-thread hop is downstream. **No seam** — injected time source goes in the *loop*
	  at T7/T8. 6 Catch2 cases, lower-bounds-only (no upper bound = no runner-load flake).
	  **Also carried:** T3's stderr-fallback flatten fix + T10's `[[nodiscard]]` ban.
- [x] **S2-T1** — Diagnostics ADR (Logger + Assert) · P1 · 🟢 Deep — **Jul 25** →
	  [[ADR-011 — Diagnostics (Logger & Assert)]] Accepted; **T2–T6 unblocked**. Seam changed
	  under review: `fmt`-in-header was unbuildable → **`std::format`**, spdlog private, no new
	  dep. Frame stamp **pushed by `app`**, not pulled. Partial supersession of ADR-006 §6's
	  assert-tier clause tracked in [[ADR Index]].
- [ ] **S2-T12** — Land the accumulated vault + AI-config work through the ruleset · P3 · 🟡 Light
	  — **Jul 26** → PR #11 24 files, **+409/−237, zero code**. Backlog is now a
	  staging area — entries cut at planning, not swept later (350→289 lines); `/task-start` +
	  `/task-wrap` bracket every task off a freshly fetched `origin/master`, which is the fix for
	  the squash-merge branch reuse that conflicted PRs #8–#10; CLAUDE.md conventions reduced to a
	  pointer + the 3 AI-default corrections, and its stale test-target names fixed
	  (`TechEngineBaseTests` / `sdk-smoke` / `TechEngine::sdk`).
	  **Done-condition not yet met** — it *is* the PR lifecycle (opened → 8 checks green →
	  squash-merged), so it closes on merge, not here. Card was retitled: the original
	  `feat/improving-vault` branch no longer exists.




%% kanban:settings
```
{"kanban-plugin":"board","list-collapse":[null]}
```
%%