---
description: Sweep the vault for stale, redundant, malformed, or orphaned content and clean it
argument-hint: "[optional folder to scope to, e.g. '03 Architecture']"
---

Act as my vault maintainer for TechEngine. Sweep the `docs/` vault (or only the scoped
folder if given: $ARGUMENTS) and leave it **current, lean, and well-formed** — without
losing anything load-bearing. Scope is the `docs/` vault only; the AI memory files are
maintained separately (`/consolidate-memory`).

## Reconcile everything against the sources of truth
- **State:** [[Dashboard]] (Now + Rhythm), [[Sprint Board]] (task states), the **design notes**
  in `04 Design Docs/` (current shape of each system), [[ADR Index]] (the Accepted set — the
  *why*, and frozen, so a note and an ADR disagreeing is not automatically the note's fault:
  check *Partial supersessions* first), and **today's date**.
- **Rules:** repo-root `CLAUDE.md` → "Token economy — chat responses AND the vault".

**Budget:** the report is **one line per finding** — `note:line → action`. No per-finding
paragraphs, no restating the content you're flagging. Group headers only.

## What to find (scan the whole target)
1. **Stale / outdated** — content that contradicts the sources of truth: done or dropped
   work still shown as pending, superseded decisions, an old "current state", past dates
   framed as future, cadence/roles/naming that has since changed.
2. **Malformed** — empty template fields (`| **X** | |`, `- Blocker:` with nothing),
   unfilled placeholders (`YYYY-MM-DD`, `<title>`, `_none logged_`, stray `$ARGUMENTS`),
   headings with no body, broken tables, stray characters (e.g. a leading `/#`),
   **ASCII diagrams that should be Mermaid** (CLAUDE.md), a sentence that lost its link.
3. **Redundant** — the same fact in more than one note, or content restating `CLAUDE.md`,
   an ADR, or the code. It should be a `[[link]]` / `file:line`, not a copy.
4. **Broken links** — `[[wikilinks]]` to notes that don't exist; `file:line` refs whose
   file is missing (**v2 is greenfield — most v2 code paths won't resolve yet**; don't
   "fix" a path that's legitimately not-written-yet, just flag it); links to archived or
   renamed notes.
5. **Orphans / empty** — notes linked from nowhere and reachable by no path, or notes
   that are empty / placeholder-only.
6. **Bloat** — notes over ~150 lines or covering more than one topic (→ split); append-
   only growth; prose where a table/bullets would carry it in fewer tokens.
   **ADRs are never split** — an ADR is one decision record, and splitting it
   breaks the `§` refs the whole vault cites. But length is still a **signal**: an ADR past
   ~150 lines usually carries *mechanism* that belongs in the system's design note (surface
   tables, diagrams, workflows). Report it as a peel candidate for the **note**, never as a
   split, and never by editing the Accepted ADR. Judge the ADR itself on whether it covers
   more than one *decision*.
7. **[[Backlog]] leaks** — entries are cut at `/sprint-plan`, so anything scheduled or
   built that's *still there* is a leak from planning, not routine cleanup. Flag entries
   marked `✅ Scheduled`, already on the [[Sprint Board]], or settled by an Accepted ADR.
   Before proposing a cut, **check the thinking has a durable home** (sprint note, design
   note, ADR, code) — if the entry is its only home, propose a **move**. Settled-without-
   building items collapse to a one-line tombstone. A parked item with no `**Trigger:**`
   is also a finding.

## Guardrails — do NOT over-clean
- **Never rewrite an Accepted ADR's body in a cleanup** ([[ADR Index]] § *Amending an
  Accepted ADR*). A cleanup fixes genuine malformatting only: typos, broken links, stale
  boilerplate. Amendments and supersessions are decisions, not hygiene, so both stay out of
  scope here even where the policy would allow one. Report them as findings instead.
- **Distinguish a placeholder awaiting its time from a stale one.** A "fill at end"
  section dated in the future (e.g. an unfilled Sprint review for an ongoing sprint) is
  **not** an error — leave it. Flag only abandoned / never-to-be-filled placeholders.
- **Never delete load-bearing content** to save tokens — surface it for my call.

## Do
1. Produce a **report grouped by the categories above**; each finding = `note` (+ line
   or section) → proposed action (fix / merge / split / delete / relink), ordered
   safest-first.
2. **Auto-apply only the safe mechanical fixes** — formatting, dead empty fields, typos,
   relinking to renamed notes, ASCII→Mermaid — and update any index / [[Dashboard]] a
   fix affects.
3. **Ask before anything lossy** — deleting a note, merging notes, rewriting content,
   removing a section, or splitting a note.
4. Close with a short summary: what you changed vs what needs my decision. Leave the
   vault leaner and more accurate than you found it.

This is maintenance — no engine implementation.
