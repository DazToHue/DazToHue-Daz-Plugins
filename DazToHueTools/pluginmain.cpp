#include "dzapp.h"
#include "dzplugin.h"
#include "dth_tools_action.h"
#include "version.h"

DZ_PLUGIN_DEFINITION("DazToHue Tools");
DZ_PLUGIN_AUTHOR("MRPDEAN");
DZ_PLUGIN_VERSION(PLUGIN_MAJOR, PLUGIN_MINOR, PLUGIN_REV, PLUGIN_BUILD);
DZ_PLUGIN_DESCRIPTION(QString("<a href=\"%1/DazToHueTools/index.htm\">Documentation</a><br><br>DazToHue").arg(dzApp->getDocumentationPath()));
DZ_PLUGIN_CLASS_GUID(DazToHueToolsPane, F7597E49 - A046 - 4B0C - ADC7 - C580447F1C7E);
DZ_PLUGIN_CLASS_GUID(DazToHueToolsAction, EFFECE7E - 7562 - 43A0 - 9B33 - 3761A79E39CD);