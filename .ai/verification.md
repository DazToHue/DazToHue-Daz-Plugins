# Verification — the ladder, and lessons paid for

There is no test suite here, and there cannot easily be one: every entry point
takes live `DzNode`/`DzScene` state and only exists inside a running Daz
Studio. That does not lower the bar for what may be claimed — it raises the
bar for saying which claim was actually established.

## The three rungs

Never let one stand in for another. State the rung by name.

| Rung | What it proves | How |
| --- | --- | --- |
| **1. Compiles + links** | the code is valid for *that* generation, and every symbol it uses is really exported | build both flavors (`build.ps1 -SdkVersion 4` **and** `6`) |
| **2. Loads** | Daz accepts the DLL: right name, right SDK version, entry points resolvable | install, restart Daz, check **Help → About Installed Plugins** |
| **3. Runs correctly** | the export it produces is right | a real figure, a real export, an independent reference for the result |

Rung 1 catches a surprising amount (it caught `DzScript::call()` being declared
but not exported). Rung 1 says nothing about rung 2 (a plugin whose entry
points are exported under C++-mangled names compiles perfectly and never
loads). Rung 2 says nothing about rung 3.

## Lessons, each paid for with a real failure

- **A header is not an ABI.** Reading an SDK header proves what compiles,
  never what links or runs. `DzScript::call()` is declared in the DS6 SDK and
  unresolved at link.
- **"Cooks clean" is not "correct".** A DS6 export whose Alembic was wrong on
  essentially every frame (~1 cm everywhere, 10 cm peaks — the deferred-
  evaluation bug, `gotchas-sdk.md`) sailed through the entire Houdini import
  green. The pipeline validates structure, not geometry. Geometry needs a
  point-level comparison against an independent reference — in practice: the
  same scene exported by the other Daz generation, diffed frame-by-frame in
  hython.
- **Determinism is not validity.** Two runs of one build produced Alembics
  with ~20,000 corrupt points (NaN and 10³⁰-scale values) at frame 0 — in a
  region the import chain happens to drop — and a "new export ≡ previous
  export, 0.000000" check passed, because identical garbage equals identical
  garbage. A reproducible export can be reproducibly wrong. Sweep archives for
  non-finite/absurd values, and diff against an *independently produced*
  reference, not a sibling of the thing under test.
- **A visibility mechanism with no call site protects nothing.** A refactor
  kept `unparentHiddenNodes()` and its restore counterpart but dropped the one
  call that runs it; hidden hair silently leaked into every FBX while the
  Alembic (which checks visibility itself) stayed clean. When a guarantee
  matters, probe the *artifact* (list the FBX's nodes), don't trust the code
  path exists.
- **Attribute regressions by measurement, not narrative.** When two variables
  change between a clean result and a broken one, neither explanation is
  established until a run isolates one. (Measured here: plain `main` leaked
  hair with zero fix branches installed — settling which change broke it.)

## What a runtime verification looks like here

The pattern that worked, end to end and unattended: export a real multi-item
character through **dth-character-studio's Runner** (its Daz-side job plugin —
see `architecture.md` → ecosystem), then measure in `hython` —
point-level FBX-vs-Alembic parity at the rest frame through the production
import chain, a NaN/magnitude sweep of the Alembic archive, per-frame diff
against the other generation's export of the same scene, an FBX node listing
for inclusion/exclusion contracts, and a full cook of the character's hip.
Every claim in this repo's PRs of 2026-08-17 traces to one of those probes.
