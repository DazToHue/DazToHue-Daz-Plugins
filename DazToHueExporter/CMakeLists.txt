#**********************************************************************
#	Derived from the Daz Studio SDK CMake template.
#	Copyright (C) 2002-2026 Daz 3D, Inc. All Rights Reserved.
#
#	This file may be used only in accordance with the Daz Studio SDK
#	license provided with the Daz Studio SDK.
#**********************************************************************
#
# The DTH Exporter plugin. One source list for both Daz Studio generations -
# see the top-level CMakeLists.txt for the DAZ_SDK_VERSION switch.

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
# AUTOUIC replaces the checked-in *_ui.h: each Qt version's own uic generates
# the header, so the Qt6 form (QtWidgets includes, scoped enum names) cannot
# leak into the Qt4 build or vice versa.
set( CMAKE_AUTOUIC ON )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( DZ_PLUGIN_TGT_NAME dth_exporter )
set( DZ_PLUGIN_PROJECT_NAME "DazToHue Exporter" )

# ─── Third-party dependencies ────────────────────────────────────────────────
# Identical for both generations - only Daz and Qt differ by flavor.

foreach( dep RAPID_JSON_DIR IMATH_DIR ZLIB_DIR FBX_SDK_DIR OPEN_SUBDIV_SDK ALEMBIC_SDK_DIR )
	set( ${dep} "" CACHE PATH "Path to ${dep}" )
	if( NOT EXISTS "${${dep}}" )
		message( FATAL_ERROR "Missing path to ${dep}. Set -D${dep}=<path> (see README.md)." )
	endif()
endforeach()

# The FBX SDK's library folder is named after the toolset it was built with,
# and that name changes with the SDK release (2020.0 shipped vs2017, 2020.3
# ships vs2019/vs2022). Find whichever this install actually has instead of
# hardcoding one and failing on the next SDK.
set( FBX_LIB_DIR "" )
foreach( vs vs2022 vs2019 vs2017 )
	if( EXISTS "${FBX_SDK_DIR}/lib/${vs}/x64/release/libfbxsdk-md.lib" )
		set( FBX_LIB_DIR "${FBX_SDK_DIR}/lib/${vs}/x64/release" )
		break()
	endif()
endforeach()
if( NOT FBX_LIB_DIR )
	message( FATAL_ERROR "No libfbxsdk-md.lib under ${FBX_SDK_DIR}/lib/vs20xx/x64/release - check FBX_SDK_DIR." )
endif()
message( STATUS "FBX SDK libraries: ${FBX_LIB_DIR}" )

set( OS_SOURCES
	dth_exporter.rc
	dth_exporter.def
)
if( DAZ_SDK_VERSION STREQUAL "6" )
	list( APPEND OS_SOURCES "${DAZ_SDK_INCLUDE_DIR}/dzsdkmemorydefines.cpp" )
endif()

add_library( ${DZ_PLUGIN_TGT_NAME} SHARED
	dth_exporter_action.cpp
	dth_exporter_action.h
	pluginmain.cpp
	version.h
	compat/dth_compat.h
	gui/dth_exporter_gui.cpp
	gui/dth_exporter_gui.h
	gui/dth_exporter_gui.ui
	resources/resources.qrc
	resources/scripts/visibility.dsa
	resources/images/action_icon.png
	daz/daz_helpers.cpp
	daz/daz_helpers.h
	daz/daz_static_helpers.cpp
	daz/daz_static_helpers.h
	dth/dth_settings.cpp
	dth/dth_settings.h
	dth/dth_static_helpers.cpp
	dth/dth_static_helpers.h
	dth/dth_writer.cpp
	dth/dth_writer.h
	dth/dth_logger.cpp
	dth/dth_logger.h
	dth/dth_exporter.cpp
	dth/dth_exporter.h
	fbx/fbx_common.cpp
	fbx/fbx_common.h
	fbx/fbx_exporter.cpp
	fbx/fbx_exporter.h
	fbx/open_fbx_interface.cpp
	fbx/open_fbx_interface.h
	fbx/open_subdivision_interface.cpp
	fbx/open_subdivision_interface.h
	alembic/alembic_exporter.cpp
	alembic/alembic_exporter.h
	alembic/sagan/sagan_exporter.cpp
	alembic/sagan/sagan_exporter.h
	alembic/sagan/common/common.cpp
	alembic/sagan/common/common.h
	alembic/sagan/decoder/alembic_node_decoder.cpp
	alembic/sagan/decoder/alembic_node_decoder.h
	alembic/sagan/geometry/exportable_mesh_object.cpp
	alembic/sagan/geometry/exportable_mesh_object.h
	alembic/sagan/geometry/exportable_optimized_mesh_object.cpp
	alembic/sagan/geometry/exportable_optimized_mesh_object.h
	alembic/sagan/geometry/geometry.cpp
	alembic/sagan/geometry/geometry.h
	alembic/sagan/output_transformer/houdini_alembic_output_transformer.cpp
	alembic/sagan/output_transformer/houdini_alembic_output_transformer.h
	alembic/sagan/output_transformer/orientations.h
	alembic/sagan/output_transformer/output_transformer.h
	${OS_SOURCES}
)

add_library( alembic STATIC IMPORTED GLOBAL )
set_target_properties( alembic PROPERTIES IMPORTED_LOCATION ${ALEMBIC_SDK_DIR}/lib/Alembic.lib )

add_library( zlib STATIC IMPORTED GLOBAL )
set_target_properties( zlib PROPERTIES IMPORTED_LOCATION ${ZLIB_DIR}/lib/zlibstatic.lib )

add_library( imath STATIC IMPORTED GLOBAL )
set_target_properties( imath PROPERTIES IMPORTED_LOCATION ${IMATH_DIR}/lib/Imath.lib )

add_library( libfbxsdk-md STATIC IMPORTED GLOBAL )
set_target_properties( libfbxsdk-md PROPERTIES IMPORTED_LOCATION ${FBX_LIB_DIR}/libfbxsdk-md.lib )

add_library( libxml2-md STATIC IMPORTED GLOBAL )
set_target_properties( libxml2-md PROPERTIES IMPORTED_LOCATION ${FBX_LIB_DIR}/libxml2-md.lib )

add_library( osdCPU STATIC IMPORTED GLOBAL )
set_target_properties( osdCPU PROPERTIES IMPORTED_LOCATION ${OPEN_SUBDIV_SDK}/build/lib/Release/osdCPU.lib )

target_link_libraries( ${DZ_PLUGIN_TGT_NAME}
	PRIVATE
	dzcore
	${DZSDK_QT_CORE_TARGET}
	${DZSDK_QT_GUI_TARGET}
	${DZSDK_QT_SCRIPT_TARGET}
	zlib
	imath
	libfbxsdk-md
	libxml2-md
	osdCPU
	alembic
)

if( DAZ_SDK_VERSION STREQUAL "6" )
	target_link_libraries( ${DZ_PLUGIN_TGT_NAME} PRIVATE dzsdkmemory )
endif()

target_include_directories( ${DZ_PLUGIN_TGT_NAME}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${RAPID_JSON_DIR}/include
		${FBX_SDK_DIR}/include
		${OPEN_SUBDIV_SDK}
		${ALEMBIC_SDK_DIR}/include
		${IMATH_DIR}/include/Imath
)

# DAZ_SDK_MAJOR_VERSION is what compat/dth_compat.h keys off.
target_compile_definitions( ${DZ_PLUGIN_TGT_NAME} PRIVATE DAZ_SDK_MAJOR_VERSION=${DAZ_SDK_VERSION} )

# PDBs in every config so crashes inside Daz resolve to symbols.
target_compile_options( ${DZ_PLUGIN_TGT_NAME} PRIVATE /Zi )
target_link_options( ${DZ_PLUGIN_TGT_NAME} PRIVATE /DEBUG /OPT:REF /OPT:ICF )

set_target_properties( ${DZ_PLUGIN_TGT_NAME}
	PROPERTIES
	FOLDER "DazToHueExporter"
	PROJECT_LABEL ${DZ_PLUGIN_PROJECT_NAME}
	OUTPUT_NAME "${DZ_PLUGIN_PREFIX}${DZ_PLUGIN_TGT_NAME}"
)

source_group( "Header Files\\compat" FILES compat/dth_compat.h )
source_group( "Header Files\\gui" FILES gui/dth_exporter_gui.h )
source_group( "Source Files\\gui" FILES gui/dth_exporter_gui.cpp )
source_group( "Header Files\\daz" FILES daz/daz_helpers.h daz/daz_static_helpers.h )
source_group( "Source Files\\daz" FILES daz/daz_helpers.cpp daz/daz_static_helpers.cpp )
source_group( "Header Files\\dth" FILES dth/dth_settings.h dth/dth_static_helpers.h dth/dth_writer.h dth/dth_exporter.h )
source_group( "Source Files\\dth" FILES dth/dth_settings.cpp dth/dth_static_helpers.cpp dth/dth_writer.cpp dth/dth_exporter.cpp )
source_group( "Header Files\\fbx" FILES fbx/fbx_common.h fbx/fbx_exporter.h fbx/open_fbx_interface.h fbx/open_subdivision_interface.h )
source_group( "Source Files\\fbx" FILES fbx/fbx_common.cpp fbx/fbx_exporter.cpp fbx/open_fbx_interface.cpp fbx/open_subdivision_interface.cpp )
source_group( "Header Files\\alembic" FILES alembic/alembic_exporter.h )
source_group( "Source Files\\alembic" FILES alembic/alembic_exporter.cpp )
source_group( "Header Files\\alembic\\sagan" FILES alembic/sagan/sagan_exporter.h )
source_group( "Source Files\\alembic\\sagan" FILES alembic/sagan/sagan_exporter.cpp )
source_group( "Header Files\\alembic\\sagan\\common" FILES alembic/sagan/common/common.h )
source_group( "Source Files\\alembic\\sagan\\common" FILES alembic/sagan/common/common.cpp )
source_group( "Header Files\\alembic\\sagan\\decoder" FILES alembic/sagan/decoder/alembic_node_decoder.h )
source_group( "Source Files\\alembic\\sagan\\decoder" FILES alembic/sagan/decoder/alembic_node_decoder.cpp )
source_group( "Header Files\\alembic\\sagan\\geometry" FILES alembic/sagan/geometry/exportable_mesh_object.h alembic/sagan/geometry/exportable_optimized_mesh_object.h alembic/sagan/geometry/geometry.h )
source_group( "Source Files\\alembic\\sagan\\geometry" FILES alembic/sagan/geometry/exportable_mesh_object.cpp alembic/sagan/geometry/exportable_optimized_mesh_object.cpp alembic/sagan/geometry/geometry.cpp )
source_group( "Header Files\\alembic\\sagan\\output_transformer" FILES alembic/sagan/output_transformer/houdini_alembic_output_transformer.h alembic/sagan/output_transformer/orientations.h alembic/sagan/output_transformer/output_transformer.h )
source_group( "Source Files\\alembic\\sagan\\output_transformer" FILES alembic/sagan/output_transformer/houdini_alembic_output_transformer.cpp )
source_group( "Resources" REGULAR_EXPRESSION ".*\\.(rc)" )
source_group( "Resources\\generated\\qrc" REGULAR_EXPRESSION "/qrc_.*\\.c(xx|pp)$" )
source_group( "Resources\\qrc" REGULAR_EXPRESSION ".*\\.qrc$" )
source_group( "Resources\\scripts" REGULAR_EXPRESSION ".*\\.dsa$" )
source_group( "Resources\\images" REGULAR_EXPRESSION ".*\\.png$" )
source_group( "Source Files\\generated\\moc" REGULAR_EXPRESSION "/(moc|mocs)_.*\\.c(xx|pp)$" )
source_group( "Header Files\\generated\\ui files" REGULAR_EXPRESSION "/ui_.*\\.h$" )
source_group( "Source Files\\gui" REGULAR_EXPRESSION ".*\\.ui$" )
