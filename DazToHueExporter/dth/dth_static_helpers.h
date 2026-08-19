#pragma once

#include <filesystem>

#include <QtCore/QStringList>

#include "dzapp.h"

namespace DthStaticHelpers
{

	std::string toStdString(const QString& qstr);
	QString getCleanNodeName(DzNode* node);
	QString getFormattedShapeName(DzNode* node);
	std::string getFormattedShapeNameAsString(DzNode* node);
	std::vector<int> parseNumberString(const std::string& input);
	int countNumberString(const std::string& input);
	bool createDirectory(const QString& filePath);
	QString relocateTempTexture(const QString& texturePath, const QString& exportDirectory);
	int removeStaleRomExportSet(const QString& exportDirectory, const QString& characterName, QStringList* failedRemovals = nullptr);
	int removeStaleAnimationExport(const QString& exportDirectory, const QString& characterName, const QString& animationName);
	int removeStaleGroomExport(const QString& exportDirectory, const QString& characterName);

}