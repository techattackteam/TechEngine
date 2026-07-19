# 💬 Prompt Library

Reusable prompts for working with AI as [[Technical Lead Charter|technical lead]].
Copy, fill the blanks, paste.

## Architecture review
```
Act as my technical lead for TechEngine. Review <system/file>. Focus on
architecture, not style. Give: (1) what's solid, (2) real risks with evidence
from the code, (3) the trade-offs of the current design, (4) whether this
warrants an ADR. Push back if I'm over-engineering or under-engineering.
```

## Rewrite vs refactor (Sprint 1)
```
For <subsystem>, help me decide keep / refactor / rewrite. Base it on the actual
code. Estimate: % that survives a rewrite unchanged, cost in coding sessions for
each option, and the biggest risk of each. Recommend one and justify it. Draft
the ADR.
```

## Feature decomposition
```
Break <feature> into an Epic → Stories → Tasks. Each task must fit one 2–6h
coding session and have a clear done condition. Flag research or ADRs needed
first.
```

## Code review of finished work
```
Review this diff/branch as a lead engineer. Correctness first, then design, then
maintainability. Rank findings by severity. Note anything that contradicts an
existing ADR.
```

## Research summary
```
Summarize <paper/technique> for a graphics engine. Give the core idea, what it
costs (memory/GPU), and the 2–3 concrete decisions it implies for TechEngine's
render graph.
```

## Weekly review helper
```
Here's what I did this week: <notes>. Help me write the weekly review: completed,
in-progress, blockers, next objective, and an honest sustainability check.
```
