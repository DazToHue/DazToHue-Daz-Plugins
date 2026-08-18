#pragma once

#include <fbxsdk.h>

#include <DzFileIOSettings.h>

#include "../daz/daz_helpers.h"
#include "../dth/dth_writer.h"
#include "../dth/dth_logger.h"

class DthFbxExporter
{

public:

	DthFbxExporter(QString exportDirectory, QString characterName, DzNode* selectedRootNode, DazHelpers& dazTools, DthWriter* r_dthWriter, DzProgress& exportProgress, DthLogger* r_dthLogger);
	~DthFbxExporter();

	struct SkeletonReferenceFrameProperty
	{
		DzNode* figure;
		DzProperty* property;
		double frameZeroValue;
		double keyedValue;
	};

	using SkeletonReferenceFrameProperties = std::map<int, std::vector<SkeletonReferenceFrameProperty>>;

	void exportRoms();
	void exportBaseFbx();
	void exportSubdividedFbx();
	void exportExperimentalRomAnimation();
	void exportAnimationOnly(QString animationName);
	bool subdivideFbx(QString baseFilePath, QString hdFilePath, QString outFilePath, std::map<std::string, int>* pLookupTable);
	void generateSkeleton(DzFigure* figure, DzNode* node, DzNode* parent, FbxNode* fbxParent, FbxScene* scene, QMap<DzNode*, FbxNode*>& boneMap, QMap<DzNode*, DzNode*>& exportParentMap);
	void exportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, QMap<DzNode*, DzNode*>& ExportParentMap, FbxAnimLayer* AnimBaseLayer, float FigureScale);
	void exportSkeletonReferenceFrames(QString referenceFrames);
	void applyReferenceFrameProperties(SkeletonReferenceFrameProperties skeletonReferenceFrameProperties);
	void restoreReferenceFrameProperties(SkeletonReferenceFrameProperties skeletonReferenceFrameProperties);
	void exportReferenceSkeleton(int frame);
	void getFigureReferenceFrameProperties(DzNode* figureNode, int frame, std::vector<SkeletonReferenceFrameProperty>& referenceFrameProperties);
	void setFbxRomExportOptions(DzFileIOSettings& exportOptions);
	void setFbxReferenceSkeletonExportOptions(DzFileIOSettings& exportOptions);
	SkeletonReferenceFrameProperties getSkeletonReferenceFrameProperties(DzNode* rootNode, int frame);

private:

	QString exportDirectory_;
	QString characterName_;
	DzNode* selectedRootNode_;
	DazHelpers& dazHelpers_;
	DthWriter* dthWriter_ = nullptr;
	DthLogger* dthLogger_ = nullptr;
	DzProgress& exportProgress_;
	QString characterBaseExportPath_;
	QString characterHDExportPath_;
	QString characterExperimentalRomExportPath_;
	QString referenceFrameExportPath_;

};