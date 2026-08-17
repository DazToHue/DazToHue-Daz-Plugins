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
#include <vector>

#include <QtCore/QString>

#include <dznode.h>
#include <dzquat.h>
#include <dzfloatproperty.h>

#include "output_transformer.h"

namespace Sagan
{

	enum FigureGeneration
	{
		Genesis9,
		Genesis81Female,
		Genesis81Male,
		Genesis8Female,
		Genesis8Male,
		Genesis3Female,
		Genesis3Male,
		Genesis2Female,
		Genesis2Male,
		Genesis1,
		M4,
		V4
	};

	enum Visual
	{

		Up,
		Down,
		Left,
		Right,
		Forward,
		Backward

	};

	using Rest = std::map< QString, Visual >;

	struct FigureInfo;
	using FigureGenerationInfo = std::map< QString, FigureInfo >;

	struct FigureInfo
	{

		static Rest m4;
		static Rest v4;
		static Rest g1;
		static Rest g2;
		static Rest g3_8_81;
		static Rest g9;

		QString presentation;
		FigureGeneration generation;
		Rest* restOrientations;
		QString rootBoneName;

		static FigureGenerationInfo figureGenerationInfo;

	};

	struct BoneInfo
	{

		BoneInfo()
		{

			centerPoint = { 0, 0, 0 };
			endPoint = { 0, 0, 0 };
			orientation = { 0, 0, 0 };
			rotationOrder = DzRotationOrder::XYZ;

		}

		explicit BoneInfo(const DzNode* node)
		{

			const auto o = node->getOrigin();
			centerPoint[0] = o[0];
			centerPoint[1] = o[1];
			centerPoint[2] = o[2];

			const auto e = node->getEndPoint();
			endPoint[0] = e[0];
			endPoint[1] = e[1];
			endPoint[2] = e[2];

			const auto orientControlX = node->getOrientXControl();
			orientation[0] = orientControlX->getValue();
			const auto orientControlY = node->getOrientYControl();
			orientation[1] = orientControlY->getValue();
			const auto orientControlZ = node->getOrientZControl();
			orientation[2] = orientControlZ->getValue();

			rotationOrder = node->getRotationOrder();

		}

		Vector centerPoint;
		Vector endPoint;
		Orientation orientation;
		DzRotationOrder rotationOrder;

		std::map< QString, BoneInfo > children;

	};

	static Quaternion blender2DAZQuat(const BoneInfo& boneNode, const Quaternion& blenderQuaternion, const Rest* restOrientations, const QString& boneName);

};