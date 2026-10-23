---
description: Write a system's capability brief with me as product owner - interview first, then draft
argument-hint: "<system, e.g. renderer, terrain, networking>"
---

Act as my product owner's analyst for TechEngine. We are writing the capability brief for:
$ARGUMENTS

A brief says **what** a system must support and **for whom**, never **how**. It is the input
the system's ADR decides against ([[Principles]] P3). I am the product owner, so I decide every
line; you interview, recommend and draft.

## Gate: check before anything else

1. **The system has a section** in `docs/01 Product/Capability Map.md`. If not, say so and stop:
   a system the map does not know is a `/triage` first.
2. **Is there already a brief** at `docs/01 Product/Capabilities/<System> - Brief.md`? If so,
   this is a **revision**: update it in place, and list what changed at the end.

## Gather

Read these before asking anything, so no question asks what is already decided:

- `docs/01 Product/Vision.md`, `Reference Game.md`, `Principles.md` and `Channel.md`.
- The **Capability Map**: its shared targets and the system's section.
- `docs/02 Roadmap/Roadmap.md`: the goals and stages that need this system, and its showcase.
- The system's design note and ADRs, **only for constraints already decided**. They are inputs,
  not questions to reopen.
- `docs/06 Sprints/Backlog.md` entries that name the system.

## Interview

- **Rounds of at most four questions**, one decision per question, my recommendation first, in
  plain words. Draft the defaults from the Reference Game and the map, and ask only where a real
  fork exists.
- **Product questions only**: who uses it, what they do, how much, how good, what is left out.
  Never data structures, algorithms or libraries. If a technical choice changes what users get,
  ask about the visible consequence ("can players see their edits within a second?"), not the
  mechanism.
- **Cover, in order**: users and scenarios · what the showcase must show · what the demo needs ·
  the numbers · what developers coming from Unity, Unreal or Godot will expect (say when that
  comes from memory) · what is out for now · what it depends on.
- **Stop** when every *Must* row and every number has an answer. Anything left open becomes an
  open question in the brief, with what it blocks.

## Write

1. Create or update `docs/01 Product/Capabilities/<System> - Brief.md` from
   `docs/Templates/Capability Brief Template.md`. **Status: draft** until I say it is agreed.
2. **Every answer I gave becomes a *Decided* row**, dated. Nothing I said is paraphrased into
   something stronger or weaker.
3. Update the system's section of the Capability Map: link the brief, and correct any row the
   interview changed. Say which rows changed.
4. **Check the brief against the product**, and flag each hit for my call:
   - a requirement TechAttack does not need (P1);
   - a requirement that puts game rules in the engine (P2);
   - a *Must* that cannot be shown on video (P4);
   - a number that breaks a shared target (60 fps at 1080p on a mid-range PC).
5. Keep it to about a page, with no section over ~30 lines.

## Report

Five lines on what the brief now says, its open questions, and which ADR it unblocks.

## Guardrails

- **No ADR, no design, no cards.** The ADR is `/adr`'s; cards are `/sprint-prep`'s.
- **Never decide for me.** A default you drafted and I did not confirm stays a question.
- **This runs attended only.** The unattended lane never writes a brief, because every line is
  a product decision.
- Vault only. Do not commit unless I ask.
