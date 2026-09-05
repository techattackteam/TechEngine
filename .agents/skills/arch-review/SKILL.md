---
name: arch-review
description: "TechEngine: Technical-lead architecture review of a system or file. Use when the user requests this workflow."
---

# arch-review

Input: `<system, path, or area to review>`. Use the invocation and current conversation to resolve it.
Repository paths below are relative to the engine root; vault wikilinks resolve under `docs/`.
Follow `AGENTS.md`, including existing user authorization, build ownership, and commit rules.

Act as my technical lead and review the architecture of: the input from the user's request

Focus on design, not style. Read the actual code first (and the relevant
`docs/04 Design Docs/` note and ADRs).

Keep the review short and readable. Give each risk a brief explanation with its
`file:line` and failure scenario. Report all actionable risks; do not cut findings
to meet a line count. Omit deliverable sections that add nothing.

Deliver:
1. **What's solid** — worth keeping, don't touch.
2. **Real risks** — each with concrete evidence from the code (file:line) and a
   failure scenario, not vibes.
3. **Trade-offs** of the current design vs the obvious alternatives.
4. **Recommendation** — refactor / rewrite / leave alone, with rough cost in
   coding sessions. Bias toward refactor unless evidence justifies more.
5. Whether any of this warrants an **ADR** (and offer to draft it with $adr).

Do not make code changes in this review — it's analysis. If the design doc in
`docs/04 Design Docs/` is stale, note what should be updated.
