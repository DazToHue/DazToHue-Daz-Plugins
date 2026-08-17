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

#pragma once

#include <set>
#include <map>
#include <vector>
#include <tuple>
#include <optional>

#include <QtCore/QString>
#include <QtCore/QSet>

#include <rapidjson/document.h>

#include "dznode.h"
#include "dzobject.h"
#include "dzfacetmesh.h"
#include "dzface.h"
#include "dzgeneraldefs.h"
#include "dzintproperty.h"

namespace Sagan
{

	constexpr const int RecordDelimiter = 0x1e;
	constexpr const int FieldDelimiter = 0x1f;

	const static QString minimumUText = "MinimumU";
	const static QString maximumUText = "MaximumU";
	const static QString minimumVText = "MinimumV";
	const static QString maximumVText = "MaximumV";

	enum RootFindingStrategy : int
	{
		MaximumU,
		MinimumU,
		MaximumV,
		MinimumV
	};

	const extern std::map< RootFindingStrategy, QString > rootFindingStrategy2StringMap;
	const extern std::map< QString, RootFindingStrategy > string2RootFindingStrategyMap;

	// Presentation conversions
	QString rootFindingStrategy2String(const RootFindingStrategy& rfs);
	RootFindingStrategy string2RootFindingStrategy(const QString& s);

	// conversions for entire list of strategies
	// TODO(mrpdean): DIVERGENCE - live in the DS4 tree, commented out in the
	// DS6 tree. Nothing in either tree calls them, so the DS6 state was kept.
	// Delete outright, or restore them if something outside these plugins does.
	//std::map< QString, RootFindingStrategy > makeRootFindingStrategies(const QString& s);
	//QString makeRootFindingStrategyRecords(const std::map< QString, RootFindingStrategy >& rootFindingStrategies);


	bool removeDirectoryContents(const QString& dirPath);

	QString sanitize(const QString& s);
	std::string toStdString(const QString& qstr);

	template< typename T>
	inline QString list1D(const T& items)
	{

		QString s;
		QTextStream ss(&s);

		auto first = true;

		for (const auto& item : items)
		{

			if (!first) ss << ",";

			ss << item;

			first = false;

		}

		return *ss.string();

	}
	template< typename T >
	QString list2D(const std::vector< T >& items)
	{

		QString s;
		QTextStream ss(&s);

		auto first = true;

		for (const auto& item : items)
		{
			if (!first) ss << ",";

			ss << '(' << item[0] << ',' << item[1] << ')';

			first = false;

		}

		return *ss.string();

	}

	template< typename T >
	QString list3D(const std::vector< T >& items)
	{

		QString s;
		QTextStream ss(&s);

		auto first = true;

		for (const auto& item : items)
		{

			if (!first) ss << ",";

			ss << '(' << item[0] << ',' << item[1] << ',' << item[2] << ')';

			first = false;

		}

		return *ss.string();

	}
}

