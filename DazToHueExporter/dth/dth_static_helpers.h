#pragma once

#include <filesystem>

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

}