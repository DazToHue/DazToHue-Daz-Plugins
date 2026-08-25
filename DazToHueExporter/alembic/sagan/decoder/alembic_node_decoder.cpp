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

#include "alembic_node_decoder.h"
#include "../sagan_exporter.h"

#include "../../../daz/daz_static_helpers.h"
#include "../../../dth/dth_static_helpers.h"
#include "../../../dth/dth_writer.h"

#include "dzscene.h"
#include "dzfigure.h"

#include "../../../version.h"
#include "../../../dth/dth_fault_probe.h"

Sagan::AlembicNodeDecoder::AlembicNodeDecoder(SaganExporter* saganExporter, DazHelpers& dazHelpers, DthWriter* dthWriter) : saganExporter(saganExporter), r_dazHelpers(dazHelpers), r_dthWriter(dthWriter)
{
}

Sagan::AlembicNodeDecoder::~AlembicNodeDecoder()
{
}

void Sagan::AlembicNodeDecoder::decodeSelected(DzNode* node)
{
	if (node->isVisible())
	{
		decodeNode(node);
	}
}

void Sagan::AlembicNodeDecoder::decodeNode(DzNode* node, const AlembicObjectPtr& parent)
{
	if (!node->isVisible())
	{
		return;
	}

	const auto nodeLabel = node->getLabel();
	const auto className = node->className();
	const auto nodeName = node->getName();

	if (className == "DzFigure")
	{
		if (DazStaticHelpers::isStrandBasedHair(node))
		{
			const auto theFigure = dynamic_cast<const DzFigure*>(node);

			if (theFigure)
			{
				decodeFigureNode(node, parent);
			}
			else
			{
				decodeObjectNode(node, parent);
			}
		}
		else
		{
			decodeFigureNode(node, parent);
		}
	}
	else if (className == "DzLegacyFigure")
	{
		decodeFigureNode(node, parent);
	}
	else if (className == "DzGeometryShellNode")
	{
		decodeGeometryShellNode(node, parent);
	}
	else if (className == "DzNode")
	{
		decodeObjectNode(node, parent);
	}
	else
	{
		decodeChildNodes(node, parent);
	}

	if (DazStaticHelpers::isGeoshell(node))
	{
		if (r_dthWriter != nullptr) r_dthWriter->addAlembicGeoshell(DthStaticHelpers::getFormattedShapeName(node));
	}

	// We could probably just not export the duplicate geograft geo here
	// but I wonder if there is any potential use for them down the track
	if (DazStaticHelpers::isGeograft(node))
	{
		if (r_dthWriter != nullptr) r_dthWriter->addAlembicGeograft(DthStaticHelpers::getFormattedShapeName(node));
	}
}

void Sagan::AlembicNodeDecoder::decodeFigureNode(DzNode* node, const AlembicObjectPtr& parent)
{
	decodeObjectNode(node, parent);
}

void Sagan::AlembicNodeDecoder::decodeObjectNode(DzNode* node, const AlembicObjectPtr& parent)
{
	if (DazStaticHelpers::isRootNode(node))
	{
		const auto topObjPtr = getTopLevelObjectPointer();
		initNormalObject(node, topObjPtr);
	}
	else
	{
		initNormalObject(node, parent);
	}
}

void Sagan::AlembicNodeDecoder::decodeGeometryShellNode(DzNode* node, const AlembicObjectPtr& parent)
{
	if (DazStaticHelpers::isRootNode(node))
	{
		const auto topObjPtr = getTopLevelObjectPointer();
		initGeometryShellObjectNode(node, topObjPtr);
	}
	else
	{
		initGeometryShellObjectNode(node, parent);
	}
}

void Sagan::AlembicNodeDecoder::initGeometryShellObjectNode(DzNode* node, const AlembicObjectPtr& parent)
{
	const auto hiddenFaces = r_dazHelpers.getHiddenFaces(node);
	const auto [uvArray, materialGroupNames, facetsByMaterialIndex, faceVertexCounts, faceVertexIndices, visible2OriginalVertexIndices] = Sagan::getImmutables(node, Sagan::WindingOrder::Alembic, hiddenFaces);
	initObject(node, parent, faceVertexCounts, faceVertexIndices, facetsByMaterialIndex, materialGroupNames, uvArray, visible2OriginalVertexIndices);
}

void Sagan::AlembicNodeDecoder::decodeChildNodes(DzNode* node, const AlembicObjectPtr& parent)
{
	DzNodeListIterator it = node->nodeChildrenIterator();

	while (it.hasNext())
	{
		DzNode* childNode = it.next();
		decodeNode(childNode, parent);
	}
}

void Sagan::AlembicNodeDecoder::initNormalObject(DzNode* node, const AlembicObjectPtr& parent)
{
	const auto hiddenFaces = r_dazHelpers.getHiddenFaces(node);
	const auto [uvArray, materialGroupNames, facetsByMaterialIndex, faceVertexCounts, faceVertexIndices, visible2OriginalVertexIndices] = Sagan::getImmutables(node, Sagan::WindingOrder::Alembic, hiddenFaces);
	initObject(node, parent, faceVertexCounts, faceVertexIndices, facetsByMaterialIndex, materialGroupNames, uvArray, visible2OriginalVertexIndices);
}

void Sagan::AlembicNodeDecoder::initObject(DzNode* node, const AlembicObjectPtr& parent, const Sagan::FaceVertexCounts& faceVertexCounts, const FaceVertexIndices& faceVertexIndices, const FacetsByMaterialIndex& facetsByMaterialIndex, const MaterialGroupNames& materialGroupNames, const UVArray& uvArray, const Visible2OriginalVertexIndices& visible2OriginalVertexIndices)
{
	//DazStaticHelpers::enableInteractiveUpdates(node);

	const auto label = node->getLabel();
	const auto stdName = getFormattedShapeNameAsString(node);

	// Reserve the alembic mesh
	auto container = std::make_shared< Alembic::AbcGeom::OXform>(*parent, stdName, saganExporter->getTimeSampling());
	auto meshObj = std::make_shared< Alembic::AbcGeom::OPolyMesh>(*container, stdName, saganExporter->getTimeSampling());
	saganExporter->getAlembicMeshObjects()[label] = meshObj;

	// Convert the UVs
	AlembicUVSet uvs;
	for (const auto& uvElement : uvArray)
	{
		AlembicUV uv;
		uv[0] = uvElement[0];
		uv[1] = uvElement[1];
		uvs.push_back(uv);
	}

	const auto exportableOptimizedMeshObjectPtr = std::make_shared<ExportableOptimizedMeshObject>(node, faceVertexCounts, faceVertexIndices, facetsByMaterialIndex, materialGroupNames, uvs, visible2OriginalVertexIndices);

	saganExporter->getExportableMeshObjects()[label] = exportableOptimizedMeshObjectPtr;

	auto& meshSchema = meshObj->getSchema();

	// Encode the facesets
	for (const auto& [materialIndex, facets] : facetsByMaterialIndex)
	{
		auto qMaterialSlotName = materialGroupNames[materialIndex];
		const auto materialSlotName = Sagan::toStdString(qMaterialSlotName);
		meshSchema.createFaceSet(materialSlotName);
	}

	// Set the UV name
	const auto uvIdentifier = Sagan::toStdString(label);
	meshSchema.setUVSourceName(uvIdentifier);

	// Add a detail attribute in Houdini to tell that this Alembic was exported from this plugin
	Alembic::Abc::OCompoundProperty arbGeomParams = meshSchema.getArbGeomParams();
	Alembic::Abc::OStringProperty sourceProp(arbGeomParams, "dth_exporter");
	sourceProp.set(PLUGIN_VERSION_STRING);

	Alembic::Abc::OStringProperty oldSourceProp(arbGeomParams, "source");
	oldSourceProp.set("dth");

	// Add the node to the list of rom exportable nodes
	m_exportableNodes.insert(node);

	// Decode child nodes
	decodeChildNodes(node, parent);
}

QStringList Sagan::AlembicNodeDecoder::getFrozenMeshes() const
{
	QStringList frozen;

	for (const auto& [label, motion] : m_motionByLabel)
	{
		// framesWritten > 1: a single-frame bake cannot "move" and is not a
		// statue. framesMoved == 0 across a multi-frame walk means every
		// sample after the first was byte-identical to its predecessor.
		if (motion.framesWritten > 1 && motion.framesMoved == 0) frozen.append(label);
	}

	return frozen;
}

int Sagan::AlembicNodeDecoder::getWrittenMeshCount() const
{
	return static_cast<int>(m_motionByLabel.size());
}

QStringList Sagan::AlembicNodeDecoder::getMotionSummary() const
{
	QStringList lines;

	for (const auto& [label, motion] : m_motionByLabel)
	{
		lines.append(QString("%1: moved on %2 of %3 frames").arg(label).arg(motion.framesMoved).arg(motion.framesWritten));
	}

	return lines;
}

int Sagan::AlembicNodeDecoder::writeObjects(bool firstFrame) const
{
	// The breadcrumb per node, not just per frame: these are DzNode pointers
	// decoded once and dereferenced every frame with processEvents() running
	// in between - if one goes stale, the fault report has to name it.
	int nodeIndex = 0;
	int moved = 0;

	for (const auto& node : m_exportableNodes)
	{
		DthFaultProbe::setNodeIndex(nodeIndex);
		DthFaultProbe::setNode(node->getLabel());

		if (writeObject(node, firstFrame)) moved++;

		nodeIndex++;
	}

	DthFaultProbe::setNodeIndex(-1);

	return moved;
}

void Sagan::AlembicNodeDecoder::refreshExportedGeometry() const
{
	for (DzNode* node : m_exportableNodes)
	{
		if (DazStaticHelpers::isStrandBasedHair(node)) continue;

		if (DzObject* object = node->getObject())
		{
			// isRender = TRUE - the RENDER-path evaluation, not the viewport
			// flavor. Everything tried with isRender=false (per-node here,
			// DzScene::update() upstream, event pumping) was measured
			// ineffective against the follower freeze: 400+ firings per
			// degraded run, zero frames recovered, five sessions. Renders are
			// never wrong about followers, so the render flavor is the last
			// untried public lever below forceCacheUpdate() (the known crash).
			object->update(*node, true);
			object->finalize(*node, true, false);
		}
	}
}

bool Sagan::AlembicNodeDecoder::writeObject(const DzNode* node, bool firstFrame) const
{
	const auto label = node->getLabel();
	const auto& exportableMeshObjectPtr = saganExporter->getExportableMeshObjects().at(label);
	const auto vertices = exportableMeshObjectPtr->getMeshVertices();

	std::vector<Imath::V3f> alembicVertices;

	// The motion hash accumulates in the transform loop that has to run
	// anyway - no second pass, one uint64 per mesh. FNV-1a over the exact
	// float bits that go into the archive: if this hash repeats, the sample
	// IS a byte-identical repeat (and Ogawa will deduplicate it on disk).
	// A bounding box was tried first and lied - see MeshMotion in the header.
	std::uint64_t frameHash = 1469598103934665603ull;

	for (const auto& vertex : vertices)
	{
		const auto transformedVertex = saganExporter->getOutputTransformer()->vertex(vertex);
		const Imath::V3f v(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
		alembicVertices.push_back(v);

		for (int axis = 0; axis < 3; axis++)
		{
			std::uint32_t bits;
			std::memcpy(&bits, &v[axis], sizeof(bits));

			for (int byte = 0; byte < 4; byte++)
			{
				frameHash ^= (bits >> (byte * 8)) & 0xff;
				frameHash *= 1099511628211ull;
			}
		}
	}

	bool movedThisFrame = false;

	{
		MeshMotion& motion = m_motionByLabel[label];

		motion.framesWritten++;

		if (motion.haveLastHash && frameHash != motion.lastHash)
		{
			motion.framesMoved++;
			movedThisFrame = true;
		}

		motion.lastHash = frameHash;
		motion.haveLastHash = true;
	}

	auto& meshSchema = saganExporter->getAlembicMeshObjects().at(label)->getSchema();

	if (firstFrame)
	{
		Alembic::AbcGeom::OPolyMeshSchema::Sample psamp;
		psamp.setPositions(Alembic::Abc::V3fArraySample(alembicVertices));

		auto const& faceVertexCounts = exportableMeshObjectPtr->faceVertexCounts;
		auto const& faceVertexIndices = exportableMeshObjectPtr->faceVertexIndices;
		auto const& uvArray = exportableMeshObjectPtr->uvs;
		auto& facetsByMaterialIndex = exportableMeshObjectPtr->facetsByMaterialIndex;

		psamp.setFaceIndices(Alembic::Abc::Int32ArraySample(faceVertexIndices));
		psamp.setFaceCounts(Alembic::Abc::Int32ArraySample(faceVertexCounts));

		const auto& uvs = exportableMeshObjectPtr->uvs;
		psamp.setUVs(Alembic::AbcGeom::OV2fGeomParam::Sample(uvs, Alembic::AbcGeom::kFacevaryingScope));

		meshSchema.set(psamp);

		auto& materialGroupNames = exportableMeshObjectPtr->materialGroupNames;

		for (const auto& [materialIndex, facets] : facetsByMaterialIndex)
		{
			auto& qMaterialSlotName = materialGroupNames[materialIndex];
			const auto materialSlotName = Sagan::toStdString(qMaterialSlotName);

			Alembic::AbcGeom::OFaceSet faceSet = meshSchema.getFaceSet(materialSlotName);

			auto& faceSetSchema = faceSet.getSchema();

			Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSchemaSample(facets);
			faceSetSchema.set(faceSetSchemaSample);
		}
	}
	else
	{
		Alembic::AbcGeom::OPolyMeshSchema::Sample psamp(alembicVertices);

		try
		{
			meshSchema.set(psamp);
		}
		catch (const std::runtime_error& e)
		{
			//Logger::getInstance().log(FILEANDLINE) << e.what();
		}
		catch (...)
		{
			//Logger::getInstance().log(FILEANDLINE) << "something went wrong";
		}
	}

	return movedThisFrame;
}

Sagan::ExportableNodes Sagan::AlembicNodeDecoder::getExportableNodes() const
{
	return m_exportableNodes;
}

std::shared_ptr<Alembic::AbcGeom::OObject> Sagan::AlembicNodeDecoder::getTopLevelObjectPointer()
{
	const auto topObjPtr = std::make_shared< Alembic::AbcGeom::OObject>(saganExporter->getArchive()->getTop());
	return topObjPtr;
}

std::string Sagan::AlembicNodeDecoder::getFormattedShapeNameAsString(DzNode* node)
{
	if (nodeNameFormatter_)
	{
		return nodeNameFormatter_(node);
	}

	throw std::runtime_error("No callback set");

	return "No callback set";
}

void Sagan::AlembicNodeDecoder::setShapeNameFormatter(NodeNameFormatterCallbackType nodeNameFormatter)
{
	nodeNameFormatter_ = nodeNameFormatter;
}