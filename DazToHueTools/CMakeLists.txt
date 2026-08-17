#**********************************************************************
#	Derived from the Daz Studio SDK CMake template.
#	Copyright (C) 2002-2026 Daz 3D, Inc. All Rights Reserved.
#
#	This file may be used only in accordance with the Daz Studio SDK
#	license provided with the Daz Studio SDK.
#**********************************************************************
#
# The DTH Tools plugin. Daz + Qt only - no third-party dependencies.

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC ON )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( DZ_PLUGIN_TGT_NAME dth_tools )
set( DZ_PLUGIN_PROJECT_NAME "DazToHue Tools" )

set( OS_SOURCES
	dth_tools.rc
	dth_tools.def
)
if( DAZ_SDK_VERSION STREQUAL "6" )
	list( APPEND OS_SOURCES "${DAZ_SDK_INCLUDE_DIR}/dzsdkmemorydefines.cpp" )
endif()

add_library( ${DZ_PLUGIN_TGT_NAME} SHARED
	dth_tools_action.cpp
	dth_tools_action.h
	gui/dth_tools_gui.cpp
	gui/dth_tools_gui.h
	gui/dth_tools_gui.ui
	resources/resources.qrc
	resources/images/action_icon.png
	pluginmain.cpp
	version.h
	${OS_SOURCES}
)

target_link_libraries( ${DZ_PLUGIN_TGT_NAME}
	PRIVATE
	dzcore
	${DZSDK_QT_CORE_TARGET}
	${DZSDK_QT_GUI_TARGET}
	${DZSDK_QT_SCRIPT_TARGET}
)

if( DAZ_SDK_VERSION STREQUAL "6" )
	target_link_libraries( ${DZ_PLUGIN_TGT_NAME} PRIVATE dzsdkmemory )
endif()

target_include_directories( ${DZ_PLUGIN_TGT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )

target_compile_definitions( ${DZ_PLUGIN_TGT_NAME} PRIVATE DAZ_SDK_MAJOR_VERSION=${DAZ_SDK_VERSION} )

target_compile_options( ${DZ_PLUGIN_TGT_NAME} PRIVATE /Zi )
target_link_options( ${DZ_PLUGIN_TGT_NAME} PRIVATE /DEBUG /OPT:REF /OPT:ICF )

set_target_properties( ${DZ_PLUGIN_TGT_NAME}
	PROPERTIES
	FOLDER "DazToHueTools"
	PROJECT_LABEL ${DZ_PLUGIN_PROJECT_NAME}
	OUTPUT_NAME "${DZ_PLUGIN_PREFIX}${DZ_PLUGIN_TGT_NAME}"
)

source_group( "Header Files\\gui" FILES gui/dth_tools_gui.h )
source_group( "Source Files\\gui" FILES gui/dth_tools_gui.cpp )
source_group( "Resources" REGULAR_EXPRESSION ".*\\.(rc)" )
source_group( "Resources\\generated\\qrc" REGULAR_EXPRESSION "/qrc_.*\\.c(xx|pp)$" )
source_group( "Resources\\qrc" REGULAR_EXPRESSION ".*\\.qrc$" )
source_group( "Resources\\images" REGULAR_EXPRESSION ".*\\.png$" )
source_group( "Source Files\\generated\\moc" REGULAR_EXPRESSION "/(moc|mocs)_.*\\.c(xx|pp)$" )
source_group( "Header Files\\generated\\ui files" REGULAR_EXPRESSION "/ui_.*\\.h$" )
source_group( "Source Files\\gui" REGULAR_EXPRESSION ".*\\.ui$" )
