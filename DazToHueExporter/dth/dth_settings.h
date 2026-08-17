#pragma once

#include <memory>

#include <QtCore/QString>

#include <QFileDialog>

#include <dzjsonwriter.h>
#include <dzjsonreader.h>
#include <IDzJsonIO.h>

class DthSettings
{
public:

	struct Settings
	{
		QString defaultExportPath;
		QString lastExportPath;
		QString defaultCharacterName;
		QString lastCharacterName;
	};

	DthSettings();
	~DthSettings();

	Settings getSettings();
	void saveSettings(Settings settings);

private:

	void initialiseSettings();
	QString loadJsonFileIntoString(const QString& filePath);
	Settings getDefaultSettings();

	QString settingsFilePath_;
	std::unique_ptr<QFile> dthFile_;
	std::unique_ptr<DzJsonReader> dthReader_;
	std::unique_ptr<DzJsonWriter> dthWriter_;

};