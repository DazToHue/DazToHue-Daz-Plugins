# Conventions & working rules

The non-obvious "how we do things here". CLAUDE.md has the short version; this
is the reference. This repo is young — rules land here as they are earned, and
a thin section means "no ritual exists yet", not "anything goes".

## Working rules (every task, before anything else)

This project has very few hands — the repo owner (mrpdean) merges, and
contributors work on it in spare time. Every round trip a human spends
re-stating something they already said is the expensive resource here.

**The definition of done: every point of the prompt is solved.** "The code
compiles" is a precondition, never a completion criterion. Treat the prompt as
a ticket and each point in it as an acceptance criterion; if a point cannot be
met, it is named in the reply as outstanding — that is the only honest
alternative to doing it.

- **Open with the parsed todo list.** On any non-trivial prompt, the first
  thing in the reply is the short list of what was parsed out of it — before
  the work starts. Close by ticking that list, naming anything not done and
  why. A multi-part prompt is a checklist; account for every clause.
- **Effort scales with the prompt.** Resolve ambiguity within what was asked
  by taking the wider reading and saying so — but never add adjacent work the
  prompt didn't name. A one-line fix is a one-line fix.
- **Every `gh`/`git` command must be shaped so it cannot need a second
  attempt.** The recurring killer is PowerShell 5.1 quoting. Any multi-line or
  quote-bearing text goes to a file first, then
  `gh pr create --body-file <path>` / `git commit -F <path>` — or a
  single-quoted here-string whose closing `'@` sits at column 0. Never inline
  fragile text into a command line.
- **State the verification rung with every claim** (`verification.md`):
  compiles+links / loads / runs correctly. Both flavors
  (`build.ps1 -SdkVersion 4` and `6`) must reach rung 1 before a PR claims
  the build is fine — when only one SDK is installed on the machine, say
  which flavor went unbuilt instead of implying both.

## Repo mechanics

- **Branches:** work branches off `main`, named by intent — `fix/…`,
  `docs/…` (measured examples: `fix/unparent-hidden-nodes`,
  `docs/measured-knowledge`). Push the branch to `origin` with upstream
  tracking before the session ends — enforced by
  `.claude/hooks/check-branch-upstream.ps1` (a `git push` from an untracked
  branch fails the turn). Branch config only — `origin` stays as it is, never
  reconfigure the remote.
- **`main` is merged by the repo owner.** Contributors open PRs; merging —
  and deleting the merged branch — is mrpdean's call. Closing your own
  superseded PR (with a comment saying why, linking the real fix) is fine and
  has precedent (PR #1).
- **There is no CI** (no `.github/` in the tree). Every gate is local: the
  two-flavor build, the install, the in-Studio checks. That raises the bar on
  honest reporting — nothing downstream will catch an untested claim.
- **PR descriptions carry the evidence.** The convention set by PRs #1–#4:
  state what was measured, on which Daz Studio version, and how attribution
  was established (see `verification.md` → "Attribute regressions by
  measurement, not narrative"). A claims table beats prose.
- **Capture learnings in the PR that earned them.** A debugged footgun goes to
  `gotchas-sdk.md` or `gotchas-build.md`, a lesson about claims to
  `verification.md`, a new ritual here, changed structure to
  `architecture.md` — in the same PR, while the evidence is fresh.
- **Cross-references between docs and open PRs go stale** — measured on this
  very branch: a doc claimed a fix was "in this tree" while the fix's PR sat
  unmerged, then the PR was closed and the claim became false. When a doc
  references code that only exists on an open PR, say "lands with PR #N";
  whoever merges either side fixes the wording.

## Building & installing (the daily loop)

- One flavor per `build.ps1` invocation; `-Clean` nukes that flavor's build
  dir. Full dependency layout: `README.md` + `gotchas-build.md`.
- **Installing over a running Daz Studio silently fails** — Studio holds the
  plugin DLL open. Close Studio first, then copy (the plugins folder under
  Program Files needs elevation), then verify the installed DLL's timestamp.
  `build.ps1 -Install` from an admin shell does the copy; verifying is still
  on you.
- Back up the currently-installed DLLs before overwriting them — a tested
  rollback is one file copy; a re-build of an old commit is not.
- The built `.pdb` next to the DLL is worth installing too: Studio crash dumps
  resolve through it.

## Code style

- Match the surrounding file: tabs, Daz SDK naming (`DzNode* node`), Qt
  string-based `SIGNAL()`/`SLOT()` connections (required by Qt4, work on
  Qt6 — `gotchas-build.md`).
- Generation differences go through `compat/dth_compat.h` or a portable call
  pattern — never a scattered `#if DAZ_SDK_MAJOR_VERSION` in feature code
  (`architecture.md`).
- Comment density in this tree is high where behavior was measured — a
  comment that cites a measurement or a ticket number
  (`gotchas-sdk.md` style) is worth more than one that paraphrases the code.
