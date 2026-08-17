# Gotchas — the build, the toolchain, the dependencies

Measured 2026-08-14..17 on Windows 11 + VS 2022 Build Tools + CMake 3.31,
while merging and then maintaining the two-generation tree this repo builds.

- **Qt 4.8's headers use the `register` keyword, which C++17 removed — so the
  DS4 flavor must build WITHOUT `/permissive-`.** With conforming mode on, the
  SDK's own `QtCore/qsharedpointer_impl.h` and `qmetatype.h` fail with
  `C2760: syntax error: 'register' was unexpected here`, before any project code
  is even reached. **C++17 itself is fine** — the sources need it (structured
  bindings, `std::variant`) — it is only `/permissive-` that has to go. The
  top-level CMake applies `/permissive-` to the DS6 flavor only.
- **Qt 4 has no pointer-to-member `connect()`** — that syntax arrived in Qt5. The
  string-based `SIGNAL()`/`SLOT()` macros work unchanged on both Qt4 and Qt6.
  Second, independent reason: some of the signals involved
  (`QTreeWidget::itemChanged`, `DzScene::primarySelectionChanged`) are declared
  **protected**, and the pointer-to-member form cannot name them from outside
  the class at all (`C2248`).
- **A committed `*_ui.h` can only ever serve one Qt version.** AUTOUIC/AUTOMOC/
  AUTORCC generate `ui_*.h` / `moc_*.cpp` / `qrc_*.cpp` at build time with each
  Qt's own tools (all three support Qt4). Never commit a generated file.
- **Include what you use — the two SDKs' headers pull in different things.**
  A file that reached `<stdexcept>` transitively through the DS6 SDK headers
  failed on DS4 with `C2039: 'runtime_error': is not a member of 'std'`. A file
  that compiles for one generation is not evidence it compiles for the other.
- **Imath's default library name is versioned.** A stock build produces
  `Imath-3_1.lib`; configure it with `-DIMATH_LIB_SUFFIX=` (empty) or the link
  fails to find `Imath.lib`.
- **OpenSubdiv's expected path includes its in-tree `build/` folder**:
  `OpenSubdiv-3.4.4/build/lib/Release/osdCPU.lib`. Only the `osd_static_cpu`
  target is needed
  (`-DNO_OPENGL=1 -DNO_TBB=1 -DNO_CUDA=1 -DNO_OPENCL=1 -DNO_DX=1 -DNO_PTEX=1`).
- **The FBX SDK installer wants UAC and will hang or fail in a non-interactive
  shell.** It is an NSIS package: `7z x fbx<version>_fbxsdk_vs20xx_win.exe
  -o<dest>` extracts `include/` and `lib/` with no elevation at all.
- **The FBX library folder is named after the toolset and changes between SDK
  releases** (2020.0 shipped `vs2017`, 2020.3 ships `vs2019`). The CMake
  searches `vs2022 → vs2019 → vs2017` and prints which it found. Don't hardcode
  one back.
- **Everything must be built `/MD`.** The Daz SDK ships release-only libraries
  and the plugins compile `/MD /U_DEBUG` in every configuration; a dependency
  built `/MT` will fail to link or crash at runtime. Pass
  `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL` when building third-party
  libraries.
- **A running Daz Studio holds the plugin DLL open** — a copy into the plugins
  folder silently fails or is skipped until Daz is closed (measured: an
  elevated Copy-Item completed with the OLD file still in place). Verify the
  installed DLL's timestamp after installing; the plugins folder is under
  Program Files, so installing needs an admin shell.
- **Never pipe the build through `2>&1` in Windows PowerShell 5.1 — the build
  will report a failure it did not have.** During the DS4 configure, a Qt 4.8
  tool writes `Qt: Untested Windows version 6.2 detected!` to stderr; 5.1 wraps
  native stderr in an ErrorRecord, and with `$ErrorActionPreference = "Stop"`
  the script aborts before compiling anything. The log then ends on a Qt
  warning with no compiler error anywhere in it — that is the tell. To capture
  a build log, redirect stdout only and leave stderr alone.
