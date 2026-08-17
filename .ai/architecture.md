# Architecture

Two Daz Studio plugins from one source tree, buildable against two Daz Studio
generations. The value lives in the exporter; the tools plugin is a small
helper pane.

## The one-tree / two-SDK design

`DAZ_SDK_VERSION` (4 or 6) is the **only** switch (top-level `CMakeLists.txt`).
It decides:

- which SDK library layout is read (SDK4: `lib/x64`, SDK6: `x64/lib`)
- which Qt to build against (SDK4: the Qt 4.8 bundled *inside* the SDK —
  no separate Qt 4 install; SDK6: an external Qt 6 devkit matching the Qt in
  Daz Studio 6)
- whether `dzsdkmemory` is linked (SDK6 only)
- the DLL name prefix (SDK6 only loads plugins named `dsp_*.dll`; DS4 has no
  such rule)

It reaches the compiler as `DAZ_SDK_MAJOR_VERSION`, and the **only** file in
the tree that reads that define is `DazToHueExporter/compat/dth_compat.h`.
Everything else is generation-agnostic — that is the design invariant to
preserve: a new SDK difference gets a shim in `dth_compat.h` (or better, a
portable call pattern shared by both SDKs — see the header's comment for the
checked list of what does and does not need a shim), never an `#if` in a
feature file.

Build entry point: `build.ps1` (one flavor per invocation, output under
`build-sdk4/` / `build-sdk6/`). Toolchain/dependency facts: `gotchas-build.md`.

## The two plugins

### DazToHueExporter → `dth_exporter.dll` / `dsp_dth_exporter.dll`

A `DzAction` (`dth_exporter_action.h`) in File → DazToHue, exposing three
`Q_INVOKABLE` entry points so workflow scripts can drive it without the GUI:
`doExport` (the full character export), `doExportAnimation`,
`doExportAlembicGroomPoses`. GUI in `gui/dth_exporter_gui.*`. Registered in
`pluginmain.cpp` (`DZ_PLUGIN_*` macros; version in `version.h`).

Internal layout — one directory per concern:

- **`dth/`** — orchestration and output. `dth_exporter.cpp` runs the pipeline
  (below). `dth_writer.*` writes the `.dth` JSON manifest (`DzJsonWriter`):
  artifact paths, materials/textures, subdivision properties, joint
  orientations, geoshell/geograft/rigid-follower lists. `dth_logger.*`,
  `dth_settings.*`, `dth_static_helpers.*` are what they look like.
- **`daz/`** — scene-side preparation, all against live `DzScene` state.
  `daz_helpers.*` owns preprocessing: candidate-node gathering, hidden-node
  unparenting/reparenting, the subdivision level map, property
  locking/unlocking, undo. `daz_static_helpers.*` = stateless node queries
  (geograft/geoshell detection etc.).
- **`fbx/`** — the FBX side. `fbx_exporter.cpp` drives Daz's own
  `DzFbxExporter` (found by class name via `DzExportMgr`), then post-processes
  the file with the FBX SDK (`open_fbx_interface.*`) and OpenSubdiv
  (`open_subdivision_interface.*`).
- **`alembic/`** — the Alembic side, a merged-in derivative of the Sagan
  exporter (`alembic/sagan/**`). Reads viewport-cached geometry
  (`sagan/geometry/geometry.cpp`: `getCachedGeom()`) and writes through the
  Alembic SDK. `houdini_alembic_output_transformer.*` adapts axes/orientation
  for Houdini.
- **`compat/`** — `dth_compat.h`, the generation shim (see above).

### DazToHueTools → `dth_tools.dll` / `dsp_dth_tools.dll`

A `DzPaneAction` + `DzPane` (`dth_tools_action.h`) hosting one widget
(`gui/dth_tools_gui.*`): a node-visibility tree for the scene (the workflow
hides hair items before export; this pane is the convenient way to do it).
Slots use the string-based `SIGNAL()`/`SLOT()` macros — Qt4 has no
pointer-to-member connect, and some involved signals are protected
(`gotchas-build.md`).

## The export pipeline (`doExport`, `dth/dth_exporter.cpp`)

Order matters; several steps mutate scene state that later steps depend on and
that the final steps must restore:

1. **Preprocess** (`daz_helpers.cpp preprocessScene()`): gather candidate
   nodes, normalize degenerate SubD states, record the per-shape SubD map,
   lock the SubD properties, unparent hidden children of the selected root
   (`unparentHiddenNodes` — `DzFbxExporter` ignores visibility on fitted
   followers, so a hidden hair item stays in the FBX unless detached; the
   Alembic decoder checks `isVisible` itself).
2. **Alembic ROM** export (interactive updates enabled around it).
3. **FBX ROM** export: base mesh first; if any shape has real subdivision, a
   second pass restores the recorded viewport levels, exports the HD FBX, and
   `subdivideFbx` (OpenSubdiv) subdivides the base cage to carry skin weights
   to the HD mesh, replacing the base file.
4. **Reference frames** (skeleton FBX per requested frame).
5. **Write the `.dth` manifest** (`dth_writer`).
6. **Restore**: undo changes, reparent the hidden children, unlock SubD.

### The SubD invariant (why FBX and Alembic topologies match)

Both exporters are driven by the **viewport** properties `SubDIALevel` +
`lodlevel` — the render SubD level is never read anywhere:

- `generateSubdivisionLevelMap()` (`daz_helpers.cpp`) records the viewport
  level per shape at preprocess time.
- The Alembic pass reads the viewport-tessellated cache (`getCachedGeom()`),
  i.e. that same level.
- The FBX HD pass restores that same recorded map onto the shapes before
  `DzFbxExporter` writes (`setUserDefinedSubdivisionLevels()`), and the OSD
  weight-transfer subdivides by the same map
  (`open_subdivision_interface.h: SubdivideFbxScene`).
- Locking (`lockSubdivisionLevels()`) keeps the levels from drifting between
  the Alembic capture and the FBX export.

Degenerate combos are normalized up front (`processSubdivisionLevel()`):
High Resolution with SubD 0, or Base resolution with SubD ≠ 0, become plain
base — so `hasSubdivisions_` is true only for High Resolution + SubD > 0, and
only then does the HD FBX pass run.

Consequence for users: set the **viewport** SubD to the level you want
exported. Matching the render SubD is about fidelity to your Daz renders, not
about FBX↔Alembic parity.

## Plugin loading facts (both plugins)

- Entry points are resolved by plain C names (`getSDKVersion`,
  `getPluginDefinition`); the `.def` files provide the unmangled exports —
  the SDK's `DZ_PLUGIN_DEFINITION` macro alone emits C++-mangled names
  (`gotchas-sdk.md`).
- A DLL built against an SDK newer than the running Studio can fail with
  "requires a newer version of Daz Studio" — the stamped `DZ_SDK_VERSION`
  must be compatible, which is why the top-level CMake refuses to reuse a
  cached include path from a previous `-DDAZ_SDK_DIR`.
- Install = copy the DLL (and `.pdb` for crash symbols) into
  `<DAZStudio dir>\plugins\`, restart Studio, check Help → About Installed
  Plugins. Traps (locked DLL, silent copy failure): `gotchas-build.md`.
