# 🏛️ ADR Index

Architecture Decision Records. Every load-bearing decision gets one. Use
[[ADR Template]] to start a new record. Numbered sequentially, never reused.

| #   | Title                                               | Status   | Date    |
| --- | --------------------------------------------------- | -------- | ------- |
| 009 | [[ADR-009 — Branching strategy & merge rules]]      | Proposed | 2026-07 |
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
> Next number is **010** (numbers are never reused).

## Statuses

- **Proposed** — under discussion, not yet binding.
- **Accepted** — the current decision. Build to it.
- **Superseded by ADR-NNN** — replaced; kept for history.
- **Deprecated** — no longer relevant.

> ADRs are immutable once Accepted. To change a decision, write a new ADR that
> supersedes the old one — don't edit history.
