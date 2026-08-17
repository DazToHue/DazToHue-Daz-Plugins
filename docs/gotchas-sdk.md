# Gotchas — the Daz Studio SDKs and Qt

Measured facts, not recollections. Gathered 2026-08-14..17 against the
**DAZStudio4.5+ SDK** and the **Daz Studio 6.25+ BETA SDK** (line numbers are
from those copies; the *lesson* survives a version bump even if a line number
doesn't), partly in this tree and partly in its single-source predecessor.
Where a bug was reported to Daz, the support ticket number is next to it.

- **A header is not an ABI: SDK6 DECLARES `DzScript::call()` and dzcore does not
  EXPORT it.** `dzscript.h:124` has `Q_INVOKABLE bool call(const QString&, const
  QVariantList&)`, the code compiles, and the link then fails with
  `LNK2019: unresolved external symbol … DzScript::call`. Found by building, not
  by reading — which is the general lesson: reading an SDK header proves what
  compiles, never what links. The exporter therefore invokes its Daz Script two
  different ways (`compat/dth_compat.h`): SDK4 calls the function by name, SDK6
  appends `return execute();` and runs the program.
- **SDK6's `DzScript` is ref-counted with a PROTECTED destructor**
  (`dzscript.h:104`) — it must be heap-allocated and released with `unref()`.
  SDK4's destructor is public and virtual (`dzscript.h:69`), and SDK4's `DzBase`
  has **no `ref()`/`unref()` at all** (they appear only in SDK6's
  `dzbase.h:69-70`). The disposal genuinely needs an `#if` in compat; the
  allocation does not (`new` is valid in both).
- **`DzFileIOSettings` also has a protected destructor on SDK6**
  (`dzfileiosettings.h:44` then `:50`). Heap-allocate it in both flavors — legal
  on SDK4, required on SDK6.
- **`DzSkeleton::getAllBones()` returns a different type per SDK — but both ship
  the out-parameter overload.** SDK4: `QObjectList getAllBones() const`; SDK6:
  `DzBoneList getAllBones() const`. Both also have
  `void getAllBones(DzBoneList&) const`, which needs no shim. Check for a shared
  overload like this BEFORE reaching for `#if`.
- **`DzNode::isVisibileInRender` is a typo that SDK4 never fixed.** SDK4 has only
  the misspelling (`dznode.h:196`). SDK6 has the correct `isVisibleInRender`
  (`dznode.h:350`) **and** keeps the typo as a deprecated alias explicitly marked
  `// TODO : SDK Next : remove` (`dznode.h:453`). Writing the typo'd name would
  compile on both today and break on the next SDK.
- **`DzScript::result()` is `QVariant` on SDK4 and `QJSValue` on SDK6.** Both
  have `.toString()`, so `auto` covers every call site — but SDK6's `dzscript.h`
  only FORWARD-DECLARES `QJSValue`, so using `auto` there needs `<QJSValue>`
  included first, or you get `C2027: use of undefined type`.
- **Headers that exist in one SDK only.** `dzgeometryshell.h` is SDK4-only —
  geoshells are identified by `className() == "DzGeometryShellNode"`, which
  works on both. `dzmessagebox.h` is SDK6-only.
- **Daz Studio 6 only loads plugins named `dsp_*.dll`** — an unprefixed DLL is
  silently skipped at startup, with nothing in the log to say why. DS4 has no
  such rule. The build applies the prefix from `DAZ_SDK_VERSION`.
- **Daz resolves plugin entry points by their PLAIN C names** —
  `getSDKVersion` / `getPluginDefinition` — on both generations. The `.def`
  files do that job; verified with `dumpbin /exports` on both built DLLs. If a
  plugin fails to load with *"could not locate the getSDKVersion() function"*,
  check the exports first: the SDK's own `DZ_PLUGIN_DEFINITION` macro emits
  C++-MANGLED names, and something (a `.def` file, or an `extern "C"`
  pre-declaration) has to fix that.
- **`DzFbxExporter` with `doAnims=true` ZEROES keyed morph channels while it
  writes, and the mesh it captures is baked in that state.** (Measured
  2026-08-16/17 on DS 4.24.0.4 and DS 6.25.2026.14722; Daz ticket **503954**.)
  During `writeFile()` the exporter drives keyed morph channels — including down
  to 0 — then restores every value before returning, so a before/after snapshot
  sees nothing. The exported mesh has the character's keyed shape morphs
  flattened. `doAnims` is the trigger; `doMorphs` makes no difference; the
  Alembic path is unaffected (it never goes through `DzFbxExporter`).
- **It is NOT only the keyed channels — the unkeyed auto-follow copies go to
  zero too, and they are the majority.** Same measurements: 1312 properties
  moved during the write; 18 were driven below a non-zero starting value, and
  only **5** of those had keys. The other **13** are the unkeyed auto-follow
  copies of the same morphs on every fitted item — clothing, geograft,
  geoshell. A protection keyed on `DzProperty::hasKeys()` covers the master and
  leaves every follower exposed. Characters without keyed shape dials in the
  ROM are unaffected — which is why only some characters ever showed the bug.
  The mitigation lives UPSTREAM, not in this tree: dth-character-studio's ROM
  generation guarantees every walked morph evaluates to 0 at frame 0
  (polynaut/dth-character-studio#873), which makes the baker's drive-to-zero a
  no-op. A structural workaround in the exporter (mesh export with
  `doAnims=false`, animation injected afterwards) was prototyped and closed
  unmerged once that root cause was found — see PR #1.
- **`DzProperty::lock()` is SELECTIVE, and the mechanism is unknown.** The
  predecessor tree worked around the baker by locking every numeric property
  during `writeFile()`: the drive-to-zero was stopped entirely, yet the same
  locked dials still moved ~1% (magnitude matching their ROM keys — an
  inference, nobody traced the caller). So somewhere inside closed-source
  dzcore, `lock()` gates one write path and not another. Remember that before
  building anything on `lock()`.
- **`DzProperty::currentValueChanged()` is private in the DS4 SDK and public in
  the DS6 one — and the string-based `SIGNAL()` macro connects to it on both.**
  DS4 declares it under `#ifndef Q_MOC_RUN / private:` (`dzproperty.h:233`);
  DS6 puts it in a plain `Q_SIGNALS:` block (`dzproperty.h:417`). MOC registers
  it either way, and `SIGNAL()` resolves by string at runtime, so C++ access
  control never enters into it. (This is how the morph-zeroing was measured: a
  watcher connected to every property during export.)
- **Daz Studio 6 DEFERS mesh evaluation after `dzScene->setFrame()` —
  `getCachedGeom()` can lag the scene by frames, and `processEvents()` does not
  flush it.** (Measured 2026-08-17, DS 6.25.2026.14722; Daz ticket **503955**.)
  `getCachedGeom()` only holds what the last *completed* evaluation produced;
  DS4 evaluates synchronously enough that a per-frame scrub-and-read exporter
  worked for years and silently broke on DS6 — every exported Alembic frame was
  wrong (median ~1 cm, peaks 10 cm). The fix is
  `DzObject::forceCacheUpdate(node)` before every read — declared identically
  on both generations (`dzobject.h`), and virtual, so it cannot hit the
  `DzScript::call()` missing-export trap. The fix lands with PR #2
  (`fix/ds6-alembic-stale-mesh`, touching
  `alembic/sagan/decoder/alembic_node_decoder.cpp`); until that merges, `main`
  still has the stale-read behavior on DS6.
- **`forceCacheUpdate()` on a STRAND-BASED HAIR node hangs DS6** (>8 minutes at
  0% on an operation that takes ~2 s without it; Daz ticket **503956**), and a
  DS6 session later crashed fatally at groom-archive creation in the same area.
  The force is therefore confined to the ROM frame loop and skips SBH nodes;
  the groom-poses path is left untouched. (Same PR #2 as above.)
- **The DS6 SDK is beta.** A rebuild against the final SDK may be needed when it
  goes GA, and an SDK newer than the oldest Daz Studio you support can reference
  `dzcore` exports that Studio lacks — which surfaces as *"requires a newer
  version of Daz Studio"* from a Studio that looks new enough.
