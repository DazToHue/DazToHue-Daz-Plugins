# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository.

## What this is

Two C++ plugins for **Daz Studio**, serving the DazToHue (Daz Studio → Houdini)
workflow: **DazToHueExporter** exports everything the workflow needs from one
character in one run — an FBX ROM (via Daz's own `DzFbxExporter`, post-processed
with the FBX SDK + OpenSubdiv), an Alembic ROM (a merged-in derivative of the
Sagan exporter), skeleton reference frames, and a `.dth` JSON manifest tying it
together. **DazToHueTools** is a small pane for toggling node visibility before
export. One source tree builds both plugins for **two Daz Studio generations**
(DS4 and DS6); Windows x64 only.

Upstream sits **dth-character-studio** (separate repo): it generates the ROM
script that prepares the scene this exporter consumes, and its Runner plugin
can drive Daz — including this exporter's entry points — **unattended**.
Runtime verification goes through that path plus `hython` probes, not through
a human clicking Daz/Houdini (`.ai/architecture.md` → ecosystem).

## Commands

```powershell
.\build.ps1 -SdkVersion 4 -SdkDir "<DAZStudio4.5+ SDK>"          # → build-sdk4\...\Release\dth_exporter.dll + dth_tools.dll
.\build.ps1 -SdkVersion 6 -SdkDir "<DS 6.25+ SDK>" -QtDir <qt6>  # → build-sdk6\...\Release\dsp_dth_exporter.dll + dsp_dth_tools.dll
# -Install copies into the Daz plugins folder (admin shell, Daz Studio CLOSED)
# -Clean removes that flavor's build dir first
```

Dependency layout (`-DepsRoot`) and prerequisites: `README.md`. There is no
test suite and no CI — the verification ladder in `.ai/verification.md` is the
gate, and it runs on a human's machine and eyes.

The shell here is **PowerShell 5.1** (a Bash tool is also available); use the
right syntax for each. Never pipe a build through `2>&1` in 5.1 — a harmless
Qt stderr warning becomes a fake failure (`.ai/gotchas-build.md`).

## Architecture (short version — full: `.ai/architecture.md`)

- `DAZ_SDK_VERSION` (4|6) is the **only** generation switch (top
  `CMakeLists.txt`). The only file allowed to read it is
  `DazToHueExporter/compat/dth_compat.h` — new SDK differences get a shim
  there or a portable call pattern, never an `#if` in feature code.
- Exporter layout: `dth/` orchestration + `.dth` manifest writer, `daz/`
  scene preparation (candidate nodes, hidden-node unparenting, the SubD level
  map, locking, undo), `fbx/` DzFbxExporter driving + FBX SDK/OpenSubdiv
  post-processing, `alembic/sagan/` the Alembic side, `gui/` the dialog.
- Pipeline order in `dth/dth_exporter.cpp doExport()` is load-bearing:
  preprocess → Alembic ROM → FBX ROM (base, then HD + OSD weight transfer) →
  reference frames → `.dth` manifest → restore scene state.

### Invariants (do not break)

- **Both exporters are driven by the viewport SubD properties**
  (`SubDIALevel`/`lodlevel`) — the render SubD level is read nowhere. The
  recorded per-shape map + property locking is what keeps FBX and Alembic
  topology matched; preserve that mechanism when touching either side.
- **Scene mutations must be restored.** Preprocessing unparents hidden nodes
  and locks properties; `doExport` ends with undo/reparent/unlock. A
  preparation step without its restore counterpart — or a counterpart whose
  call got lost (that happened: PR #3) — corrupts the user's scene or the
  export.
- **Both flavors must build.** A change is not done when one generation
  compiles; the SDKs differ in headers, exports, and Qt
  (`.ai/gotchas-sdk.md`).

## Philosophy

**This project optimizes for epistemic honesty over perceived helpfulness. A
transparent limitation is always preferable to an incorrect answer presented
confidently — missing knowledge can be filled in, incorrect knowledge silently
spreads.** Never claim something works unless it was actually verified; say
what was tested and what wasn't; mark inferences as inferences.

**Evidence over hypothesis.** A plausible story is a hypothesis, not a
finding — when the source, the SDK headers, or a measurement can settle a
question, settle it that way before answering or acting. Repeatedly proven
here: a "does X matter?" question answered by reading the actual property
reads instead of folklore; a regression attributed by isolating runs instead
of narrative; a root cause that turned out to live in a different repo than
the plausible suspect. When evidence can't be obtained, say so and label the
hypothesis as one.

In this repo
that has a sharp edge: most behavior only exists inside a running Daz Studio,
and a header is not an ABI — state the rung of `.ai/verification.md` every
claim sits on. The full version is **`.ai/philosophy.md`**, injected into
every session (with the Working rules) by
`.claude/hooks/inject-agent-context.ps1`; it outranks any single rule here. It
is deliberately model-agnostic: it holds for any agent in this repo.

## Communication

- **Open a non-trivial response with the todo list parsed from the prompt**,
  before starting the work, and close by ticking it off — naming anything not
  done. A multi-part prompt is a checklist; account for every clause.
- **Always close every non-trivial response with a `TL;DR:` section.**
  The TL;DR exists for fast human scanning. It is a navigation aid, not a
  second summary. It must summarize the outcome, not repeat the explanation.

  Keep it to **3–8 short bullet points** (prefer one line each). Include only
  sections that actually apply.

  | Section | Content |
  | ------- | ------- |
  | ✅ Done | What was completed. |
  | ❌ Not Done | Requested work that was not completed. |
  | ⚠️ Assumptions | Anything inferred instead of explicitly known. |
  | 🧪 Needs Verification | Anything requiring manual testing or user confirmation — in this repo, usually "run it in Daz Studio". Never claim success until verified. |
  | 📌 Next Step | The single most useful next action for the user. |

  Never use sub-bullets in the TL;DR. If additional explanation is needed, it
  belongs in the main response above. The TL;DR is always the final section of
  the response.

## Key docs

- **`.ai/` — agent deep-dive docs. Read the relevant one BEFORE scanning
  source**: `philosophy.md` (how to work here at all), `architecture.md`
  (the two plugins, the pipeline, the compat layer), `conventions.md`
  (working rules, repo mechanics, the build/install loop),
  `gotchas-sdk.md` + `gotchas-build.md` (measured DS4/DS6/toolchain facts,
  with Daz ticket numbers), `verification.md` (the
  compiles/loads/runs-correctly ladder). Index: `.ai/README.md`.
  **Important learnings always land there, in the same PR that earned them.**
  CLAUDE.md stays the short version.
- `README.md` — prerequisites, dependency table, build, DS4 notes.
