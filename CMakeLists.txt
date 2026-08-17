#**********************************************************************
#	Derived from the Daz Studio SDK CMake template.
#	Copyright (C) 2002-2026 Daz 3D, Inc. All Rights Reserved.
#
#	This file may be used only in accordance with the Daz Studio SDK
#	license provided with the Daz Studio SDK.
#
#	See http://www.daz3d.com to contact DAZ 3D or for more
#	information about the Daz Studio SDK.
#**********************************************************************
#
# ONE source tree, two Daz Studio generations.
#
# `DAZ_SDK_VERSION` (4 or 6) is the only switch. It decides:
#   - which SDK library layout to read      (SDK4: lib/x64, SDK6: x64/lib)
#   - which Qt to build against             (SDK4: Qt 4.8 bundled IN the SDK,
#                                            SDK6: an external Qt 6 devkit)
#   - whether dzsdkmemory is linked         (SDK6 only, as before the merge)
#   - the DLL name prefix                   (SDK6 only loads plugins named dsp_*)
# and is passed to the compiler as DAZ_SDK_MAJOR_VERSION, which the three
# shims in DazToHueExporter/compat/dth_compat.h key off. Everything else in
# the tree is generation-agnostic.
#
# Configure examples (or just use ..\build.ps1):
#   cmake -S . -B build-sdk6 -G "Visual Studio 17 2022" -A x64 ^
#       -DDAZ_SDK_VERSION=6 -DDAZ_SDK_DIR="<Daz Studio 6.25+ SDK>" ^
#       -DQt6_DIR="C:/Qt/6.10.3/msvc2022_64/lib/cmake/Qt6" ...
#   cmake -S . -B build-sdk4 -G "Visual Studio 17 2022" -A x64 ^
#       -DDAZ_SDK_VERSION=4 -DDAZ_SDK_DIR="<DAZStudio4.5+ SDK>" ...

cmake_minimum_required( VERSION 3.22.0 )

set( CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING INTERNAL FORCE )

project( "DazToHue" )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# ─── The generation switch ───────────────────────────────────────────────────

set( DAZ_SDK_VERSION "6" CACHE STRING "Target Daz Studio SDK major version (4 or 6)" )
set_property( CACHE DAZ_SDK_VERSION PROPERTY STRINGS "4" "6" )
if( NOT DAZ_SDK_VERSION STREQUAL "4" AND NOT DAZ_SDK_VERSION STREQUAL "6" )
	message( FATAL_ERROR "DAZ_SDK_VERSION must be '4' or '6', got '${DAZ_SDK_VERSION}'" )
endif()

message( STATUS "Building the DazToHue plugins for Daz Studio ${DAZ_SDK_VERSION}" )

if( NOT WIN32 )
	message( FATAL_ERROR "This build currently targets Windows only." )
endif( NOT WIN32 )

if( NOT CMAKE_SIZEOF_VOID_P EQUAL 8 )
	message( FATAL_ERROR "Configure a 64-bit build (-A x64)." )
endif()

set( DZ_LIB_SUFFIX ".lib" )
set( DZ_BIN_SUFFIX ".dll" )
set( DZ_LIB_PREFIX "" )
set( UTIL_EXT ".exe" )
set( DZ_PLATFORM x64 )
set( DZ_MIXED_PLATFORM x64 )

# Daz Studio 6 only loads plugins named dsp_*.dll - without the prefix the DLL
# is silently skipped at startup. Daz Studio 4 has no such requirement.
if( DAZ_SDK_VERSION STREQUAL "6" )
	set( DZ_PLUGIN_PREFIX "dsp_" )
else()
	set( DZ_PLUGIN_PREFIX "" )
endif()

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

cmake_policy( SET CMP0063 NEW )
set( CMAKE_CXX_VISIBILITY_PRESET hidden )
set( CMAKE_C_VISIBILITY_PRESET hidden )
set( CMAKE_VISIBILITY_INLINES_HIDDEN 1 )

# The SDK ships release-only libraries - use /MD and undef _DEBUG in every config.
add_compile_options( "/MD" "/U_DEBUG" )

if( DAZ_SDK_VERSION STREQUAL "6" )
	# Prevent crash on launch with mixed VS runtimes
	add_definitions( -D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR )
	# Until use of DZ_QT_MIGRATION is removed
	add_definitions( -DDZ_QT_MIGRATION )
endif()

if( MSVC )
	string( APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus" )
	# /permissive- for SDK6 only. Qt 4.8's headers use the `register` keyword,
	# which C++17 removed - conforming mode rejects them outright
	# (qsharedpointer_impl.h, qmetatype.h). The pre-merge DS4 project built the
	# same way: LanguageStandard stdcpp17 with ConformanceMode Default.
	if( DAZ_SDK_VERSION STREQUAL "6" )
		string( APPEND CMAKE_CXX_FLAGS " /permissive-" )
	endif()
endif( MSVC )

# ─── Daz Studio SDK ──────────────────────────────────────────────────────────

set( DAZ_SDK_DIR "" CACHE PATH "Path to root of the Daz Studio SDK (4.5+ or 6.x)" )
if( NOT DAZ_SDK_DIR )
	message( FATAL_ERROR "Missing path to the Daz Studio SDK - set DAZ_SDK_DIR." )
endif()

# SDK6 moved the platform directory above lib: x64/lib/dzcore.lib.
# SDK4 layout is lib/x64/dzcore.lib.
if( DAZ_SDK_VERSION STREQUAL "6" )
	set( DAZ_SDK_LIB_DIR "${DAZ_SDK_DIR}/${DZ_MIXED_PLATFORM}/lib" )
	set( DAZ_SDK_BIN_DIR "${DAZ_SDK_DIR}/${DZ_MIXED_PLATFORM}/bin" )
else()
	set( DAZ_SDK_LIB_DIR "${DAZ_SDK_DIR}/lib/${DZ_PLATFORM}" )
	set( DAZ_SDK_BIN_DIR "${DAZ_SDK_DIR}/bin/${DZ_PLATFORM}" )
endif()

# A plain variable ON PURPOSE (not CACHE): `set(... CACHE)` never overwrites an
# existing cache entry, so a cached include path would silently keep compiling
# against a PREVIOUS -DDAZ_SDK_DIR's headers after a reconfigure - which is how
# you ship a DLL stamped with the wrong DZ_SDK_VERSION and get "requires a newer
# version of Daz Studio" from a Studio that is in fact new enough.
set( DAZ_SDK_INCLUDE_DIR "${DAZ_SDK_DIR}/include" )
if( NOT EXISTS ${DAZ_SDK_INCLUDE_DIR} )
	message( FATAL_ERROR "Daz Studio SDK includes not found at ${DAZ_SDK_INCLUDE_DIR} - check DAZ_SDK_DIR." )
endif()

set( DAZ_SDK_DZCORE_LIB "${DAZ_SDK_LIB_DIR}/${DZ_LIB_PREFIX}dzcore${DZ_LIB_SUFFIX}" )
if( NOT EXISTS ${DAZ_SDK_DZCORE_LIB} )
	message( FATAL_ERROR "dzcore not found at ${DAZ_SDK_DZCORE_LIB} - check DAZ_SDK_DIR/DAZ_SDK_VERSION." )
endif()

add_library( dzcore SHARED IMPORTED )
set_property( TARGET dzcore APPEND PROPERTY IMPORTED_IMPLIB ${DAZ_SDK_DZCORE_LIB} )
set_property( TARGET dzcore APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DAZ_SDK_INCLUDE_DIR}" )

# dzsdkmemory: linked for SDK6 only. Both SDKs ship it, but the DS4 plugin was
# built without it before the merge and swapping a plugin's allocator is not a
# free change - left as it was. See MERGE-NOTES.md.
if( DAZ_SDK_VERSION STREQUAL "6" )
	set( DAZ_SDK_MEMORY_LIB "${DAZ_SDK_LIB_DIR}/${DZ_LIB_PREFIX}dzsdkmemory${DZ_LIB_SUFFIX}" )
	if( NOT EXISTS ${DAZ_SDK_MEMORY_LIB} )
		message( FATAL_ERROR "dzsdkmemory not found at ${DAZ_SDK_MEMORY_LIB} - check DAZ_SDK_DIR." )
	endif()
	add_library( dzsdkmemory SHARED IMPORTED )
	set_property( TARGET dzsdkmemory APPEND PROPERTY IMPORTED_IMPLIB ${DAZ_SDK_MEMORY_LIB} )
	set_property( TARGET dzsdkmemory APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DAZ_SDK_INCLUDE_DIR}" )
endif()

# ─── Qt ──────────────────────────────────────────────────────────────────────
#
# SDK4: Qt 4.8 lives INSIDE the SDK (include/QtCore, lib/x64/QtCore4.lib,
#       bin/x64/qmake.exe) - no separate Qt 4.8.7 install is needed.
# SDK6: the SDK ships no Qt import libraries at all. Build against an external
#       Qt 6 devkit matching the Qt bundled in Daz Studio 6 (6.10.3 for DS 6.25);
#       at runtime the plugin resolves Qt against Daz Studio's own already-loaded
#       Qt DLLs.
#
# Either way the module include directories come from the Qt targets, which is
# what lets the sources use module-less includes (<QMessageBox>) on both.

if( DAZ_SDK_VERSION STREQUAL "6" )
	set( Qt6_DIR "" CACHE PATH "Path to Qt6Config.cmake - e.g. C:/Qt/6.10.3/msvc2022_64/lib/cmake/Qt6" )
	if( NOT EXISTS ${Qt6_DIR} )
		message( FATAL_ERROR "Missing path to the Qt 6 cmake directory - set Qt6_DIR." )
	endif()

	# Only what the two plugins actually use - the Daz template's full component
	# list needs a complete Qt install (WebEngine, Qt3D, Multimedia) that a
	# plain msvc2022_64 devkit does not have.
	find_package( Qt6 REQUIRED COMPONENTS Core Core5Compat Gui Widgets Qml )

	# Qml: DzScript results are QJSValue on SDK6 (QtScript is gone).
	set( DZSDK_QT_CORE_TARGET Qt6::Core Qt6::Core5Compat )
	set( DZSDK_QT_GUI_TARGET Qt6::Gui Qt6::Widgets )
	set( DZSDK_QT_SCRIPT_TARGET Qt6::Qml )
else()
	set( QT_QMAKE_EXECUTABLE   "${DAZ_SDK_BIN_DIR}/qmake${UTIL_EXT}" )
	set( QT_BINARY_DIR         "${DAZ_SDK_BIN_DIR}" )
	set( QT_HEADERS_DIR        "${DAZ_SDK_INCLUDE_DIR}" )
	set( QT_QTCORE_INCLUDE_DIR "${DAZ_SDK_INCLUDE_DIR}/QtCore" )
	set( QT_QTCORE_LIBRARY_RELEASE "${DAZ_SDK_LIB_DIR}/QtCore4${DZ_LIB_SUFFIX}" )
	set( QT_IMPORTS_DIR        "${DAZ_SDK_LIB_DIR}" )

	# The Qt4 find module insists on an mkspecs folder; fake it.
	file( MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/DUMMY_MKSPECS/default" )
	set( QT_MKSPECS_DIR "${CMAKE_CURRENT_BINARY_DIR}/DUMMY_MKSPECS" )

	find_package( Qt4 4.8.1 REQUIRED QtCore QtGui QtScript )

	set( DZSDK_QT_CORE_TARGET Qt4::QtCore )
	set( DZSDK_QT_GUI_TARGET Qt4::QtGui )
	set( DZSDK_QT_SCRIPT_TARGET Qt4::QtScript )
endif()

# -i: the generated moc_*.cpp is #included at the bottom of its .cpp, so it must
# not re-include the header.
list( APPEND CMAKE_AUTOMOC_MOC_OPTIONS -i )

# ─── Output ──────────────────────────────────────────────────────────────────

set( DAZ_STUDIO_EXE_DIR "" CACHE PATH "Optional: Daz Studio executable directory - built plugins land in its plugins/ folder (must be writeable)." )
if( DAZ_STUDIO_EXE_DIR )
	foreach( curConfig ${CMAKE_CONFIGURATION_TYPES} )
		string( TOUPPER ${curConfig} curConfig )
		set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${curConfig} ${DAZ_STUDIO_EXE_DIR}/plugins )
		set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${curConfig} ${DAZ_STUDIO_EXE_DIR}/plugins )
	endforeach()
endif()

add_subdirectory( DazToHueExporter )
add_subdirectory( DazToHueTools )
