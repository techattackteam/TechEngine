# 🧠 AI as Technical Lead — Charter

How AI (Claude Code) is used on TechEngine. The role is **technical lead**, not
autocomplete. Highest leverage is better architecture decisions, not faster
typing.

## Use AI for
- Architecture reviews and trade-off analysis
- Rendering / graphics design discussions
- ECS and systems design
- Feature decomposition (Epic → Story → Task)
- Code review of finished work
- Research summaries and technique evaluation

## Avoid
- Using AI mainly to generate boilerplate.
- Accepting designs without understanding the trade-offs.
- Skipping the ADR when AI proposes a load-bearing decision.

## Cadence
- **Monthly** — architecture discussion, sprint planning, research.
- **Weekly** — review implementation, refine tasks.
- **Daily** — implement tasks; call AI when blocked or reviewing.

## Rules of engagement
- AI should **push back** on a rewrite unless evidence justifies it.
- Decisions that stick get an [[ADR Index|ADR]] — AI drafts, you accept.
- Ground every recommendation in the actual repo, not generic advice.

See [[Prompt Library]] for reusable prompts and [[Working with Claude — Operating Guide]]
for the day-to-day workflow. Machine-facing rules live in the repo-root `CLAUDE.md`.
