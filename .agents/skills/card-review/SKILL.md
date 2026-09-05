---
name: card-review
description: "Review a TechEngine card's implementation against its acceptance criteria, correctness, conventions, and architecture. Use for card review or pre-merge review of a card; report findings without editing or closing it."
---

# Card review

Invoke as `$card-review S5-T2`, optionally naming a PR or revision. With no ID,
infer the card from the current conversation, branch, and active sprint. Ask for
the ID if several cards fit. A supplied diff can still receive a code-only review;
state when the card's acceptance criteria could not be assessed.

Paths are relative to the engine root. Follow `AGENTS.md`, including build
ownership and existing authorization. Review only: do not edit files, update the
board, fix findings, commit, push, or close the card.

## Establish the scope

1. Read the card in the active sprint and its acceptance criteria. Identify the
   target PR, branch, working changes, or merged commit. State the revision and
   any uncommitted work included so the review has a clear boundary.
2. For branch work, compare the whole branch against its merge base with
   `origin/master`, including relevant staged, unstaged, and untracked changes.
   Do not review only the last commit. For an explicit PR or revision, use that
   target and keep unrelated local changes out.
3. Check the Dashboard's reconciliation stamp. Read the affected system's design
   note, then relevant ADR sections and amendments. Stale or missing documentation
   is uncertainty to resolve against evidence, not permission to invent a design.
4. Read `CONVENTIONS.md` and the changed files in full. Follow the callers, private
   helpers, and tests needed to understand the changed behavior.

## Review

- **Correctness first.** Trace the normal path and relevant failure paths. Check
  ownership, lifetime, bounds, state transitions, cleanup, and concurrency where
  the change uses them. Report concrete defects introduced or exposed by the work.
- **Acceptance and tests.** Check every done-condition against the implementation
  and available evidence. Distinguish implemented, unmet, and unverified conditions.
  Identify missing tests only when a specific behavior or failure needs coverage.
  For rendering, inspect supplied demo evidence and state what remains unverified.
- **Conventions.** Apply the judgment rules in `CONVENTIONS.md`, with surrounding
  style taking precedence. Avoid duplicating formatter output. Automated checks
  do not justify ignoring a correctness defect. An undecided convention is a
  decision gap, not a violation.
- **Architecture.** Check affected module boundaries, public surfaces, and accepted
  decisions. Flag contradictions and new load-bearing choices; do not redesign
  unrelated systems or silently amend an ADR.
- **Evidence.** Confirm findings in the actual execution path before reporting.
  A code finding needs an exact location and a triggering scenario. A convention
  finding needs the applicable rule. Label estimates and unresolved questions.

## Report once

Lead with actionable findings, ordered by severity. Use one short bullet per
finding: a plain title, a clickable `file:line`, and enough complete sentences to
explain the trigger, consequence, and smallest useful correction. Avoid repeating
the same finding under correctness, conventions, and architecture.

Report acceptance or design gaps separately only when there are any. Finish with
a short verification statement: what evidence was inspected and what was not run.
Do not turn static inspection into a claim that a build, test, or demo passed.

If no findings remain, say so briefly. Do not add praise, a diff recap, empty
sections, or a table repeating every satisfied criterion. Report every actionable
finding; keep speculative improvements out of the list.

## Follow-up review

Check the new diff and previously unresolved findings. Follow affected callers
again when the revision changes their behavior. Mark earlier findings resolved,
still present, or unverified without repeating their full explanation. Reopen
untouched areas only when new evidence warrants it.

A review request does not require a separate agent. Keep `$te-review` and
`$arch-review` available for users who request those narrower reviews.
