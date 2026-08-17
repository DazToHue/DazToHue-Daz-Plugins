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

#include <map>
#include <array>
#include <memory>

#include <QtCore/QString>
#include <QLabel>

//#include "../common/common.hh"
//#include "../hair/hair.hh"
#include "geometry/geometry.h"
#include "geometry/exportable_optimized_mesh_object.h"

class DzNode;

namespace Sagan
{

	using ExportableNodes = std::set<DzNode*>;
	using AlembicMeshObjects = std::map<QString, Alembic::AbcGeom::OPolyMeshPtr>;

	class SaganExporter
	{
	public:

		SaganExporter(const Sagan::OutputTransformer* outputTransformer);

		virtual ~SaganExporter() = default;

		const OutputTransformer* getOutputTransformer() const;

		virtual const Alembic::Abc::TimeSamplingPtr getTimeSampling() const;
		virtual const Alembic::Abc::OArchive* getArchive() const;
		virtual Alembic::Abc::OArchive* getArchive();
		virtual AlembicMeshObjects& getAlembicMeshObjects();
		virtual const AlembicMeshObjects& getAlembicMeshObjects() const;
		virtual ExportableMeshObjectPtrs& getExportableMeshObjects();
		virtual const ExportableMeshObjectPtrs& getExportableMeshObjects() const;

	private:

	protected:

		std::unique_ptr<Alembic::Abc::OArchive> m_Archive;
		Alembic::Abc::TimeSamplingPtr m_TimeSampling;
		ExportableNodes exportableNodes;
		const OutputTransformer* m_outputTransformer;
		AlembicMeshObjects m_AlembicMeshObjects;
		ExportableMeshObjectPtrs m_exportableMeshObjects;

	};

}