#include "dth_tools_action.h"
#include "gui/dth_tools_gui.h"

#include <QtCore/QObject>

#include "dzapp.h"
#include "dzaction.h"
#include "dzpane.h"
#include "dzfacetmesh.h"
#include "dzobject.h"
#include "dzscene.h"
#include "dzshape.h"
#include "dzskeleton.h"
#include "dzstyle.h"
#include "dzaction.h"
#include "dzversion.h"

DazToHueToolsAction::DazToHueToolsAction() : DzPaneAction("DazToHueToolsPane")
{
	// Set the object name for the action to register it with the help and interactive lesson systems
	setObjectName(DazToHueToolsAction::metaObject()->className());

	//Setup action icon
	QImage iconImage(":/dth_images/icon");
	QPixmap basePixmap = QPixmap::fromImage(iconImage);
	QIcon icon;
	icon.addPixmap(basePixmap, QIcon::Normal, QIcon::Off);
	QAction::setIcon(icon);
}

DazToHueToolsPane::DazToHueToolsPane() : DzPane(tr("DazToHue Tools"))
{
	// Declarations
	const int margin = style()->pixelMetric(DZ_PM_GeneralMargin);
	QMargins margins(margin, margin, margin, margin);

	// Define the layout for the pane
	QVBoxLayout* mainLyt = new QVBoxLayout();
	mainLyt->setContentsMargins(margins);
	mainLyt->setSpacing(margin);

	DazToHueToolsGui* dthToolsGui = new DazToHueToolsGui(this);
	mainLyt->addWidget(dthToolsGui);

	// Set the layout for the pane
	setLayout(mainLyt);

}

/**
**/
DazToHueToolsPane::~DazToHueToolsPane()
{
}

//////////////////////////////////////////////////////////////////////////
// Qt Moc
//////////////////////////////////////////////////////////////////////////

#include "moc_dth_tools_action.cpp"