# TechEngine — Project Vault

This folder is an **Obsidian vault** and the "brain" of TechEngine. Open it by
pointing Obsidian at `C:\dev\TechEngine\docs`.

It is versioned in git alongside the engine so that design decisions evolve in
lockstep with the code that implements them.

## Core philosophy

| Layer | Tool | Purpose |
|-------|------|---------|
| Think | Obsidian (this vault) | Vision, architecture, research, planning |
| Execute | Obsidian Sprint board | Epics → Stories → Tasks |
| History | Git | What actually changed |
| Technical Lead | Claude Code | Architecture reviews, trade-off analysis, research |
| Studio Director + Lead Engineer | You | Direction and implementation |

> Planning happens in dedicated planning sessions. Implementation happens in
> coding sessions. The goal is to reduce decision fatigue and protect
> uninterrupted implementation time at a pace sustainable for years.

## Folder map

| Folder | Holds |
|--------|-------|
| `00 Dashboard` | Single at-a-glance status page. Start here. |
| `01 Vision` | Why TechEngine exists, principles, non-goals |
| `02 Roadmap` | Quarterly goals, monthly sprints, milestones |
| `03 Architecture` | ADRs, system overview, diagrams |
| `04 Systems` | Living docs per subsystem (renderer, ECS, resources…) |
| `05 Research` | Papers, SIGGRAPH notes, technique write-ups |
| `06 Sprints` | Sprint board, active sprint, backlog |
| `07 Journal` | Weekly reviews, sprint retrospectives |
| `08 AI` | Technical-lead charter, prompt library, review logs |
| `09 References` | External links and resources |
| `Templates` | Note templates (ADR, sprint, story, review, research) |

## Weekly rhythm (from the plan)

- **Sunday morning** — Weekly review (30–45 min) in `07 Journal`.
- **Sunday afternoon** — Long implementation session.
- **Last Sunday of month** — Sprint planning + retrospective.
