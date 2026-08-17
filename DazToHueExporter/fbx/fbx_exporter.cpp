
#include <stdexcept>
#include <memory>

#include "../daz/daz_static_helpers.h"
#include "../dth/dth_static_helpers.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>

#include <dzapp.h>
#include <dzscene.h>
#include <dzprogress.h>
#include <dzexportmgr.h>
#include <dzexporter.h>
#include <dzfigure.h>
#include <dzbone.h>
#include <dzimageproperty.h>
#include <dzstringproperty.h>
#include <dznumericproperty.h>
#include <dzfloatproperty.h>
#include <dzcolorproperty.h>

#include "open_fbx_interface.h"
#include "open_subdivision_interface.h"
#include "fbx_exporter.h"

DthFbxExporter::DthFbxExporter(QString exportDirectory, QString characterName, DzNode* selectedRootNode, DazHelpers& dazTools, DthWriter* dthWriter, DzProgress& exportProgress, DthLogger* dthLogger) : exportDirectory_(exportDirectory), characterName_(characterName), selectedRootNode_(selectedRootNode), dazHelpers_(dazTools), dthWriter_(dthWriter), exportProgress_(exportProgress), dthLogger_(dthLogger)
{
}

DthFbxExporter::~DthFbxExporter()
{
}

void DthFbxExporter::exportRoms()
{
	int startFrame = dzScene->getPlayRange().getStart() / dzScene->getTimeStep();

	// Export base mesh
	exportBaseFbx();

	// Export subdivided mesh if we have any subdiviions
	if (dazHelpers_.hasSubdivisions())
	{
		exportSubdividedFbx();

		// If we have a subdivided fbx then remove the base fbx
		if (QFile::exists(characterBaseExportPath_))
		{
			QFile::remove(characterBaseExportPath_);
		}

		if (dthWriter_ != nullptr) dthWriter_->setFbxRomPath(characterHDExportPath_);
	}

	dzScene->setFrame(startFrame);

	QApplication::processEvents();
}

void DthFbxExporter::exportBaseFbx()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting base FBX mesh"));

	exportProgress_.setCurrentInfo("Exporting " + selectedRootNode_->getLabel() + " base mesh");

	// Initialise Daz exporter
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Initialising Daz export manager"));
	DzExportMgr* ExportManager = dzApp->getExportMgr();
	DzExporter* Exporter = ExportManager->findExporterByClassName("DzFbxExporter");

	if (Exporter)
	{
		// Set exporter options
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Setting FBX export options"));
		DzFileIOSettings* exportOptions = new DzFileIOSettings();
		setFbxRomExportOptions(*exportOptions);

		// Construct base fbx file path
		if (dazHelpers_.hasSubdivisions())
		{
			characterBaseExportPath_ = exportDirectory_ + "/" + characterName_ + "_base.fbx";
		}
		else
		{
			characterBaseExportPath_ = exportDirectory_ + "/" + characterName_ + ".fbx";
		}

		// Export the base mesh
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Writing base FBX file"));
		Exporter->writeFile(this->characterBaseExportPath_, exportOptions);

		if (dthWriter_ != nullptr) dthWriter_->setFbxRomPath(characterBaseExportPath_);
	}

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Finished exporting base FBX mesh"));

	exportProgress_.step();

	return;
}

void DthFbxExporter::exportSubdividedFbx()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting subdivided FBX mesh"));

	exportProgress_.setCurrentInfo("Exporting " + selectedRootNode_->getLabel() + " subdivided mesh");

	dazHelpers_.setUserDefinedSubdivisionLevels();

	// Initialise Daz exporter
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Initialising Daz export manager"));
	DzExportMgr* ExportManager = dzApp->getExportMgr();
	DzExporter* Exporter = ExportManager->findExporterByClassName("DzFbxExporter");

	if (Exporter)
	{
		// Set exporter options
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Setting FBX export options"));
		DzFileIOSettings* exportOptions = new DzFileIOSettings();
		setFbxRomExportOptions(*exportOptions);

		// Construct base fbx path
		characterHDExportPath_ = exportDirectory_ + "/" + characterName_ + ".fbx";

		// Export the base mesh
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Writing subdivided FBX file"));
		Exporter->writeFile(this->characterHDExportPath_, exportOptions);

		// Subdivide skin weights
		characterHDExportPath_ = exportDirectory_ + "/" + characterName_ + ".fbx";
		subdivideFbx(characterBaseExportPath_, characterHDExportPath_, characterHDExportPath_, dazHelpers_.getSubdivisionLevelMap());
	}

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Finished exporting subdivided FBX mesh"));

	exportProgress_.step();

	return;
}

void DthFbxExporter::exportReferenceSkeleton(int frame)
{
	exportProgress_.setCurrentInfo("Exporting reference frame " + QString::number(frame));

	// Initialise Daz exporter
	DzExportMgr* ExportManager = dzApp->getExportMgr();
	DzExporter* Exporter = ExportManager->findExporterByClassName("DzFbxExporter");

	if (Exporter)
	{
		// Set exporter options
		DzFileIOSettings* exportOptions = new DzFileIOSettings();
		setFbxReferenceSkeletonExportOptions(*exportOptions);

		// Construct reference frame fbx file path
		QDir qDir(exportDirectory_ + "/Reference Skeletons/");
		if (!qDir.exists())
		{
			qDir.mkpath(exportDirectory_ + "/Reference Skeletons/");
		}
		referenceFrameExportPath_ = exportDirectory_ + "/Reference Skeletons/" + characterName_ + "_frame_" + QString::number(frame) + ".fbx";

		// Export the base mesh
		Exporter->writeFile(referenceFrameExportPath_, exportOptions);

		if (dthWriter_ != nullptr) dthWriter_->addReferenceSkeletonPath(QString::number(frame), referenceFrameExportPath_);
	}

	exportProgress_.step();

	return;
}

void DthFbxExporter::exportExperimentalRomAnimation()
{
	characterExperimentalRomExportPath_ = exportDirectory_ + "/" + characterName_ + "_experimental_rom.fbx";

	DzSkeleton* skeleton = selectedRootNode_->getSkeleton();
	DzFigure* figure = skeleton ? qobject_cast<DzFigure*>(skeleton) : NULL;

	if (!figure) return;

	// Setup FBX Exporter
	FbxManager* sdkManager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);

	int fileFormat = -1;
	fileFormat = sdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();

	FbxExporter* exporter = FbxExporter::Create(sdkManager, "");
	if (!exporter->Initialize(characterExperimentalRomExportPath_.toUtf8().data(), fileFormat, sdkManager->GetIOSettings()))
	{
		return;
	}

	// Get the Figure Scale
	float figureScale = selectedRootNode_->getScaleControl()->getValue();

	// Create the Scene
	FbxScene* scene = FbxScene::Create(sdkManager, "");

	FbxAnimStack* animStack = FbxAnimStack::Create(scene, "AnimStack");
	FbxAnimLayer* animBaseLayer = FbxAnimLayer::Create(scene, "Layer0");
	animStack->AddMember(animBaseLayer);

	// Add the skeleton to the scene
	QMap<DzNode*, FbxNode*> boneMap;
	generateSkeleton(figure, selectedRootNode_, nullptr, nullptr, scene, boneMap);

	// Get the play range
	DzTimeRange PlayRange = dzScene->getPlayRange();

	// Root Node
	exportNodeAnimation(figure, boneMap, animBaseLayer, figureScale);

	// Iterate the bones
	DzBoneList Bones = dazHelpers_.getAllBones(selectedRootNode_);
	for (auto Bone : Bones)
	{
		exportNodeAnimation(Bone, boneMap, animBaseLayer, figureScale);
	}

	// Write the FBX
	exporter->Export(scene);
	exporter->Destroy();

	if (dthWriter_ != nullptr) dthWriter_->setExperimentalRomPath(characterExperimentalRomExportPath_);
}

void DthFbxExporter::exportAnimationOnly(QString animationName)
{
	exportProgress_.setCurrentInfo("Exporting " + animationName + " animation");

	DzExportMgr* exportManager = dzApp->getExportMgr();
	DzExporter* exporter = exportManager->findExporterByClassName("DzFbxExporter");

	if (exporter)
	{
		dazHelpers_.setBaseSubdivisionLevels();

		//dazHelpers_.unlockSubdivisionLevels();

		DzFileIOSettings* exportOptions = new DzFileIOSettings();
		setFbxRomExportOptions(*exportOptions);

		characterBaseExportPath_ = exportDirectory_ + "/" + characterName_ + "_" + animationName + "_animation.fbx";

		exporter->writeFile(this->characterBaseExportPath_, exportOptions);

		dazHelpers_.setUserDefinedSubdivisionLevels();
	}

	return;
}

bool DthFbxExporter::subdivideFbx(QString baseFilePath, QString hdFilePath, QString outFilePath, std::map<std::string, int>* lookupTable)
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Subdividing mesh"));

	exportProgress_.setCurrentInfo("Subdividing mesh");

	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* baseMeshScene = openFBX->createScene("Base Mesh Scene");
	if (openFBX->loadScene(baseMeshScene, baseFilePath.toUtf8().data()) == false)
	{
		QMessageBox::warning(0, "Error", "An error occurred while loading the base scene", QMessageBox::Ok);
		return false;
	}

	SubdivideFbxScene subdivider = SubdivideFbxScene(baseMeshScene, lookupTable);
	subdivider.ProcessScene();
	FbxScene* hdMeshScene = openFBX->createScene("HD Mesh Scene");
	if (openFBX->loadScene(hdMeshScene, hdFilePath.toUtf8().data()) == false)
	{
		QMessageBox::warning(0, "Error", "An error occurred while loading the subdivided scene", QMessageBox::Ok);
		return false;
	}

	subdivider.SaveClustersToScene(hdMeshScene);
	if (openFBX->saveScene(hdMeshScene, outFilePath.toUtf8().data()) == false)
	{
		QMessageBox::warning(0, "Error", "An error occurred while saving the subdivided scene", QMessageBox::Ok);
		return false;
	}

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Finished subdividing mesh"));

	return true;
}

void DthFbxExporter::generateSkeleton(DzFigure* figure, DzNode* node, DzNode* parent, FbxNode* fbxParent, FbxScene* scene, QMap<DzNode*, FbxNode*>& boneMap)
{
	FbxNode* boneNode;

	// Null parent is the root bone
	if (fbxParent == nullptr)
	{
		// Create a root bone.
		FbxSkeleton* skeletonAttribute = FbxSkeleton::Create(scene, "root");
		skeletonAttribute->SetSkeletonType(FbxSkeleton::eRoot);
		boneNode = FbxNode::Create(scene, "root");
		boneNode->SetNodeAttribute(skeletonAttribute);

		FbxNode* rootNode = scene->GetRootNode();
		rootNode->AddChild(boneNode);

		// Looks through the child nodes for more bones
		for (int childIndex = 0; childIndex < node->getNumNodeChildren(); childIndex++)
		{
			DzNode* childNode = node->getNodeChild(childIndex);
			generateSkeleton(figure, childNode, node, boneNode, scene, boneMap);
		}
	}
	else
	{
		// Child nodes need to be bones
		if (DzBone* bone = qobject_cast<DzBone*>(node))
		{
			// Create the bone
			FbxSkeleton* skeletonAttribute = FbxSkeleton::Create(scene, node->getName().toUtf8().data());
			skeletonAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			boneNode = FbxNode::Create(scene, node->getName().toUtf8().data());
			boneNode->SetNodeAttribute(skeletonAttribute);

			// Find the bones position
			DzVec3 position = node->getWSPos(DzTime(0), true);
			DzVec3 parentPosition = parent->getWSPos(DzTime(0), true);
			DzVec3 localPosition = position - parentPosition;

			// Find the bones rotation
			DzQuat rotation = node->getWSRot(DzTime(0), true);
			DzQuat parentRotation = parent->getWSRot(DzTime(0), true);
			DzQuat localRotation = node->getOrientation(true);//Rotation * ParentRotation.inverse();
			DzVec3 vectorRotation;
			localRotation.getValue(vectorRotation);

			// Set the position and rotation properties
			boneNode->LclTranslation.Set(FbxVector4(localPosition.m_x, localPosition.m_y, localPosition.m_z));
			boneNode->LclRotation.Set(FbxVector4(vectorRotation.m_x, vectorRotation.m_y, vectorRotation.m_z));

			fbxParent->AddChild(boneNode);

			// Look through the child nodes for more bones
			QList<QString> directChildBones;
			for (int childIndex = 0; childIndex < node->getNumNodeChildren(); childIndex++)
			{
				DzNode* childNode = node->getNodeChild(childIndex);
				if (DzBone* childBone = qobject_cast<DzBone*>(childNode))
				{
					directChildBones.append(childBone->getName());
				}
				generateSkeleton(figure, childNode, node, boneNode, scene, boneMap);
			}

			// Add child figure bones
			for (int childFigureIndex = 0; childFigureIndex < figure->getNumNodeChildren(); childFigureIndex++)
			{
				DzNode* childNode = figure->getNodeChild(childFigureIndex);
				if (DzFigure* childFigure = qobject_cast<DzFigure*>(childNode))
				{
					// Find matching parent bone in child figures
					if (DzNode* childFigureMatchingParentBone = childFigure->findBone(node->getName()))
					{
						// Look for new child bones
						for (int childBoneIndex = 0; childBoneIndex < childFigureMatchingParentBone->getNumNodeChildren(); childBoneIndex++)
						{
							DzNode* childNode = childFigureMatchingParentBone->getNodeChild(childBoneIndex);
							if (DzBone* childBone = qobject_cast<DzBone*>(childNode))
							{
								if (!directChildBones.contains(childBone->getName()))
								{
									directChildBones.append(childBone->getName());
									generateSkeleton(figure, childNode, node, boneNode, scene, boneMap);
								}
							}
						}
					}
				}
			}
		}
	}

	// Add the bone to the map
	boneMap.insert(node, boneNode);
}

void DthFbxExporter::exportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, FbxAnimLayer* AnimBaseLayer, float FigureScale)
{
	DzTimeRange PlayRange = dzScene->getPlayRange();

	QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

	// Get a tick size for the progress bar
	int progressTickSize = (PlayRange.getEnd() - PlayRange.getStart() + 50) / 50;

	QString Name = Bone->getName();

	FbxNode* Node = BoneMap.value(Bone);
	if (Node == nullptr) return;

	// Create a curve node for this bone
	FbxAnimCurveNode* AnimCurveNode = Node->LclRotation.GetCurveNode(AnimBaseLayer, true);

	// For each frame, write a key (equivalent of bake)
	for (DzTime CurrentTime = PlayRange.getStart(); CurrentTime <= PlayRange.getEnd(); CurrentTime += dzScene->getTimeStep())
	{
		DzTime Frame = CurrentTime / dzScene->getTimeStep();

		// Need this for the UI to update, but it's very slow, so run every 100th frame.
		if (Frame % progressTickSize == 0)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}

		DzVec3 DefaultPosition;
		DefaultPosition.m_x = Bone->getOriginXControl()->getValue(CurrentTime);
		DefaultPosition.m_y = Bone->getOriginYControl()->getValue(CurrentTime);
		DefaultPosition.m_z = Bone->getOriginZControl()->getValue(CurrentTime);
		DzMatrix3 Scale = Bone->getWSScale();

		DzVec3 Position;
		Position.m_x = Bone->getXPosControl()->getValue(CurrentTime);
		Position.m_y = Bone->getYPosControl()->getValue(CurrentTime);
		Position.m_z = Bone->getZPosControl()->getValue(CurrentTime);

		// Get an initial rotation via the controls
		DzVec3 ControlRotation;
		ControlRotation.m_x = Bone->getXRotControl()->getValue(CurrentTime);
		ControlRotation.m_y = Bone->getYRotControl()->getValue(CurrentTime);
		ControlRotation.m_z = Bone->getZRotControl()->getValue(CurrentTime);

		DzVec3 VectorRotation = ControlRotation;

		// Scale
		DzVec3 ControlScale(1.0f, 1.0f, 1.0f);
		FigureScale = Bone->getScaleControl()->getValue(CurrentTime);
		ControlScale.m_x = Bone->getXScaleControl()->getValue(CurrentTime) * FigureScale;
		ControlScale.m_y = Bone->getYScaleControl()->getValue(CurrentTime) * FigureScale;
		ControlScale.m_z = Bone->getZScaleControl()->getValue(CurrentTime) * FigureScale;

		// Get the rotation and position relative to the parent
		if (DzNode* ParentBone = Bone->getNodeParent())
		{

			// Get the local orientation
			DzQuat Orientation = Bone->getOrientation(true) * ParentBone->getOrientation(true).inverse();

			// Fix the rotation order
			VectorRotation = ControlRotation;
			DzQuat ReorderQuat;
			VectorRotation.m_x = VectorRotation.m_x / FBXSDK_180_DIV_PI;
			VectorRotation.m_y = VectorRotation.m_y / FBXSDK_180_DIV_PI;
			VectorRotation.m_z = VectorRotation.m_z / FBXSDK_180_DIV_PI;
			ReorderQuat.setValue(Bone->getRotationOrder().order(), VectorRotation);
			ReorderQuat = ReorderQuat * Orientation;

			ReorderQuat.getValue(DzRotationOrder::RotOrder::XYZ, VectorRotation);
			VectorRotation.m_x = VectorRotation.m_x * FBXSDK_180_DIV_PI;
			VectorRotation.m_y = VectorRotation.m_y * FBXSDK_180_DIV_PI;
			VectorRotation.m_z = VectorRotation.m_z * FBXSDK_180_DIV_PI;

			DzVec3 ParentPosition;
			ParentPosition.m_x = ParentBone->getXPosControl()->getValue(CurrentTime);
			ParentPosition.m_y = ParentBone->getYPosControl()->getValue(CurrentTime);
			ParentPosition.m_z = ParentBone->getZPosControl()->getValue(CurrentTime);

			DzVec3 DefaultParentPosition;
			DefaultParentPosition.m_x = ParentBone->getOriginXControl()->getValue(CurrentTime);
			DefaultParentPosition.m_y = ParentBone->getOriginYControl()->getValue(CurrentTime);
			DefaultParentPosition.m_z = ParentBone->getOriginZControl()->getValue(CurrentTime);

			DzVec3 RelativeDefaultPosition = DefaultPosition - DefaultParentPosition;
			//float Length = RelativeDefaultPosition.length();
			DzVec3 OrientedRelativeDefaultPosition = ParentBone->getOrientation(true).inverse().multVec(RelativeDefaultPosition);

			DzVec3 RelativeMovement = Position - ParentPosition;
			if (ParentBone->isRootNode())
			{
				Position = (Position + OrientedRelativeDefaultPosition) * FigureScale;
			}
			else
			{
				Position = OrientedRelativeDefaultPosition + RelativeMovement;
			}
		}

		// Set the frame
		FbxTime Time;
		int KeyIndex = 0;
		Time.SetFrame(Frame);

		// Write X Rot
		FbxAnimCurve* RotXCurve = Node->LclRotation.GetCurve(AnimBaseLayer, "X", true);
		RotXCurve->KeyModifyBegin();
		KeyIndex = RotXCurve->KeyAdd(Time);
		RotXCurve->KeySet(KeyIndex, Time, VectorRotation.m_x);
		RotXCurve->KeyModifyEnd();

		// Write Y Rot
		FbxAnimCurve* RotYCurve = Node->LclRotation.GetCurve(AnimBaseLayer, "Y", true);
		RotYCurve->KeyModifyBegin();
		KeyIndex = RotYCurve->KeyAdd(Time);
		RotYCurve->KeySet(KeyIndex, Time, VectorRotation.m_y);
		RotYCurve->KeyModifyEnd();

		// Write Z Rot
		FbxAnimCurve* RotZCurve = Node->LclRotation.GetCurve(AnimBaseLayer, "Z", true);
		RotZCurve->KeyModifyBegin();
		KeyIndex = RotZCurve->KeyAdd(Time);
		RotZCurve->KeySet(KeyIndex, Time, VectorRotation.m_z);
		RotZCurve->KeyModifyEnd();

		// Write X Pos
		FbxAnimCurve* PosXCurve = Node->LclTranslation.GetCurve(AnimBaseLayer, "X", true);
		PosXCurve->KeyModifyBegin();
		KeyIndex = PosXCurve->KeyAdd(Time);
		PosXCurve->KeySet(KeyIndex, Time, Position.m_x);
		PosXCurve->KeyModifyEnd();

		// Write Y Pos
		FbxAnimCurve* PosYCurve = Node->LclTranslation.GetCurve(AnimBaseLayer, "Y", true);
		PosYCurve->KeyModifyBegin();
		KeyIndex = PosYCurve->KeyAdd(Time);
		PosYCurve->KeySet(KeyIndex, Time, Position.m_y);
		PosYCurve->KeyModifyEnd();

		// Write Z Pos
		FbxAnimCurve* PosZCurve = Node->LclTranslation.GetCurve(AnimBaseLayer, "Z", true);
		PosZCurve->KeyModifyBegin();
		KeyIndex = PosZCurve->KeyAdd(Time);
		PosZCurve->KeySet(KeyIndex, Time, Position.m_z);
		PosZCurve->KeyModifyEnd();

		// Write X Scale
		FbxAnimCurve* ScaleXCurve = Node->LclScaling.GetCurve(AnimBaseLayer, "X", true);
		ScaleXCurve->KeyModifyBegin();
		KeyIndex = ScaleXCurve->KeyAdd(Time);
		ScaleXCurve->KeySet(KeyIndex, Time, ControlScale.m_x);
		ScaleXCurve->KeyModifyEnd();

		// Write Y Scale
		FbxAnimCurve* ScaleYCurve = Node->LclScaling.GetCurve(AnimBaseLayer, "Y", true);
		ScaleYCurve->KeyModifyBegin();
		KeyIndex = ScaleYCurve->KeyAdd(Time);
		ScaleYCurve->KeySet(KeyIndex, Time, ControlScale.m_y);
		ScaleYCurve->KeyModifyEnd();

		// Write Z Scale
		FbxAnimCurve* ScaleZCurve = Node->LclScaling.GetCurve(AnimBaseLayer, "Z", true);
		ScaleZCurve->KeyModifyBegin();
		KeyIndex = ScaleZCurve->KeyAdd(Time);
		ScaleZCurve->KeySet(KeyIndex, Time, ControlScale.m_z);
		ScaleZCurve->KeyModifyEnd();
	}

}

void DthFbxExporter::exportSkeletonReferenceFrames(QString referenceFrames)
{
	dazHelpers_.setBaseSubdivisionLevels();

	//dazHelpers_.unlockSubdivisionLevels();

	std::vector<int> frameNumbers = DthStaticHelpers::parseNumberString(referenceFrames.toStdString());

	for (int frame : frameNumbers)
	{
		if (DazStaticHelpers::isValidFrame(frame))
		{
			SkeletonReferenceFrameProperties skeletonReferenceFrameProperties;
			skeletonReferenceFrameProperties = getSkeletonReferenceFrameProperties(selectedRootNode_, frame);
			applyReferenceFrameProperties(skeletonReferenceFrameProperties);
			exportReferenceSkeleton(frame);
			restoreReferenceFrameProperties(skeletonReferenceFrameProperties);
		}
		else
		{
			exportProgress_.setInfo("Skipping invalid reference frame " + QString::number(frame));
		}
	}

	dazHelpers_.setUserDefinedSubdivisionLevels();

	QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void DthFbxExporter::applyReferenceFrameProperties(SkeletonReferenceFrameProperties skeletonReferenceFrameProperties)
{
	for (const auto& entry : skeletonReferenceFrameProperties)
	{
		int frame = entry.first;
		const std::vector<SkeletonReferenceFrameProperty>& referenceFrameProperties = entry.second;

		for (const auto& referenceFrameProperty : referenceFrameProperties)
		{
			DzProperty* property = referenceFrameProperty.property;
			DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
			double valueToSet = referenceFrameProperty.keyedValue;
			int frameZero = 0;
			DzTime ticksPerFrame = dzScene->getTimeStep();
			DzTime dzTimeZero = frameZero * ticksPerFrame;
			numericProperty->setDoubleValue(dzTimeZero, valueToSet);
		}
	}
}

void DthFbxExporter::restoreReferenceFrameProperties(SkeletonReferenceFrameProperties skeletonReferenceFrameProperties)
{
	for (const auto& entry : skeletonReferenceFrameProperties)
	{
		int frame = entry.first;
		const std::vector<SkeletonReferenceFrameProperty>& referenceFrameProperties = entry.second;

		for (const auto& referenceFrameProperty : referenceFrameProperties)
		{
			DzProperty* property = referenceFrameProperty.property;
			DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
			double valueToSet = referenceFrameProperty.frameZeroValue;
			int frameZero = 0;
			DzTime ticksPerFrame = dzScene->getTimeStep();
			DzTime dzTimeZero = frameZero * ticksPerFrame;
			numericProperty->setDoubleValue(dzTimeZero, valueToSet);
		}
	}
}

void DthFbxExporter::setFbxRomExportOptions(DzFileIOSettings& exportOptions)
{
	exportOptions.setIntValue("RunSilent", true);
	exportOptions.setBoolValue("doSelected", true);
	exportOptions.setBoolValue("doVisible", true);			// I think we generally want this set to true?
	exportOptions.setBoolValue("doFigures", true);
	exportOptions.setBoolValue("doProps", true);
	exportOptions.setBoolValue("doLights", false);
	exportOptions.setBoolValue("doCameras", false);
	exportOptions.setBoolValue("doAnims", true);
	exportOptions.setBoolValue("doMorphs", false);
	exportOptions.setStringValue("rules", "");
	exportOptions.setBoolValue("doLocks", false);
	exportOptions.setBoolValue("doLimits", false);
	exportOptions.setStringValue("format", "FBX 2020 -- Binary");
	exportOptions.setBoolValue("doEmbed", false);
	exportOptions.setBoolValue("doCopyTextures", false);
	exportOptions.setBoolValue("doDiffuseOpacity", false);
	exportOptions.setBoolValue("doMergeClothing", true);	// Also affects visibility of hair when exporting
	exportOptions.setBoolValue("doStaticClothing", false);
	exportOptions.setBoolValue("degradedSkinning", true);
	exportOptions.setBoolValue("degradedScaling", true);
	exportOptions.setBoolValue("doSubD", true);
	exportOptions.setBoolValue("doCollapseUVTiles", true);  // This used to be false
}

void DthFbxExporter::setFbxReferenceSkeletonExportOptions(DzFileIOSettings& exportOptions)
{
	exportOptions.setBoolValue("doSelected", true);
	exportOptions.setBoolValue("doVisible", true);
	exportOptions.setBoolValue("doFigures", true);
	exportOptions.setBoolValue("doProps", false);
	exportOptions.setBoolValue("doLights", false);
	exportOptions.setBoolValue("doCameras", false);
	exportOptions.setBoolValue("doAnims", false);
	exportOptions.setBoolValue("doLocks", false);
	exportOptions.setBoolValue("doLimits", false);
	exportOptions.setBoolValue("doMorphs", false);
	exportOptions.setStringValue("rules", "");
	exportOptions.setStringValue("format", "FBX 2020 -- Binary");
	exportOptions.setIntValue("RunSilent", true);
	exportOptions.setBoolValue("doEmbed", false);
	exportOptions.setBoolValue("doCopyTextures", false);
	exportOptions.setBoolValue("doDiffuseOpacity", false);
	exportOptions.setBoolValue("doMergeClothing", true);
	exportOptions.setBoolValue("doStaticClothing", false);
	exportOptions.setBoolValue("degradedSkinning", true);
	exportOptions.setBoolValue("degradedScaling", true);
	exportOptions.setBoolValue("doSubD", true);
	exportOptions.setBoolValue("doCollapseUVTiles", false);
}

DthFbxExporter::SkeletonReferenceFrameProperties DthFbxExporter::getSkeletonReferenceFrameProperties(DzNode* rootNode, int frame)
{
	std::vector<DthFbxExporter::SkeletonReferenceFrameProperty> referenceFrameProperties;

	QList<DzNode*> figureList;
	dazHelpers_.getFigures(rootNode, figureList);

	foreach(DzNode * figureNode, figureList)
	{
		DthFbxExporter::getFigureReferenceFrameProperties(figureNode, frame, referenceFrameProperties);
	}

	DthFbxExporter::SkeletonReferenceFrameProperties skeletonReferenceFrameProperties;
	skeletonReferenceFrameProperties[frame] = referenceFrameProperties;
	return skeletonReferenceFrameProperties;
}

void DthFbxExporter::getFigureReferenceFrameProperties(DzNode* figureNode, int frame, std::vector<SkeletonReferenceFrameProperty>& referenceFrameProperties)
{
	if (figureNode)
	{
		DzPropertyList propertyList;
		propertyList = dazHelpers_.getAllNodeProperties(figureNode);

		for (auto property : propertyList)
		{
			QString propertyName = property->getName();
			QString propertyLabel = property->getLabel();
			DzPresentation* presentation = property->getPresentation();

			if (presentation)
			{
				DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);

				if (numericProperty)
				{
					int frameZero = 0;
					int frameNumber = frame;
					DzTime ticksPerFrame = dzScene->getTimeStep();
					DzTime dzTimeZero = frameZero * ticksPerFrame;
					DzTime dzTimeValue = frameNumber * ticksPerFrame;

					double defaultValue = numericProperty->getDoubleDefaultValue();
					double valueAtZero = numericProperty->getDoubleValue(dzTimeZero);
					double valueAtFrame = numericProperty->getDoubleValue(dzTimeValue);
					int keyIndex;

					DzPresentation* presentation = property->getPresentation();

					if (numericProperty->isKey(dzTimeValue, keyIndex))
					{
						if (valueAtFrame != valueAtZero)
						{
							DthFbxExporter::SkeletonReferenceFrameProperty referenceFrameProperty = { figureNode, property, valueAtZero, valueAtFrame };
							referenceFrameProperties.push_back(referenceFrameProperty);
						}
					}
				}
			}
		}
	}
}
