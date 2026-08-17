#pragma once

#include "dzaction.h"
#include "dzpane.h"

class DazToHueToolsAction : public DzPaneAction
{
	Q_OBJECT
public:

	// Constructor
	//DazToHueToolsAction() : DzPaneAction("DazToHueToolsPane")
	//{

	//}

	DazToHueToolsAction();
};

class DazToHueToolsPane : public DzPane
{
	Q_OBJECT

public:

	// Constructor
	DazToHueToolsPane();

	// Destructor
	~DazToHueToolsPane();

};