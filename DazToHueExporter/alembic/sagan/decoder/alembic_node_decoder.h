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
#include <map>
#include <array>
#include <cstdint>

#include "../../../daz/daz_static_helpers.h"
#include "../../../dth/dth_static_helpers.h"
#include "../../../dth/dth_writer.h"

class DzNode;
class DthWriter;

namespace Sagan
{

	using AlembicObjectPtr = std::shared_ptr <Alembic::AbcGeom::OObject>;
	using ExportableNodes = std::set<DzNode*>;
	using NodeNameFormatterCallbackType = std::function<std::string(DzNode* node)>;

	// How many frames a single mesh's geometry actually CHANGED on, recorded
	// as the bake writes it. A mesh at 0 here is a statue: the walk advanced
	// the frame and the scene never re-evaluated.
	//
	// The measure is a hash of the vertex data actually written, NOT a
	// bounding box. A box answers "did the extents change", which is a proxy
	// that disagrees with the geometry: measured 2026-08-25, three runs
	// reported the figure moving on 131 of 484 frames while writing a
	// full-size 1.1 GB archive - and Ogawa deduplicates identical samples, so
	// a genuinely frozen bake collapses to 23.7 MB (which the 0-of-484 run
	// did). Full size means the data was changing while the box was not. A
	// hash cannot disagree with the file that way, and it is what any gate
	// built on this must be able to trust.
	struct MeshMotion
	{
		std::uint64_t lastHash = 0;
		bool haveLastHash = false;
		int framesWritten = 0;
		int framesMoved = 0;
	};

	class AlembicNodeDecoder
	{

	public:
		AlembicNodeDecoder(SaganExporter* saganExporter, DazHelpers& dazHelpers, DthWriter* dthWriter);
		~AlembicNodeDecoder();

		void decodeSelected(DzNode* selectedRootNode);
		/**
			Write one sample per exported mesh; returns how many meshes'
			geometry differed from their previous sample. 0 on a multi-frame
			bake means the frame was written from a scene that did not
			re-evaluate.
		*/
		int writeObjects(bool firstFrame) const;

		void setShapeNameFormatter(NodeNameFormatterCallbackType nodeNameFormatter);
		std::string getFormattedShapeNameAsString(DzNode* node);
		ExportableNodes getExportableNodes() const;

		/** One "<mesh>: moved on N of M frames" line per exported mesh. */
		QStringList getMotionSummary() const;

		/**
			Labels of meshes whose geometry never changed across a multi-frame
			bake - statues. Empty on any healthy export of an animated range.
		*/
		QStringList getFrozenMeshes() const;

		/** How many meshes the bake wrote at all. */
		int getWrittenMeshCount() const;

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
		bool writeObject(const DzNode* node, bool firstFrame) const;
		std::shared_ptr<Alembic::AbcGeom::OObject> getTopLevelObjectPointer();

		ExportableNodes m_exportableNodes;

		// NOTE: m_exportableNodes is a std::set<DzNode*>, so it orders by
		// POINTER VALUE - heap layout, stable within a session, different
		// between runs. writeObjects() iterates it, which is why two runs of
		// one build produce Alembics with identical geometry at different byte
		// offsets.
		mutable std::map<QString, MeshMotion> m_motionByLabel;
	};

}
