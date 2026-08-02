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
6. **Bloat** — any **section over ~30 lines** (→ split it, or spin it out as its own note),
   notes covering more than one topic, append-only growth, prose where a table/bullets would
   carry it in fewer tokens. Judge **sections**, not files: a long design doc made of tight
   sections is healthy. **ADRs are exempt** — one immutable decision record, and splitting it
   breaks the `§` refs the vault cites; judge an ADR on whether it covers more than one
   *decision*.
7. **[[Backlog]] leaks** — it is a parking lot of one-line wants, so the tripwires are
   mechanical. Flag any entry that:
   - **runs past ~2 lines**, or carries a decision, a rationale, a dropped alternative or a
     `How:` — it has outgrown the file; its content belongs in a design note or ADR, and
     that promotion is the finding (do **not** fix it by trimming words);
   - is **settled** by an Accepted ADR or a design note — decided ⇒ deleted, **no tombstone**;
   - is already on the [[Sprint Board]], or marked `✅ Scheduled` — pulling is a move, not a
     copy;
   - carries **no `**Trigger:**`**;
   - has a trigger that has **already fired** — it's either a card or a re-trigger, not parked.

   Cutting is safe here by design: entries hold no decisions, so there is nothing to rescue
   first. An entry that *would* lose something on deletion is itself finding #1.
8. **[[Known Issues]] rot** — the list only earns its keep if it stays true. Flag a `D<n>`
   that: is **already fixed** in the code (verify the `file:line`, don't assume — then it's
   deleted, not ticked) · has had its **condition fire**, so it is now a *bug* and belongs on
   the [[Sprint Board]] as a `B` card · **misbehaves today** and was filed here instead of as
   a bug · would fail **loudly** and so is below the bar · has sat **3+ sprints** untouched
   (card it or delete it) · or whose `file:line` no longer resolves.

## Guardrails — do NOT over-clean
- **Accepted ADRs are immutable** ([[ADR Index]]). Never rewrite an Accepted ADR's
  decision. You may only fix genuine malformatting (typos, broken links) or add a
  **dated positioning amendment** that changes no decision. An actual change is a
  **superseding** ADR — out of scope for a cleanup.
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
