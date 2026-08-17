# DazToHue exporter and tools for Daz Studio

The primary goal of this project was to combine the FBX exporting capabilities of the [DazBridgeUtils](https://github.com/daz3d/DazBridgeUtils) with the alembic exporting capabilities of the [Sagan](https://www.daz3d.com/forums/discussion/428856/sagan-a-daz-to-blender-alembic-exporter) alembic exporter plugin, into a single exporter plugin.

As a result, this plugin is capable of exporting all of the files required by the [DazToHue](https://www.artstation.com/marketplace/p/BLM5K/daztohue) workflow with minimal user interaction.

## Credits

* Donald (aka TheMysteryIsThePoint) who very kindly shared the [Sagan](https://www.daz3d.com/forums/discussion/428856/sagan-a-daz-studio-to-blender-alembic-exporter/p1) alembic code and helped with the initial project setup.
* danielbui78, David-Vodhanel and raagaard for creating the [DazBridgeUtils](https://github.com/daz3d/DazBridgeUtils).
* jebbie for merging the code bases and providing numerous bug fixes.

## Prerequisites

- Windows, 64-bit, **Visual Studio 2022** (Build Tools are enough — MSVC v143)
- The matching **Daz Studio SDK**, installed via Daz Install Manager:
  - *DAZ Studio 4.5+ SDK* for the DS4 build — it bundles Qt 4.8, so **no separate
    Qt 4 install is needed**
  - *Daz Studio 6.x SDK* (beta) for the DS6 build
- For the DS6 build only: a **Qt 6.10.x msvc2022 x64 devkit** matching the Qt
  bundled in Daz Studio 6 (6.10.3 for DS 6.25). No Qt account needed:

### Third-party dependencies

Both flavors link the same set. `build.ps1` expects them under one root
(`-DepsRoot`, default `D:\Development\Sources`) in this layout:

| Dependency | Expected path | Notes |
| --- | --- | --- |
| RapidJSON | `rapidjson/include` | header-only |
| Imath | `imath/{include/Imath, lib/Imath.lib}` | build with `-DIMATH_LIB_SUFFIX=` so the lib is `Imath.lib` |
| zlib | `zlib/lib/zlibstatic.lib` | |
| Alembic | `alembic/{include, lib/Alembic.lib}` | static; needs Imath |
| OpenSubdiv 3.4.4 | `OpenSubdiv-3.4.4/build/lib/Release/osdCPU.lib` | CPU only (`-DNO_OPENGL=1 -DNO_TBB=1 …`), target `osd_static_cpu` |
| FBX SDK | `FBXSDK/{include, lib/vs20xx/x64/release}` | the `vs20xx` folder is found automatically |

Build everything with the **`/MD`** runtime (`-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`)
— the Daz SDK ships release-only libraries and the plugins compile `/MD /U_DEBUG`.

## Build

```powershell
# Daz Studio 6  ->  build-sdk6\...\Release\dsp_dth_exporter.dll
.\build.ps1 -SdkVersion 6 -SdkDir "<Daz Studio 6.25+ SDK>" -QtDir C:\Qt\6.10.3\msvc2022_64

# Daz Studio 4  ->  build-sdk4\...\Release\dth_exporter.dll
.\build.ps1 -SdkVersion 4 -SdkDir "<DAZStudio4.5+ SDK>"

# add -Install to copy the DLLs into the Daz plugins folder (admin shell)
.\build.ps1 -SdkVersion 6 -Install
```

Install manually by copying the built `.dll` (and optionally the `.pdb`, for
crash symbols) into `<DAZStudio dir>\plugins\`, then restart Daz Studio and check
Help → About Installed Plugins.

## A note on the DS4 build

Two things the DS4 flavor needs that the DS6 one does not, both handled by the
build:

- **No `/permissive-`.** Qt 4.8's headers use the `register` keyword, which C++17
  removed; conforming mode rejects `qsharedpointer_impl.h` and `qmetatype.h`
  outright. C++17 itself is fine (the code uses structured bindings), and this
  matches how the pre-merge DS4 project was configured.
- **Qt 4 has no pointer-to-member `connect()`.** Signal/slot connections use the
  string-based `SIGNAL()`/`SLOT()` macros, which work unchanged on Qt 6.
