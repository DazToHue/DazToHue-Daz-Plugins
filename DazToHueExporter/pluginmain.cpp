#include "dzapp.h"
#include "dzplugin.h"
#include "dth_exporter_action.h"
#include "version.h"

DZ_PLUGIN_DEFINITION("DazToHue Exporter");
DZ_PLUGIN_AUTHOR("MRPDEAN");
DZ_PLUGIN_VERSION(PLUGIN_MAJOR, PLUGIN_MINOR, PLUGIN_REV, PLUGIN_BUILD);
DZ_PLUGIN_DESCRIPTION(QString("<a href=\"%1/DazToHueExporter/index.htm\">Documentation</a><br><br>DazToHue").arg(dzApp->getDocumentationPath()));
DZ_PLUGIN_CLASS_GUID(DazToHueExporterAction, 2E7F823B - 033F - 4CC4 - 8595 - 618A53298799);
