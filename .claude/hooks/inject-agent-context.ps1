# SessionStart hook: inject .ai/philosophy.md and the Working rules section of
# .ai/conventions.md into every session's context.
#
# Why: .ai/* is read-on-demand, and "on demand" means the agent decides - which
# is the exact failure for the two documents that must hold on EVERY task.
# Reads the live files, so editing a doc updates the injection.

$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent

$philosophy = Join-Path $root ".ai\philosophy.md"
$conventions = Join-Path $root ".ai\conventions.md"

if (Test-Path $philosophy) {
    Get-Content $philosophy -Raw
    Write-Output ""
}

if (Test-Path $conventions) {
    # Extract "## Working rules ..." up to (not including) the next "## " heading.
    $lines = Get-Content $conventions
    $out = New-Object System.Collections.Generic.List[string]
    $in = $false
    foreach ($line in $lines) {
        if ($line -match '^## Working rules') { $in = $true }
        elseif ($in -and $line -match '^## ') { break }
        if ($in) { $out.Add($line) }
    }
    if ($out.Count -gt 0) {
        Write-Output "# Working rules (from .ai/conventions.md)"
        Write-Output ($out -join "`n")
    }
}

exit 0
