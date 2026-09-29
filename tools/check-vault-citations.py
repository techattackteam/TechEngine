#!/usr/bin/env python3
"""Checks the vault's `file:line` citations against an engine commit.

Mechanical validity only: the cited path exists at the commit and the cited lines are in
range. Whether a line still supports the sentence around it is a human read.

A citation is an inline code span holding `path:N` or `path:N-M`. It is checked at the
Dashboard's "Reconciled against" commit, unless an earlier code span in the same paragraph
names an engine commit (a hex sha or a tag, e.g. `681ddf6b` or `v1-reference`); then it is
checked at that commit, and reported if that commit is not in this clone. A path that is not
in the engine tree at all is looked up in the vault.

Usage:  python3 tools/check-vault-citations.py [--vault DIR] [--sha REV] [--exclude PREFIX ...]
Exit status is 1 when any citation is broken, 2 when the commit cannot be resolved.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

CODE_SPAN = re.compile(r"`([^`\n]+)`")
CITATION = re.compile(
    r"^(?P<path>[A-Za-z0-9_.\-/]*[A-Za-z0-9_\-]\.[A-Za-z0-9]+):(?P<first>\d+)(?:-(?P<last>\d+))?$")
HEX_SHA = re.compile(r"^(?=[0-9]*[a-f])[0-9a-f]{7,40}$")
TAG_LIKE = re.compile(r"^v\d[\w.\-]*$")
STAMP = re.compile(r"\*\*Reconciled against\*\*\s*\|\s*engine\s*`([0-9a-f]{7,40})`")
FENCE = re.compile(r"^\s*(```|~~~)")
BLOCK_START = re.compile(r"^\s*([-*+]|\d+\.|#+|\|)\s")


def git(engine, *arguments):
    return subprocess.run(["git", "-C", str(engine), *arguments], capture_output=True, text=True)


def resolvePath(paths, path):
    path = path.removeprefix("./")
    if path in paths:
        return [path]
    return sorted(candidate for candidate in paths if candidate.endswith("/" + path))


class Tree:
    def __init__(self, engine, commit):
        self.engine = engine
        self.commit = commit
        listing = git(engine, "ls-tree", "-r", "--name-only", commit)
        self.paths = set(listing.stdout.splitlines())
        self.lineCounts = {}

    def resolve(self, path):
        return resolvePath(self.paths, path)

    def lineCount(self, path):
        if path not in self.lineCounts:
            blob = git(self.engine, "cat-file", "-p", f"{self.commit}:{path}")
            self.lineCounts[path] = len(blob.stdout.splitlines())
        return self.lineCounts[path]


class VaultTree:
    def __init__(self, vault):
        self.vault = vault
        files = (path for path in vault.rglob("*") if path.is_file() and ".git" not in path.parts)
        self.paths = {path.relative_to(vault).as_posix() for path in files}

    def resolve(self, path):
        return resolvePath(self.paths, path)

    def lineCount(self, path):
        return len((self.vault / path).read_text(encoding="utf-8", errors="replace").splitlines())


def resolveCommit(engine, revision):
    result = git(engine, "rev-parse", "--verify", "--quiet", f"{revision}^{{commit}}")
    return result.stdout.strip() if result.returncode == 0 else None


def paragraphs(text):
    lines = text.splitlines()
    inFence = False
    block = []
    for number, line in enumerate(lines, 1):
        if FENCE.match(line):
            inFence = not inFence
            if block:
                yield block
                block = []
            continue
        if inFence:
            continue
        if not line.strip() or BLOCK_START.match(line):
            if block:
                yield block
            block = [(number, line)] if line.strip() else []
            continue
        block.append((number, line))
    if block:
        yield block


def check(tree, vaultTree, path, first, last):
    for source in (tree, vaultTree):
        matches = source.resolve(path)
        if len(matches) > 1:
            return f"ambiguous path, matches {', '.join(matches[:4])}{' ...' if len(matches) > 4 else ''}"
        if len(matches) == 1:
            count = source.lineCount(matches[0])
            if first < 1 or last < first:
                return f"malformed line range in {matches[0]}"
            if last > count:
                return f"line {last} is past the end of {matches[0]} ({count} lines)"
            return None
    return "path not found"


def main():
    engine = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--vault", type=Path, default=engine / "docs", help="vault checkout (default: docs/)")
    parser.add_argument("--sha", help="engine commit to check against (default: the Dashboard's stamp)")
    parser.add_argument("--exclude", action="append", default=[], metavar="PREFIX", help="skip vault paths starting with PREFIX")
    options = parser.parse_args()

    vault = options.vault.resolve()
    dashboard = vault / "00 Dashboard" / "Dashboard.md"
    if options.sha:
        stamp = options.sha
    else:
        found = STAMP.search(dashboard.read_text(encoding="utf-8")) if dashboard.is_file() else None
        if not found:
            print(f"error: no 'Reconciled against' stamp in {dashboard}", file=sys.stderr)
            return 2
        stamp = found.group(1)

    stampCommit = resolveCommit(engine, stamp)
    if not stampCommit:
        shallow = git(engine, "rev-parse", "--is-shallow-repository").stdout.strip() == "true"
        reason = "the clone is shallow; unshallow it and retry" if shallow else "it is not in this complete clone's history"
        print(f"error: cannot resolve engine commit {stamp}: {reason}", file=sys.stderr)
        return 2

    trees = {}
    vaultTree = VaultTree(vault)

    def treeFor(commit):
        if commit not in trees:
            trees[commit] = Tree(engine, commit)
        return trees[commit]

    checked = 0
    broken = 0
    for note in sorted(vault.rglob("*.md")):
        relative = note.relative_to(vault).as_posix()
        if ".git" in note.parts or any(relative.startswith(prefix) for prefix in options.exclude):
            continue
        for block in paragraphs(note.read_text(encoding="utf-8", errors="replace")):
            anchor = stampCommit
            anchorName = stamp
            for number, line in block:
                for span in CODE_SPAN.finditer(line):
                    token = span.group(1).strip()
                    if HEX_SHA.match(token) or TAG_LIKE.match(token):
                        commit = resolveCommit(engine, token)
                        if commit or HEX_SHA.match(token):
                            anchor = commit
                            anchorName = token
                        continue
                    citation = CITATION.match(token)
                    if not citation:
                        continue
                    first = int(citation.group("first"))
                    last = int(citation.group("last") or first)
                    checked += 1
                    if anchor is None:
                        broken += 1
                        print(f"{relative}:{number}: `{token}` at {anchorName}: that commit is not in this clone")
                        continue
                    problem = check(treeFor(anchor), vaultTree, citation.group("path"), first, last)
                    if problem:
                        broken += 1
                        print(f"{relative}:{number}: `{token}` at {anchorName}: {problem}")

    print(f"{checked} citations checked, {broken} broken (paths and line ranges only, not what the lines say)")
    return 1 if broken else 0


if __name__ == "__main__":
    sys.exit(main())
