# .ai — agent documentation

Deep-dive docs for AI coding agents (and curious humans) working on this repo.
`CLAUDE.md` at the repo root is the entry point and stays short; these files
hold the detail so a fresh session doesn't need to re-scan the codebase — or
re-pay for a lesson — to understand it. Assembled 2026-08 from the predecessor
tree's docs and the debugging sessions behind this repo's first PRs; update
them when the facts they state change — they are documentation, not
archaeology.

| File | Read it when… |
|---|---|
| [philosophy.md](philosophy.md) | **always — it is injected into every session** (with the Working rules) by `.claude/hooks/inject-agent-context.ps1`. Epistemic honesty over perceived helpfulness: what to do when the rules elsewhere don't cover the case. Model-agnostic. |
| [architecture.md](architecture.md) | you need the lay of the land: the two plugins, the one-tree/two-SDK design, the export pipeline, the compat layer, the SubD mechanism. |
| [conventions.md](conventions.md) | you branch, commit, open a PR, or install a build — the working rules and repo mechanics. |
| [gotchas-sdk.md](gotchas-sdk.md) | you touch anything that calls into the Daz SDK or Qt — measured DS4/DS6 facts (with Daz ticket numbers) that headers alone won't tell you. |
| [gotchas-build.md](gotchas-build.md) | the build breaks or you set up a machine — toolchain, dependency, and PowerShell traps. |
| [verification.md](verification.md) | before claiming anything works — the compiles/loads/runs-correctly ladder and the lessons each rung was paid for with. |

Ground rules for editing these docs:

- **Facts only, paths always.** Every claim should be checkable against a file;
  prefer `path/file.cpp` references over prose.
- **Missing beats wrong** (see [philosophy.md](philosophy.md)). A fact that
  can't be stated accurately is left out and flagged as unknown — a gap sends
  the next reader to the code, a confident error sends them down the wrong
  path.
- **No secrets, no personal/machine specifics.** This folder is public.
  (Default local paths like the SDK install locations live in `build.ps1`,
  where they are parameters — not here, where they would read as facts.)
- Keep CLAUDE.md the short version — if something is needed on *every* task it
  belongs there; if it's needed when working *in an area*, it belongs here.
- **Capture learnings in the PR that earned them.** A session that debugs a
  footgun or measures an SDK behavior folds it into the matching file right
  away — a lesson that only lives in a PR description or chat log is lost to
  the next session.
