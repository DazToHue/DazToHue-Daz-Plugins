#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include "dth_static_helpers.h"

#include <QtCore/qdir.h>

#include <dznode.h>

namespace DthStaticHelpers
{

	std::string toStdString(const QString& qstr)
	{

		if (qstr.isEmpty())
		{
			return std::string();
		}

		// Convert to UTF-8
		QByteArray utf8Data = qstr.toUtf8();

		// Check if conversion was successful
		if (utf8Data.isEmpty() && !qstr.isEmpty())
		{
			throw std::runtime_error("Failed to convert QString to UTF-8");
		}

		// Create string from the UTF-8 data
		// Note: utf8Data.constData() ensures we get a null-terminated string
		return std::string(utf8Data.constData());

	}

	QString getCleanNodeName(DzNode* node)
	{

		return node->getName().replace(" ", "_");

	}

	QString getFormattedShapeName(DzNode* node)
	{

		return node->getName() + ".Shape";

	}

	std::string getFormattedShapeNameAsString(DzNode* node)
	{

		std::string name = QVariant(node->getName() + ".Shape").toString().toUtf8().data();

		return name;

	}

	std::vector<int> parseNumberString(const std::string& input)
	{

		std::vector<int> result;
		std::stringstream ss(input);
		std::string token;

		while (ss >> token)
		{
			size_t hyphenPos = token.find('-');

			if (hyphenPos != std::string::npos)
			{
				int start = std::stoi(token.substr(0, hyphenPos));
				int end = std::stoi(token.substr(hyphenPos + 1));

				for (int i = start; i <= end; ++i)
				{
					result.push_back(i);
				}
			}
			else
			{
				result.push_back(std::stoi(token));
			}
		}

		return result;

	}

	int countNumberString(const std::string& input)
	{

		std::vector<int> result;
		std::stringstream ss(input);
		std::string token;

		while (ss >> token)
		{
			size_t hyphenPos = token.find('-');

			if (hyphenPos != std::string::npos)
			{
				int start = std::stoi(token.substr(0, hyphenPos));
				int end = std::stoi(token.substr(hyphenPos + 1));

				for (int i = start; i <= end; ++i)
				{
					result.push_back(i);
				}
			}
			else
			{
				result.push_back(std::stoi(token));
			}
		}

		return result.size();

	}

	bool createDirectory(const QString& dirPath)
	{

		QDir dir(dirPath);

		if (!dir.mkpath("."))
		{
			return false;
		}
		else
		{
			return true;
		}

	}
}