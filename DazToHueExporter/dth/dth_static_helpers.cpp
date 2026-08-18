#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include "dth_static_helpers.h"

#include <QtCore/qdir.h>
#include <QtCore/QFile>

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

	namespace
	{
		// Removes the file when present. Counts a successful removal in
		// removed; a file that exists but cannot be removed (locked by another
		// application) lands in failedRemovals when the caller asked for them.
		void removeStaleFile(const QString& filePath, int& removed, QStringList* failedRemovals)
		{
			if (!QFile::exists(filePath)) return;

			if (QFile::remove(filePath))
			{
				removed++;
			}
			else if (failedRemovals != nullptr)
			{
				failedRemovals->append(filePath);
			}
		}
	}

	// Pre-existing output files must have ZERO influence on an export:
	// dth-character-studio measured exporter 2.0.2 (DS 4.24, 2026-08-11)
	// rewriting an existing set as a static ROM, and works around it by
	// deleting the set before every doExport. Deleting our own set up front
	// makes that guarantee the exporter's, so every writer below starts from
	// an empty target. Only the set's own names are touched - anything else
	// in the folder belongs to the user.
	int removeStaleRomExportSet(const QString& exportDirectory, const QString& characterName, QStringList* failedRemovals)
	{
		int removed = 0;

		const QString ownFiles[] = {
			characterName + ".dth",
			characterName + ".abc",
			characterName + ".fbx",
			characterName + "_base.fbx",
			characterName + "_experimental_rom.fbx",
		};

		for (const QString& fileName : ownFiles)
		{
			removeStaleFile(exportDirectory + "/" + fileName, removed, failedRemovals);
		}

		QDir referenceSkeletonDir(exportDirectory + "/Reference Skeletons");
		if (referenceSkeletonDir.exists())
		{
			const QStringList entries = referenceSkeletonDir.entryList(QDir::Files);
			for (const QString& entry : entries)
			{
				if (!entry.startsWith(characterName + "_frame_")) continue;

				removeStaleFile(referenceSkeletonDir.absoluteFilePath(entry), removed, failedRemovals);
			}
		}

		return removed;
	}

	int removeStaleAnimationExport(const QString& exportDirectory, const QString& characterName, const QString& animationName)
	{
		int removed = 0;
		removeStaleFile(exportDirectory + "/" + characterName + "_" + animationName + "_animation.fbx", removed, nullptr);
		return removed;
	}

	int removeStaleGroomExport(const QString& exportDirectory, const QString& characterName)
	{
		int removed = 0;
		removeStaleFile(exportDirectory + "/" + characterName + "_grooms.abc", removed, nullptr);
		return removed;
	}
}