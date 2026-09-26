---
description: Record papers Miguel has explicitly approved in Paper.md and move their Research.md cards to topic columns
argument-hint: "[approved paper titles, links, or cards]"
---

Record approved papers in the TechEngine research vault. Approved: $ARGUMENTS

Use this for paper approval, not for screening or automatic synchronization. Resolve
references such as "1 and 3" against the current conversation. A shortlist, review column,
or checked box alone is not approval. If the approved papers cannot be identified, ask which
ones I approve.

Follow `CLAUDE.md`. Paths below are relative to the engine root; `docs/` is a separate
repository. This workflow edits only `docs/05 Research/Research.md` and
`docs/05 Research/Paper.md`.

1. Read both files in full. Find each approved card and its source link. Use the paper's actual title from the card or source; do not derive it from a URL slug. If a link is an index, project page without an identifiable paper, or inaccessible source, resolve a stable paper link before recording it. Report any paper whose title or source remains uncertain.
2. Check `Paper.md` for an existing entry by title **and** source URL. Reuse an existing entry rather than duplicating it; preserve any notes beneath it. Resolve a title collision with a different paper before linking. For each new paper, add a `## <paper title>` heading and a `- <source URL>` line, matching the existing format.
3. Move each approved card from its pending or review column to the matching existing topic column in `Research.md`. Link the card to its `Paper.md` heading as `[[Paper#<paper title>|<paper title>]]`. Keep useful card notes. Do not move or validate unapproved or merely shortlisted cards. Leave rejected cards unless I explicitly reverse my rejection. If a card is already in a topic column, just repair a missing or incorrect link.
4. Check that every requested paper has exactly one `Paper.md` entry, its source link is present, and its board link resolves. Review the vault diff and run `git -C docs diff --check`; report any paper left unresolved. Do not commit or push unless I ask.

Do not run a bulk sync. Column position never grants approval.
