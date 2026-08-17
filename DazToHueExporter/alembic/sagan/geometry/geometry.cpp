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

#include "geometry.h"

#include "../common/common.h"

#include <unordered_set>
#include <stdexcept>

#include "dzfacegroup.h"
#include "dzscene.h"
#include "dzscript.h"
// dzgeometryshell.h is SDK4-only and not needed: geoshells are matched by className().
#include "dzfigure.h"
#include "dzobject.h"

namespace
{

	Sagan::MaterialGroupNames getMaterialGroupNames(const DzNode* node)
	{

		Sagan::MaterialGroupNames materialGroupNames;

		auto facetMesh = Sagan::getFacetMesh(node);

		auto materialGroupCount = facetMesh->getNumMaterialGroups();

		for (decltype(materialGroupCount) materialGroupIndex = 0; materialGroupIndex < materialGroupCount; materialGroupIndex++)
		{

			auto materialGroup = facetMesh->getMaterialGroup(materialGroupIndex);
			auto materialGoupName = materialGroup->getName();
			materialGroupNames.push_back(materialGoupName);

		}

		return materialGroupNames;

	}

	QVector<QString> getFaceGroupNames(const DzFacetMesh* facetMesh)
	{

		QVector< QString > faceGrouplNames;

		auto faceGroupCount = facetMesh->getNumFaceGroups();

		for (decltype(faceGroupCount) faceGroupIndex = 0; faceGroupIndex < faceGroupCount; faceGroupIndex++)
		{

			auto faceGroup = facetMesh->getFaceGroup(faceGroupIndex);
			auto faceGroupName = faceGroup->getName();
			faceGrouplNames.push_back(faceGroupName);

		}

		return faceGrouplNames;

	}

	Sagan::FacetsByMaterialIndex groupFacetsByMaterialIndex(const Sagan::FaceMaterialGroups& faceMaterialGroups)
	{

		Sagan::FacetsByMaterialIndex facetsByMaterialIndex;

		for (int faceIndex = 0; faceIndex < faceMaterialGroups.size(); faceIndex++)
		{

			const auto materialGroupIndex = faceMaterialGroups.at(faceIndex);
			facetsByMaterialIndex[materialGroupIndex].push_back(faceIndex);

		}

		return facetsByMaterialIndex;

	}

}

DzFacetMesh* Sagan::getFacetMesh(const DzNode* node)
{

	auto object = node->getObject();

	if (!object) return nullptr;

	auto geometry = object->getCachedGeom();

	if (!geometry) return nullptr;

	auto facetMesh = dynamic_cast<DzFacetMesh*>(geometry);

	return facetMesh;

}

int Sagan::determineVertexWithinFace(const int faceVertexCount, const int vertexIndex, const Sagan::WindingOrder windingOrder)
{

	if (windingOrder == WindingOrder::Alembic)
	{

		return faceVertexCount - vertexIndex - 1;

	}
	else if (windingOrder == WindingOrder::USD)
	{

		return vertexIndex;

	}
	else
	{

		throw std::runtime_error("unsupported winding order");

	}

}

Sagan::Vertices Sagan::getOptimizedMeshVertices(const DzNode* node, const Visible2OriginalVertexIndices& visible2OriginalVertexIndices)
{

	Sagan::Vertices vertices;

	const auto facetMesh = getFacetMesh(node);
	auto verticesPtr = facetMesh->getVerticesPtr();

	for (size_t i = 0; i < visible2OriginalVertexIndices.size(); i++)
	{

		const auto originalVertexIndex = visible2OriginalVertexIndices.at(i);
		const auto v = verticesPtr[originalVertexIndex];
		vertices.push_back({ v[0], v[1], v[2] });

	}

	return vertices;

}

Sagan::Vertices Sagan::getSBHVertices(const DzNode* node)
{

	Sagan::Vertices vertices;

	const auto facetMesh = getFacetMesh(node);
	auto verticesPtr = facetMesh->getVerticesPtr();
	const auto vertexCount = facetMesh->getNumVertices();

	for (int i = 0; i < vertexCount; i++)
	{

		const auto v = verticesPtr[i];
		vertices.push_back({ v[0], v[1], v[2] });

	}

	return vertices;

}

namespace
{

	void doUV(const DzFacet& dazFace, const int faceVertexIndex, const DzPnt2* uvPtr, Sagan::UVArray& uvArray)
	{

		auto uvIndex = dazFace.m_uvwIdx[faceVertexIndex];
		const auto dzPnt2 = uvPtr[uvIndex];
		uvArray.push_back({ dzPnt2[0], dzPnt2[1] });

	}

	void doFaceVertex(const int dazFaceVertexIndex, const int faceVertexCount, const Sagan::WindingOrder windingOrder, const DzFacet& dazFace, const DzPnt2* uvPtr, Sagan::Original2VisibleVertxIndices& original2VisibleVertexIndices, std::unordered_set< int >& usedOriginalVertexIndices, Sagan::Face& back, Sagan::UVArray& uvArray, Sagan::Visible2OriginalVertexIndices& visible2OriginalVertexIndices)
	{

		const auto faceVertexIndex = determineVertexWithinFace(faceVertexCount, dazFaceVertexIndex, windingOrder);

		const auto vertexIndex = dazFace.m_vertIdx[faceVertexIndex];

		if (usedOriginalVertexIndices.count(vertexIndex) == 0)
		{

			usedOriginalVertexIndices.insert(vertexIndex);

			const int next = (int)visible2OriginalVertexIndices.size();
			visible2OriginalVertexIndices.push_back(vertexIndex);
			original2VisibleVertexIndices[vertexIndex] = next;

			back[dazFaceVertexIndex] = next;

		}
		else
		{

			back[dazFaceVertexIndex] = original2VisibleVertexIndices[vertexIndex];

		}

		doUV(dazFace, faceVertexIndex, uvPtr, uvArray);

	}

	void doFacesAndUVs(const int faceCount, const Sagan::HiddenFaces& hiddenFaces, const Sagan::WindingOrder windingOrder, const DzFacet* dazFaceArray, const DzPnt2* uvPtr, Sagan::UVArray& uvArray, Sagan::FaceMaterialGroups& faceMaterialGroups, Sagan::Faces& faces, Sagan::Visible2OriginalVertexIndices& visible2OriginalVertexIndices)
	{

		Sagan::Original2VisibleVertxIndices original2VisibleVertexIndices;
		std::unordered_set< int > usedOriginalVertexIndices;

		for (int i = 0; i < faceCount; i++)
		{

			if (hiddenFaces.contains(i)) continue;

			const auto& dazFace = dazFaceArray[i];

			const auto materialGroupIndex = dazFace.m_materialIdx;
			faceMaterialGroups.push_back(materialGroupIndex);

			faces.push_back(Sagan::Face());
			auto& back = faces.back();

			auto faceVertexCount = dazFace.m_vertIdx[3] >= 0 ? 4 : 3;

			doFaceVertex(0, faceVertexCount, windingOrder, dazFace, uvPtr, original2VisibleVertexIndices, usedOriginalVertexIndices, back, uvArray, visible2OriginalVertexIndices);
			doFaceVertex(1, faceVertexCount, windingOrder, dazFace, uvPtr, original2VisibleVertexIndices, usedOriginalVertexIndices, back, uvArray, visible2OriginalVertexIndices);
			doFaceVertex(2, faceVertexCount, windingOrder, dazFace, uvPtr, original2VisibleVertexIndices, usedOriginalVertexIndices, back, uvArray, visible2OriginalVertexIndices);

			if (faceVertexCount == 4)
			{

				doFaceVertex(3, faceVertexCount, windingOrder, dazFace, uvPtr, original2VisibleVertexIndices, usedOriginalVertexIndices, back, uvArray, visible2OriginalVertexIndices);

			}
			else
			{

				back[3] = -1;

			}

		}

	}

	std::tuple <Sagan::Faces, Sagan::FaceMaterialGroups, Sagan::UVArray, Sagan::Visible2OriginalVertexIndices> facesAndUVs(const DzNode* node, const Sagan::WindingOrder windingOrder, const Sagan::HiddenFaces& hiddenFaces)
	{

		auto facetMesh = Sagan::getFacetMesh(node);
		auto faceCount = facetMesh->getNumFacets();
		auto dazFaceArray = facetMesh->getFacetsPtr();

		Sagan::UVArray uvArray;
		auto uvMap = facetMesh->getUVs();
		auto uvPtr = uvMap->getPnt2ArrayPtr();

		Sagan::Faces faces;
		Sagan::FaceMaterialGroups faceMaterialGroups;
		Sagan::Visible2OriginalVertexIndices visible2OriginalVertexIndices;

		doFacesAndUVs(faceCount, hiddenFaces, windingOrder, dazFaceArray, uvPtr, uvArray, faceMaterialGroups, faces, visible2OriginalVertexIndices);

		return std::tuple< Sagan::Faces, Sagan::FaceMaterialGroups, Sagan::UVArray, Sagan::Visible2OriginalVertexIndices >(faces, faceMaterialGroups, uvArray, visible2OriginalVertexIndices);

	}

}

std::tuple <Sagan::UVArray, std::vector< QString >, Sagan::FacetsByMaterialIndex, std::vector<int>, std::vector<int>, Sagan::Visible2OriginalVertexIndices> Sagan::getImmutables(const DzNode* node, const Sagan::WindingOrder windingOrder, const HiddenFaces& hiddenFaces)
{

	const auto [faces, faceMaterialGroups, uvArray, visible2OriginalVertexIndices] = facesAndUVs(node, windingOrder, hiddenFaces);
	auto facetsByMaterialIndex = groupFacetsByMaterialIndex(faceMaterialGroups);

	FaceVertexCounts faceVertexCounts; // an array of the vertex counts of all the faces
	FaceVertexIndices faceVertexIndices; // and array of the vertex IDs of the vertices in all the faces

	for (int faceIndex = 0; faceIndex < faces.size(); faceIndex++)
	{

		int faceVertexCount = (faces[faceIndex][3] != -1) ? 4 : 3;

		faceVertexCounts.push_back(faceVertexCount);

		for (int i = 0; i < faceVertexCount; i++)
		{

			faceVertexIndices.push_back(faces[faceIndex][i]);

		}

	}

	auto materialGroupNames = getMaterialGroupNames(node);

	return std::tuple <Sagan::UVArray, std::vector<QString>, Sagan::FacetsByMaterialIndex, std::vector<int>, std::vector<int>, Visible2OriginalVertexIndices > {uvArray, materialGroupNames, facetsByMaterialIndex, faceVertexCounts, faceVertexIndices, visible2OriginalVertexIndices};

}
