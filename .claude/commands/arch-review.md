---
description: Technical-lead architecture review of a system or file
argument-hint: "<system, path, or area to review>"
---

Act as my technical lead and review the architecture of: $ARGUMENTS

Focus on design, not style. Read the actual code first (and the relevant
`docs/04 Systems/` note and ADRs).

Deliver:
1. **What's solid** — worth keeping, don't touch.
2. **Real risks** — each with concrete evidence from the code (file:line) and a
   failure scenario, not vibes.
3. **Trade-offs** of the current design vs the obvious alternatives.
4. **Recommendation** — refactor / rewrite / leave alone, with rough cost in
   coding sessions. Bias toward refactor unless evidence justifies more.
5. Whether any of this warrants an **ADR** (and offer to draft it with /adr).

Do not make code changes in this review — it's analysis. If the system doc in
`docs/04 Systems/` is stale, note what should be updated.
