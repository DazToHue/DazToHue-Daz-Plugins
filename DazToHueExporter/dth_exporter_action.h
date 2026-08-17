#pragma once

#include "dzaction.h"

class DazToHueExporterAction : public DzAction
{
	Q_OBJECT

public:

	// Constructor
	DazToHueExporterAction();

	virtual QString getActionGroup() const { return tr("DazToHue"); }
	virtual QString	getDefaultMenuPath() const { return tr("&File"); }

	Q_INVOKABLE void doExport(QString exportDirectory, QString characterName, QString referenceFrames, bool saveSettings = true);
	Q_INVOKABLE void doExportAnimation(QString exportDirectory, QString characterName, QString animationName, bool saveSettings = true);
	Q_INVOKABLE void doExportAlembicGroomPoses(QString exportDirectory, QString characterName, bool saveSettings = true);

protected:

	virtual void executeAction();
};