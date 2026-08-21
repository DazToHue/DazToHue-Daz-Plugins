#pragma once

#include "dzversion.h"

#define PLUGIN_MAJOR	2
#define PLUGIN_MINOR	1
#define PLUGIN_REV		4
#define PLUGIN_BUILD	0

// Helper macros to force expansion of the numbers before converting to text
#define DZ_STRINGIFY_HELPER(x) #x
#define DZ_STRINGIFY(x) DZ_STRINGIFY_HELPER(x)

#define PLUGIN_VERSION_STRING DZ_STRINGIFY(PLUGIN_MAJOR) "." DZ_STRINGIFY(PLUGIN_MINOR) "." DZ_STRINGIFY(PLUGIN_REV)

#define PLUGIN_VERSION	DZ_MAKE_VERSION( PLUGIN_MAJOR, PLUGIN_MINOR, PLUGIN_REV, PLUGIN_BUILD )
