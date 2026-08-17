# Configure + build the DazToHue plugins for one Daz Studio generation.
#
#   .\build.ps1 -SdkVersion 6 -SdkDir "<Daz Studio 6.25+ SDK>" -QtDir C:\Qt\6.10.3\msvc2022_64
#   .\build.ps1 -SdkVersion 4 -SdkDir "<DAZStudio4.5+ SDK>"
#   ... -Install    # copies the built DLLs into the Daz plugins folder (admin shell)
#
# Output:
#   build-sdk6\DazToHue*\Release\dsp_dth_exporter.dll + dsp_dth_tools.dll
#   build-sdk4\DazToHue*\Release\dth_exporter.dll     + dth_tools.dll
#
# The DS4 build needs no Qt install: Qt 4.8 ships inside the DAZStudio4.5+ SDK.

param(
    [ValidateSet("4", "6")]
    [string]$SdkVersion = "6",
    [string]$SdkDir,
    [string]$QtDir,                                     # Qt6 devkit root (SDK 6 only)
    [string]$DepsRoot = "D:\Development\Sources",       # third-party dependencies
    [string]$DazStudioDir,                              # for -Install
    [switch]$Install,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

if (-not $SdkDir) {
    $SdkDir = if ($SdkVersion -eq "6") {
        "D:\DAZ 3D\My DAZ 3D Library\Daz Studio 6.25+ BETA SDK"
    } else {
        "D:\DAZ 3D\My DAZ 3D Library\DAZStudio4.5+ SDK"
    }
}
if ($SdkVersion -eq "6" -and -not $QtDir) { $QtDir = "C:\Qt\6.10.3\msvc2022_64" }

foreach ($p in @($SdkDir, $DepsRoot)) {
    if (-not (Test-Path $p)) { throw "Not found: $p" }
}

# Prefer PATH cmake, fall back to the one bundled with VS Build Tools.
$cmake = "cmake"
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (-not (Test-Path $cmake)) { throw "cmake not found (PATH or VS Build Tools bundle)" }
}

$buildDir = "build-sdk$SdkVersion"
if ($Clean -and (Test-Path $buildDir)) { Remove-Item $buildDir -Recurse -Force }

$configureArgs = @(
    "-S", ".", "-B", $buildDir, "-G", "Visual Studio 17 2022", "-A", "x64",
    "-DDAZ_SDK_VERSION=$SdkVersion",
    "-DDAZ_SDK_DIR=$SdkDir",
    "-DRAPID_JSON_DIR=$DepsRoot\rapidjson",
    "-DIMATH_DIR=$DepsRoot\imath",
    "-DZLIB_DIR=$DepsRoot\zlib",
    "-DALEMBIC_SDK_DIR=$DepsRoot\alembic",
    "-DFBX_SDK_DIR=$DepsRoot\FBXSDK",
    "-DOPEN_SUBDIV_SDK=$DepsRoot\OpenSubdiv-3.4.4"
)
if ($SdkVersion -eq "6") {
    if (-not (Test-Path $QtDir)) { throw "Qt 6 devkit not found: $QtDir" }
    $configureArgs += "-DQt6_DIR=$QtDir\lib\cmake\Qt6"
}
if ($DazStudioDir) { $configureArgs += "-DDAZ_STUDIO_EXE_DIR=$DazStudioDir" }

& $cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw "configure failed" }

& $cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "build failed" }

$prefix = if ($SdkVersion -eq "6") { "dsp_" } else { "" }
$built = @(
    (Join-Path $buildDir "DazToHueExporter\Release\${prefix}dth_exporter.dll"),
    (Join-Path $buildDir "DazToHueTools\Release\${prefix}dth_tools.dll")
)
foreach ($dll in $built) {
    if (Test-Path $dll) { Write-Host ("built  " + (Resolve-Path $dll)) }
    else { Write-Warning "expected but missing: $dll" }
}

if ($Install) {
    if (-not $DazStudioDir) {
        $DazStudioDir = if ($SdkVersion -eq "6") { "C:\Program Files\DAZ 3D\DAZStudio6" } else { "C:\Program Files\DAZ 3D\DAZStudio4" }
    }
    $dst = Join-Path $DazStudioDir "plugins"
    if (-not (Test-Path $dst)) { throw "No plugins folder at $dst" }
    foreach ($dll in $built) {
        if (-not (Test-Path $dll)) { continue }
        Copy-Item $dll $dst -Force
        Copy-Item ($dll -replace "\.dll$", ".pdb") $dst -Force -ErrorAction SilentlyContinue
        Write-Host ("installed " + (Split-Path $dll -Leaf) + " -> $dst")
    }
    Write-Host "Restart Daz Studio to load the plugins."
}
