#pragma once


#include <QString>

#include "dzapp.h"

#include "dth/dth_settings.h"

class DthExporter : public DzBase
{
	Q_OBJECT

public:

	DthExporter();
	~DthExporter();

	Q_INVOKABLE void doExport(QString exportDirectory, QString characterName, QString referenceFrames, bool saveSettings = true);
	Q_INVOKABLE void doExportAnimation(QString exportDirectory, QString characterName, QString animationName, bool saveSettings = true);
	Q_INVOKABLE void doExportAlembicGroomPoses(QString exportDirectory, QString characterName, bool saveSettings = true);
	void saveSettings(QString exportDirectory, QString characterName);

private:

	bool readyToExport(QString exportDirectory, QString characterName, QString animationName, QString referenceFrames, bool exportingAnimationOnly = false);

	DzNode* selectedRootNode_ = nullptr;
	DthSettings dthSettings_;
	DthSettings::Settings settings_;

};