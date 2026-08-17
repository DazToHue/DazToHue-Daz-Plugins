/*
COPYRIGHT (C) 2020-2025, I WAS SERIOUS PRODUCTIONS
ALL RIGHTS RESERVED.

REDISTRIBUTION AND USE IN SOURCE AND BINARY FORMS, WITH OR WITHOUT
MODIFICATION, ARE PERMITTED PROVIDED THAT THE FOLLOWING CONDITIONS ARE MET:
1. REDISTRIBUTIONS OF SOURCE CODE MUST RETAIN THE ABOVE COPYRIGHT
   NOTICE, THIS LIST OF CONDITIONS AND THE FOLLOWING DISCLAIMER.
2. REDISTRIBUTIONS IN BINARY FORM MUST REPRODUCE THE ABOVE COPYRIGHT
   NOTICE, THIS LIST OF CONDITIONS AND THE FOLLOWING DISCLAIMER IN THE
   DOCUMENTATION AND/OR OTHER MATERIALS PROVIDED WITH THE DISTRIBUTION.
3. ALL ADVERTISING MATERIALS MENTIONING FEATURES OR USE OF THIS SOFTWARE
   MUST DISPLAY THE FOLLOWING ACKNOWLEDGEMENT:
   THIS PRODUCT INCLUDES SOFTWARE DEVELOPED BY I WAS SERIOUS PRODUCTIONS.
4. NEITHER THE NAME OF I WAS SERIOUS PRODUCTIONS NOR THE
   NAMES OF ITS CONTRIBUTORS MAY BE USED TO ENDORSE OR PROMOTE PRODUCTS
   DERIVED FROM THIS SOFTWARE WITHOUT SPECIFIC PRIOR WRITTEN PERMISSION.

THIS SOFTWARE IS PROVIDED BY I WAS SERIOUS PRODUCTIONS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL I WAS SERIOUS PRODUCTIONS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "common.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>

#include <stdexcept>

#include "dzscene.h"
#include "dzfacegroup.h"

namespace
{

	QString replace(const QString& s, const QString& notAllowed, const QChar replaceWith)
	{

		auto result = s;

		for (auto& c : result)
		{

			if (notAllowed.contains(c))
				c = ' ';
		}

		return result.toLower().simplified().replace(" ", replaceWith);

	}

}

const std::map< Sagan::RootFindingStrategy, QString > Sagan::rootFindingStrategy2StringMap = {

	{ Sagan::MaximumU, Sagan::maximumUText },
	{ Sagan::MaximumV, Sagan::maximumVText },
	{ Sagan::MinimumU, Sagan::minimumUText },
	{ Sagan::MinimumV, Sagan::minimumVText }

};

const std::map< QString, Sagan::RootFindingStrategy > Sagan::string2RootFindingStrategyMap = {

	{ Sagan::maximumUText, Sagan::MaximumU },
	{ Sagan::maximumVText, Sagan::MaximumV },
	{ Sagan::minimumUText, Sagan::MinimumU },
	{ Sagan::minimumVText, Sagan::MinimumV }

};

QString Sagan::rootFindingStrategy2String(const RootFindingStrategy& rfs)
{

	const auto it = rootFindingStrategy2StringMap.find(rfs);
	return (it != rootFindingStrategy2StringMap.end()) ? it->second : minimumVText;

}

Sagan::RootFindingStrategy Sagan::string2RootFindingStrategy(const QString& s)
{

	const auto it = string2RootFindingStrategyMap.find(s);
	return (it != string2RootFindingStrategyMap.end()) ? it->second : Sagan::RootFindingStrategy::MinimumV;

}

//std::map< QString, Sagan::RootFindingStrategy > Sagan::makeRootFindingStrategies(const QString& s)
//{
//
//	std::map< QString, Sagan::RootFindingStrategy > rootFindingStrategies;
//
//	const auto records = s.split(QChar(RecordDelimiter));
//
//	for (const auto& record : records)
//	{
//
//		const auto fields = record.split(FieldDelimiter);
//
//		if (fields.size() != 2) continue;
//
//		const auto label = fields[0];
//		const auto rootFindingStrategyText = fields[1];
//
//		const auto rootFindingStrategy = string2RootFindingStrategy(rootFindingStrategyText);
//		rootFindingStrategies[label] = rootFindingStrategy;
//
//	}
//
//	return rootFindingStrategies;
//
//}
//
//QString Sagan::makeRootFindingStrategyRecords(const std::map< QString, RootFindingStrategy >& rootFindingStrategies)
//{
//
//	QString s;
//
//	for (const auto& [label, rootFindingStrategy] : rootFindingStrategies)
//	{
//
//		QString rfs = rootFindingStrategy2String(rootFindingStrategy);
//		const auto record = label + QChar(FieldDelimiter) + rfs;
//		s.append(record).append(RecordDelimiter);
//
//	}
//
//	return s;
//
//}

QString Sagan::sanitize(const QString& s)
{

	return replace(s, "~`!@#$%^&*()_+-=|\\}]{[\"':;?/>.<,", '_');

}

std::string Sagan::toStdString(const QString& qstr)
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
		//Logger::getInstance().log(FILEANDLINE) << "Failed to convert QString to UTF-8";
		throw std::runtime_error("Failed to convert QString to UTF-8");
	}

	// Create string from the UTF-8 data
	// Note: utf8Data.constData() ensures we get a null-terminated string
	return std::string(utf8Data.constData());
}

bool Sagan::removeDirectoryContents(const QString& dirPath)
{

	QDir dir(dirPath);

	// List all files and directories, but ignore "." and ".."
	QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

	foreach(const QFileInfo & entry, entries)
	{
		if (entry.isDir())
		{
			// Recursively remove contents of the subdirectory
			if (!removeDirectoryContents(entry.absoluteFilePath()))
			{
				return false; // Propagate failure
			}

			// Remove the now empty subdirectory
			if (!dir.rmdir(entry.absoluteFilePath()))
			{
				return false;
			}

		}
		else
		{
			// Remove the file
			if (!QFile::remove(entry.absoluteFilePath()))
			{
				return false;
			}
		}
	}

	return true; // Success
}
