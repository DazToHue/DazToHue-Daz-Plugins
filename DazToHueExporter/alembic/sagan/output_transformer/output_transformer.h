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

#include <vector>
#include <array>
#include <tuple>
#include <map>

//#include <Eigen/Core>

#include "dzrotationorder.h"

namespace Sagan
{

	enum class InitialBoneOrientation { Up, Down, Left, Right, Forward, Backward };
	using InitialBoneOrientations = std::map< std::string, InitialBoneOrientation >;

	using Component = float;
	using Vertex = std::array< Component, 3>;
	using Vertices = std::vector<Vertex>;
	using Vector = Vertex;
	using Orientation = std::array< float, 3 >;
	using EulerAngles = std::array< float, 3 >;

	using Edge = std::pair< int, int >;
	using Edges = std::vector< Edge >;

	using Normal = std::array< Component, 3 >;
	using Normals = std::vector< Normal >;

	using Quaternion = std::array< double, 4 >;


	class OutputTransformer
	{
	public:
		OutputTransformer() {}

		virtual ~OutputTransformer() = default;

		virtual Vector vector(const Vector&) const = 0;
		virtual Vertex vertex(const Vertex&) const = 0;
		virtual EulerAngles	eulerXYZ(const EulerAngles&) const = 0;
		virtual Component scalar(const Component&) const = 0;
		virtual Quaternion quaternion(const Quaternion&) const = 0;

	};

	inline Vector operator-(const Vector& lhs, const Vector& rhs)
	{
		return Vector{ lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2] };
	}

	inline Vector operator+(const Vector& lhs, const Vector& rhs)
	{
		return Vector{ lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2] };
	}

}