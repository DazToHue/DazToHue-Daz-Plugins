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

#include "../sagan_exporter.h"
#include "../geometry/geometry.h"
#include "../common/common.h"

#include <variant>
#include <functional>
#include <vector>
#include <map>
#include <array>

#include <QtCore/QStringList>

#include "../../../daz/daz_static_helpers.h"
#include "../../../dth/dth_static_helpers.h"
#include "../../../dth/dth_writer.h"

class DzNode;
class DthWriter;

namespace Sagan
{

	using AlembicObjectPtr = std::shared_ptr <Alembic::AbcGeom::OObject>;
	using ExportableNodes = std::set<DzNode*>;

	// Decode order - parents before their children. Distinct from
	// ExportableNodes on purpose: see m_exportableNodesInDecodeOrder.
	using ExportableNodeSequence = std::vector<DzNode*>;

	// How many sampled frames a single mesh's bounds actually changed on.
	// Written during the bake so a ROM that silently froze its clothing is
	// visible in the export log instead of only under a Houdini probe.
	struct MeshMotion
	{
		std::array<double, 6> lastBounds{};
		bool haveLastBounds = false;
		int framesWritten = 0;
		int framesMoved = 0;
	};
	using NodeNameFormatterCallbackType = std::function<std::string(DzNode* node)>;

	class AlembicNodeDecoder
	{

	public:
		AlembicNodeDecoder(SaganExporter* saganExporter, DazHelpers& dazHelpers, DthWriter* dthWriter);
		~AlembicNodeDecoder();

		void decodeSelected(DzNode* selectedRootNode);
		void updateGeometryCaches() const;
		void writeObjects(bool firstFrame) const;
		void setShapeNameFormatter(NodeNameFormatterCallbackType nodeNameFormatter);
		std::string getFormattedShapeNameAsString(DzNode* node);
		ExportableNodes getExportableNodes() const;

		/** One "<mesh>: moved on N of M frames" line per exported mesh. */
		QStringList getMotionSummary() const;

	private:
		NodeNameFormatterCallbackType nodeNameFormatter_;
		DazHelpers& r_dazHelpers;
		DthWriter* r_dthWriter = nullptr;

	protected:

		SaganExporter* saganExporter;
		void decodeNode(DzNode* node, const AlembicObjectPtr& parent = nullptr);
		void decodeFigureNode(DzNode* node, const AlembicObjectPtr& parent = nullptr);
		void decodeGeometryShellNode(DzNode* node, const AlembicObjectPtr& parent = nullptr);
		void decodeObjectNode(DzNode* node, const AlembicObjectPtr& parent = nullptr);
		void decodeChildNodes(DzNode* node, const AlembicObjectPtr& parent);
		void initNormalObject(DzNode* node, const AlembicObjectPtr& parent);
		void initGeometryShellObjectNode(DzNode* node, const AlembicObjectPtr& parent = nullptr);
		void initObject(DzNode* node, const AlembicObjectPtr& parent, const Sagan::FaceVertexCounts& faceVertexCounts, const FaceVertexIndices& faceVertexIndices, const FacetsByMaterialIndex& facetsByMaterialIndex, const MaterialGroupNames& materialGroupNames, const UVArray& uvArray, const Visible2OriginalVertexIndices& visible2OriginalVertexIndices);
		void writeObject(const DzNode* node, bool firstFrame) const;
		std::shared_ptr<Alembic::AbcGeom::OObject> getTopLevelObjectPointer();

		ExportableNodes m_exportableNodes;

		// The SAME nodes, in the order they were decoded: a parent is always
		// ahead of its children. updateGeometryCaches() walks THIS, never
		// m_exportableNodes - a std::set<DzNode*> orders by pointer value, so
		// iterating it forces mesh evaluation in heap-address order. See
		// updateGeometryCaches() for what that cost us.
		ExportableNodeSequence m_exportableNodesInDecodeOrder;

		mutable std::map<QString, MeshMotion> m_motionByLabel;
	};

}
