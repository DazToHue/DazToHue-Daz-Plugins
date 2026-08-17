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

#include <tuple>
#include <array>
#include <optional>
#include <vector>
#include <map>
#include <unordered_set>

#include <QtCore/QSet>
#include <QtCore/QVector>

#include "../output_transformer/output_transformer.h"

#include "Alembic/Abc/OArchive.h"
#include "Alembic/AbcCoreOgawa/ReadWrite.h"
#include "Alembic/AbcGeom/OPolyMesh.h"
#include "Alembic/AbcGeom/OCurves.h"
#include "Alembic/AbcGeom/OXform.h"

#include "dznode.h"
#include "dzfacetmesh.h"
// dzgeometryshell.h is SDK4-only and not needed: geoshells are matched by className().

namespace Sagan
{

	using GeoGraftFigureLabels = QSet<QString>;

	// Relative to DAZ Studio
	enum class WindingOrder
	{
		Alembic,
		USD
	};

	using UVArray = std::vector< std::array<float, 2>>;
	using AlembicUV = Alembic::Abc::V2f;
	using AlembicUVSet = std::vector<AlembicUV>;
	using HiddenFaces = QSet<int>;
	using Face = std::array<int, 4>;
	using Faces = std::vector<Face>;
	using Visible2OriginalVertexIndices = std::vector<int>;
	using Original2VisibleVertxIndices = std::map<int, int>;
	using FaceMaterialGroups = QVector<int>;
	using FacetsByMaterialIndex = std::map<int, std::vector<int>>;
	using MaterialGroupNames = std::vector<QString>;
	using FaceVertexCounts = std::vector<int>;
	using FaceVertexIndices = std::vector<int>;

	DzFacetMesh* getFacetMesh(const DzNode* node);
	std::tuple<UVArray, std::vector<QString>, FacetsByMaterialIndex, std::vector<int>, std::vector<int>, Visible2OriginalVertexIndices>	getImmutables(const DzNode* node, const WindingOrder windingOrder, const HiddenFaces& hiddenFaces = HiddenFaces());
	int determineVertexWithinFace(const int faceVertexCount, const int vertexIndex, const WindingOrder windingOrder);
	Vertices getOptimizedMeshVertices(const DzNode* node, const Visible2OriginalVertexIndices& visible2OriginalVertexIndices);
	Sagan::Vertices getSBHVertices(const DzNode* node);

};
