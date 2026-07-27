# ADR-012 — Vault repository split (`docs/` as its own repo)

- **Status:** Accepted — 2026-07-27
- **Date:** 2026-07
- **Deciders:** Miguel (Lead Engineer), with AI as technical lead
- **Related:** [[ADR-009 — Branching strategy & merge rules]] §2 (protection on the engine
  `master` — this ADR narrows *what it governs*, see §5) ·
  [[ADR-008 — v2 build & testing baseline]] §9 (CI-minute budget) · CLAUDE.md "What this
  project is" (vault at `docs/`, unchanged by this ADR)
- **Task:** decide whether board/vault state stays versioned inside the engine repo.

## Context

The vault is currently a folder in the engine repo, so every vault edit is a change to a
**ruleset-protected** branch and must land via PR. Three costs have now been observed, not
predicted:

**1. The Done transition has no home.** A card moves Review → Done *after* its PR
squash-merges — at which point the branch is gone and `master` is protected. Every merge
orphans a small vault edit that must either open its own PR or ride an unrelated task's PR.
Observed on S2-T12.

**2. Concurrent tasks will conflict on the board.** The working pattern is explicitly not
sequential — a deep task sits in review while a light task starts on a light day. Two live
branches then edit the same regions of `Sprint Board.md` (cards leaving To Do, arriving in
In Progress). `Sprint Board.md` is already the most-churned file in the repo: **12 of 24
commits**.

**3. Docs-only changes burn a full CI matrix.** All 8 required checks run regardless of
what changed. S2-T12 was 24 files, **+409/−237, zero code**, and cost a full run
(≈22 billed min; ADR-008 §9's ~2,000 min/mo budget is live, Windows 2×). Plain
`paths-ignore` cannot fix this: required checks that never report block a protected PR
forever.

**The counter-argument was measured and does not hold.** The reason to keep docs in-repo is
atomicity — an ADR and the code it governs landing in one commit. On the three real engine
task PRs that is simply not what happens:

| PR | code files | board/backlog | **other docs** |
|---|---|---|---|
| T2/logger core (#8) | 12 | 2 | **0** |
| T3/channels (#9) | 8 | 2 | **0** |
| T6/Clock (#10) | 9 | 1 | **0** |

Design notes and ADRs are written in **planning** sessions; code lands in **task** PRs. The
only artifact they share is the board card, and a card move is not semantically coupled to
the code it describes. The atomicity being protected is not being used.

## Decision

**`docs/` becomes its own git repository, checked out in place and ignored by the engine
repo.** Not a submodule.

### 1. Layout

- `docs/` is a separate repository (`TechEngine-vault`, private, on GitHub for backup),
  cloned to `C:\dev\TechEngine\docs`.
- The engine repo lists `docs/` in `.gitignore`. Git treats it as absent.
- **The path does not change.** Obsidian still opens `C:\dev\TechEngine\docs`; every
  `docs/…` reference in `CLAUDE.md` and `.claude/commands/*` still resolves.
- **A root `.ignore` containing `!docs/` is mandatory, not optional.** ripgrep — which backs
  Claude's `Grep` and most editor search — honours `.gitignore`, so ignoring `docs/` makes
  every repo-root search **silently skip the whole vault**: no error, a result set
  indistinguishable from "no matches". `.ignore` is read by ripgrep and **not** by git, so
  `!docs/` re-includes the vault for search while git still ignores it. Verified both
  directions 2026-07-27 (`git check-ignore` still reports `docs/` ignored; a root search
  returns vault *and* code hits). Ship this in the same commit as the `.gitignore` entry —
  without it the split quietly degrades the vault's only purpose, being read.

### 2. Vault repo rules — deliberately none

No branch protection, no required checks, no PR. Commit and push to its `master` directly.
The vault has no build to break and no consumer but us; ceremony there is pure tax
(ADR-009's own "ceremony with no payoff" test).

### 3. Engine repo — unchanged

ADR-009 continues to govern it in full: protected `master`, PR-only, squash + linear
history, required checks. Nothing about the branching model changes.

### 4. Cross-reference direction

The vault points **at** the code (`file:line`, PR numbers on cards); the engine repo does
not point back. A card already records `→ PR #9`, which survives the split intact.

### 5. Relationship to ADR-009 §2

ADR-009 §2 reads "every change lands via PR." That was written when the vault was part of
the repo it governs. This ADR does **not** reverse the clause — it removes the vault from
that repo, so §2 continues to apply, unweakened, to everything remaining. Engine changes
still have exactly one path to `master`.

**Ratified as a narrowing, not a partial supersession — Miguel, 2026-07-27.** So there is
**no row** for this in [[ADR Index]] → *Partial supersessions*, and ADR-009 stays Accepted
with its scope intact. Recorded explicitly because the distinction is invisible later: a
reader finding vault commits outside the PR flow would otherwise reasonably conclude §2 had
been quietly broken.

### 6. The reconciliation stamp — skew is made visible, structurally

Chosen over relying on the weekly drift check alone — **Miguel, 2026-07-27.** The retro that
diagnosed this failure concluded the fix must be structural, and a habit is what already let
a stale `CLAUDE.md` mislead a full sprint.

- **The vault records the engine commit it was last reconciled against.** On [[Dashboard]],
  the note read first every session:
  `**Reconciled against:** engine <sha> (YYYY-MM-DD)`
- **The ceremony writes it** — `/weekly-review`, or `/sprint-plan` on the weekend it absorbs
  the review. The stamp *is* the drift check's output: it is only advanced once the check has
  actually run, never as a formality.
- **How it is read:** compare the stamp against `origin/master`. If the engine has moved
  past it, design notes are **suspect until the next check** — say so when grounding an
  answer (CLAUDE.md rule 2) rather than citing them as current.

**This makes skew visible; it does not prevent it.** A vault one commit behind and one twenty
commits behind look the same to the mechanism — the distance is the signal, and judgment
still applies. That is the honest ceiling of a stamp, and it is still strictly better than
inferring skew from nothing.

## Consequences

**Positive**

- The post-merge Done edit becomes a direct commit. The orphan-edit problem disappears
  rather than being scheduled around.
- Concurrent task branches cannot conflict on board state, because board state is not on
  those branches.
- Docs changes stop triggering the engine workflow — removes the ≈22 min/docs-PR waste and
  makes the `paths-ignore` / required-checks trap moot.
- Engine `git log` becomes code-only and more readable.
- Vault history stops being squashed into task commits; it gets its own honest granularity.

**Negative / open**

- **⚠️ Vault/code version skew — the main cost of this ADR.** In-repo, a checkout gives a
  vault and a tree at *the same commit*: a design note and the code it describes are
  consistent **by construction**. Split, the two HEADs move independently, so the vault can
  describe a shape the code has moved past with **no signal that it has**. This is the
  failure mode that bites hardest, because CLAUDE.md rule 2 makes the design note the
  *starting point* for grounding an answer — a skewed vault is confidently wrong ground
  truth. Its ancestor is already on record: the Sprint 01 retro
  ([[2026-07-25 Sprint 01 Retrospective]]) found a stale `CLAUDE.md` had misled every
  session for a whole sprint, silently, and concluded the drift check must be a *scheduled
  ritual* precisely because the failure is self-reinforcing.
  **Mitigated, not removed,** by the reconciliation stamp (Decision §6): skew becomes visible
  instead of inferred. The residual cost stands — the stamp reports distance, not whether any
  particular note actually drifted, so grounding still needs judgment when the engine has
  moved past it.
- **Two repos to clone, push, and back up.** A fresh engine clone has no vault until the
  vault is cloned separately — a README line, and a real papercut on a new machine. This is
  the *one-time* form of the skew above.
- **No enforced review on vault changes.** Nothing stops a bad vault edit. Mitigated by
  `/vault-clean` and the ceremony drift checks, not by CI.
- **`git log` in the engine no longer shows the card's *content* beside the code** — but the
  **linkage survives both ways**, which an earlier draft of this ADR understated. Squash-merge
  subjects are named from the branch, and the branch carries the card ID
  (`S2-T12/land-vault (#11)`, `T6/Clock (#10)`, `T3/channels (#9)`), so commit → card is one
  lookup; card → commit is the `→ PR #9` annotation the wrap step already writes. What is
  genuinely lost is **historical correlation**: "what did the board look like when T3 landed"
  has no shared commit graph to answer it and falls back to matching timestamps across two
  repos. Rarely needed, but unrecoverable when it is.
- **`/task-start` and `/task-wrap` must retarget their commits** at the nested repo, and
  any future command that writes the vault must know which repo it is in. One-time cost,
  but a permanent thing to remember when authoring commands.
- **Ignore-aware tooling goes blind on the vault unless told otherwise.** Not hypothetical
  and not "later" — ripgrep skips `docs/` on day one, silently, which is why the root
  `.ignore` is part of the Decision (§1) rather than a risk noted here. Residual exposure:
  any *other* tool that reads `.gitignore` but not `.ignore` will still miss the vault, and
  will do so quietly. The rule this leaves behind: **when a tool can't find something in the
  vault, suspect the ignore rules before concluding it isn't there.**

## Alternatives considered

- **Status quo — vault stays in the engine repo.** Rejected: it is the source of all three
  observed costs, and the atomicity it buys is measurably unused (see Context).
- **Git submodule.** Rejected, and this is the important rejection. A submodule pins the
  parent to a **specific vault commit**, so every vault edit leaves the pointer stale, and
  advancing the pointer is a commit in the engine repo — against protected `master`, via
  PR. That trades a board-edit PR for a pointer-bump PR: the same friction, renamed. Worse,
  skipping the bumps means a fresh clone checks out an **old vault**, which is precisely the
  poisoned-ground-truth failure CLAUDE.md rule 2 exists to prevent, and which the Sprint 01
  retro already recorded once (a stale `CLAUDE.md` misled every session for a sprint).
- **Separate repo in a sibling directory** (`C:\dev\TechEngine-vault`). Rejected: same
  benefits, but every `docs/…` path in `CLAUDE.md` and the commands breaks, for no gain over
  an in-place nested clone.
- **Move board state to GitHub Projects, keep the rest of the vault in-repo.** Rejected
  *for now* — it solves items 1 and 2 but not 3, splits planning across two homes, and
  costs the `[[wikilink]]` from card to design note, which is the vault's whole advantage
  over an issue tracker. Revisit if the split below fails to help.

## What would move this decision

- **Skew biting through the stamp** — a session grounded on a design note the code had moved
  past *despite* §6 being in place. That means the stamp is being advanced as a formality, or
  read and ignored; either way the structural mitigation has failed and the split itself
  should be reopened. Reverting is cheap: the vault is one directory and its history can be
  grafted back.
- **A second contributor** — review on vault changes stops being optional, and the "no
  ceremony" rule in §2 needs revisiting.
- **Board state outgrowing a markdown file** (genuine parallel work in flight, or wanting
  queries over cards) → GitHub Projects or task-per-note with Bases, decided on its own
  evidence rather than folded in here.
- **CI gaining a docs job** (link-checking, vale, a vault linter) — the vault would then
  have a build to break, and §2's "no ceremony" premise expires.

> Add to [[ADR Index]]. Once Accepted, treat as immutable — supersede with a new ADR.
