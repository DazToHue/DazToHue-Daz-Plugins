#pragma once

#include <functional>

#include "dzaction.h"

#include "compat/dth_compat.h"

// QScriptable is what lets the Q_INVOKABLE entry points below hand a failure
// back to the calling script as a catchable error, via
// DthCompat::raiseScriptError(). Qt's own class on SDK4, a stand-in on SDK6 -
// dth_compat.h explains why the name has to be spelled exactly this way.
class DazToHueExporterAction : public DzAction, public QScriptable
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

private:

	void runGuardedExport(const QString& entryPointName, const std::function<void()>& exportBody);
};
