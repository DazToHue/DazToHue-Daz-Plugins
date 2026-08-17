# PostToolUse hook (Bash|PowerShell): after any `git push`, verify the current
# branch has upstream tracking. Exit 2 fails the turn with the message shown to
# the agent, so a push from an untracked branch cannot end a session silently.
#
# Branch config only - origin itself is never touched here.

$ErrorActionPreference = "SilentlyContinue"

# Hook input arrives as JSON on stdin; we only care whether a push happened.
$stdin = [Console]::In.ReadToEnd()
if ($stdin -notmatch 'git\s+push') { exit 0 }

# Ignore pushes that delete a remote branch - there is nothing to track.
if ($stdin -match 'git\s+push[^"]*--delete') { exit 0 }

$branch = git rev-parse --abbrev-ref HEAD 2>$null
if (-not $branch -or $branch -eq "HEAD") { exit 0 }   # detached or not a repo

git rev-parse --abbrev-ref --symbolic-full-name "@{u}" *> $null
if ($LASTEXITCODE -ne 0) {
    [Console]::Error.WriteLine("Branch '$branch' has no upstream tracking. Set it now: git branch --set-upstream-to=origin/$branch (or push with -u). Branch config only - never reconfigure the remote.")
    exit 2
}

exit 0
