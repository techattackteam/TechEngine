# 🏛️ ADR Index

Architecture Decision Records. Every load-bearing decision gets one. Use
[[ADR Template]] to start a new record. Numbered sequentially, never reused.

| #   | Title                                               | Status   | Date    |
| --- | --------------------------------------------------- | -------- | ------- |
| 012 | [[ADR-012 — Vault repository split]]                | Accepted | 2026-07 |
| 011 | [[ADR-011 — Diagnostics (Logger & Assert)]]         | Accepted | 2026-07 |
| 010 | [[ADR-010 — User authoring model (Systems & Scripts)]] | Proposed | 2026-07 |
| 009 | [[ADR-009 — Branching strategy & merge rules]]      | Accepted | 2026-07 |
| 008 | [[ADR-008 — v2 build & testing baseline]]           | Accepted | 2026-07 |
| 007 | [[ADR-007 — v2 networking & ECS replication foundation]] | Accepted | 2026-07 |
| 006 | [[ADR-006 — v2 core architecture & module layout]]  | Accepted | 2026-07 |
| 005 | [[ADR-005 — v2 tech stack & toolchain]]             | Accepted | 2026-07 |
| 004 | [[ADR-004 — Fresh start (v2) with v1 as reference]] | Accepted | 2026-07 |

> **Active ADRs only.** v2 foundation ADRs get written **after** the deep v1 audit.
>
> ADR-001/002/003 describe the **v1 reference prototype** and are moved to
> `_archive v1/` — history/prior art, out of the active list. Mine them via
> [[v1 Code Audit]] and (post-audit) [[Lessons from v1 (reference prototype)]].
> Next number is **013** (numbers are never reused).

### Partial supersessions

A superseding ADR sometimes reverses **one clause** of an Accepted ADR, not the whole
record. The superseded ADR stays **Accepted** and its body is **never edited** — the
partial scope is tracked here.

| Clause | Superseded by | Scope |
| --- | --- | --- |
| [[ADR-006 — v2 core architecture & module layout]] §6 — assert **tier** clause (`TE_VERIFY` always-on abort) | [[ADR-011 — Diagnostics (Logger & Assert)]] §5 | **That clause only.** §6's logging bullet, `TE_ASSERT` semantics, the never-silent-`__debugbreak` rule and the `[[unlikely]]`/cold failure path all remain in force. |

## Statuses

- **Proposed** — under discussion, not yet binding.
- **Accepted** — the current decision. Build to it.
- **Superseded by ADR-NNN** — replaced; kept for history.
- **Deprecated** — no longer relevant.

> ADRs are immutable once Accepted. To change a decision, write a new ADR that
> supersedes the old one — don't edit history.
