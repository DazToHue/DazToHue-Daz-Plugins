#include "dth_settings.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include "dzapp.h"
#include <DzSettings.h>

#include "compat/dth_compat.h"

DthSettings::DthSettings()
{
	QDir dir(DthCompat::documentsPath());
	settingsFilePath_ = dir.filePath("DAZ 3D/DazToHue");
}

DthSettings::~DthSettings()
{
}

void DthSettings::initialiseSettings()
{
	DthSettings::Settings defaultSettings = getDefaultSettings();
	dthFile_ = std::make_unique<QFile>(settingsFilePath_ + QDir::separator() + "dth_settings.json");
	bool b_fileOpened = dthFile_->open(QIODevice::WriteOnly);
	dthWriter_ = std::make_unique<DzJsonWriter>(dthFile_.get());
	dthWriter_->startObject(true);
	dthWriter_->addMember("defaultExportPath", defaultSettings.defaultExportPath);
	dthWriter_->addMember("lastExportPath", defaultSettings.lastExportPath);
	dthWriter_->addMember("defaultCharacterName", defaultSettings.defaultCharacterName);
	dthWriter_->addMember("lastCharacterName", defaultSettings.lastCharacterName);
	dthWriter_->finishObject();
	dthFile_->close();
}

QString DthSettings::loadJsonFileIntoString(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return QString();
	}

	QTextStream in(&file);
	QString fileContent = in.readAll();
	file.close();

	return fileContent;
}

DthSettings::Settings DthSettings::getDefaultSettings()
{
	DthSettings::Settings defaultSettings;
	defaultSettings.defaultExportPath = QDir::homePath() + "/Desktop";
	defaultSettings.lastExportPath = QString("");
	defaultSettings.defaultCharacterName = QString("Character");
	defaultSettings.lastCharacterName = QString("");
	return defaultSettings;
}

DthSettings::Settings DthSettings::getSettings()
{
	DthSettings::Settings settings = getDefaultSettings();

	if (!QFile::exists(settingsFilePath_ + QDir::separator() + "dth_settings.json"))
	{
		initialiseSettings();
	}

	QString jsonString = loadJsonFileIntoString(settingsFilePath_ + QDir::separator() + "dth_settings.json");

	DzSettings* dthSettings = new DzSettings();
	if (dthSettings->fromString(jsonString))
	{
		if (dthSettings->hasKey("defaultExportPath"))
		{
			settings.defaultCharacterName = dthSettings->getStringValue("defaultExportPath");
		}

		if (dthSettings->hasKey("lastExportPath"))
		{
			settings.lastExportPath = dthSettings->getStringValue("lastExportPath");
		}

		if (dthSettings->hasKey("defaultCharacterName"))
		{
			settings.defaultCharacterName = dthSettings->getStringValue("defaultCharacterName");
		}

		if (dthSettings->hasKey("lastCharacterName"))
		{
			settings.lastCharacterName = dthSettings->getStringValue("lastCharacterName");
		}
	}

	return settings;
}

void DthSettings::saveSettings(DthSettings::Settings settings)
{

	dthFile_ = std::make_unique<QFile>(settingsFilePath_ + QDir::separator() + "dth_settings.json");
	bool b_fileOpened = dthFile_->open(QIODevice::WriteOnly);
	dthWriter_ = std::make_unique<DzJsonWriter>(dthFile_.get());
	dthWriter_->startObject(true);
	dthWriter_->addMember("defaultExportPath", settings.defaultExportPath);
	dthWriter_->addMember("lastExportPath", settings.lastExportPath);
	dthWriter_->addMember("defaultCharacterName", settings.defaultCharacterName);
	dthWriter_->addMember("lastCharacterName", settings.lastCharacterName);
	dthWriter_->finishObject();
	dthFile_->close();

}