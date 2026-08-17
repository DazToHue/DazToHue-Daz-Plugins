
/****************************************************************************************
 Portions of this file is based on source code from Autodesk and is used under license below:

   Copyright (C) 2015 Autodesk, Inc.
   All rights reserved.
   Use of this software is subject to the terms of the Autodesk license agreement
   provided at the time of installation or download, or which otherwise accompanies
   this software in either electronic or hard copy form.
****************************************************************************************/

#include <QTCore/QString>
#include <QTCore/qcoreapplication.h>
#include <fbxsdk.h>
#include <dzapp.h>
#include "open_fbx_interface.h"

OpenFBXInterface* OpenFBXInterface::singleton_ = nullptr;

OpenFBXInterface::OpenFBXInterface()
{
	FbxManager* result = FbxManager::Create();

	if (result == nullptr)
	{
		throw (std::runtime_error("OpenFBXInterface: could not create FbxManager"));
	}

	fbxManager_ = result;

	fbxIOSettings_ = FbxIOSettings::Create(fbxManager_, IOSROOT);

	fbxManager_->SetIOSettings(fbxIOSettings_);

	FbxString appPath = FbxGetApplicationDirectory();
	fbxManager_->LoadPluginsDirectory(appPath.Buffer());

	errorCode_ = 0;
	errorString_ = "";
	defaultScene_ = createScene("DefaultScene");
}

OpenFBXInterface::~OpenFBXInterface()
{
	if (defaultScene_) defaultScene_->Destroy();
	if (fbxIOSettings_) fbxIOSettings_->Destroy();
	if (fbxManager_) fbxManager_->Destroy();
}

bool OpenFBXInterface::saveScene(FbxScene* scene, QString filename, int fileFormat, bool embedMedia)
{
	bool status = true;

	FbxExporter* exporter = FbxExporter::Create(fbxManager_, "");

	//bool useAscii = false;

	//if (useAscii)
	//{
	//	int lFormatIndex, lFormatCount = fbxManager_->GetIOPluginRegistry()->GetWriterFormatCount();
	//	for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
	//	{
	//		if (fbxManager_->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
	//		{
	//			FbxString lDesc = fbxManager_->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
	//			if (lDesc.Find("ascii") >= 0)
	//			{
	//				fileFormat = lFormatIndex;
	//				break;
	//			}
	//		}
	//	}
	//}

	if (fileFormat < 0 || fileFormat >= fbxManager_->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		fileFormat = fbxManager_->GetIOPluginRegistry()->GetNativeWriterFormat();
	}

	fbxIOSettings_->SetBoolProp(EXP_FBX_MATERIAL, true);
	fbxIOSettings_->SetBoolProp(EXP_FBX_TEXTURE, true);
	fbxIOSettings_->SetBoolProp(EXP_FBX_EMBEDDED, embedMedia);
	fbxIOSettings_->SetBoolProp(EXP_FBX_SHAPE, true);
	fbxIOSettings_->SetBoolProp(EXP_FBX_GOBO, true);
	fbxIOSettings_->SetBoolProp(EXP_FBX_ANIMATION, true);
	fbxIOSettings_->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	if (exporter->Initialize(filename.toUtf8().data(), fileFormat, fbxIOSettings_) == false)
	{
		errorString_ = QString(exporter->GetStatus().GetErrorString());
		errorCode_ = exporter->GetStatus().GetCode();
		exporter->Destroy();
		return false;
	}

	bool nonBlocking = false;
	status = exporter->Export(scene, nonBlocking);
	if (!status)
	{
		errorString_ = QString(exporter->GetStatus().GetErrorString());
		errorCode_ = exporter->GetStatus().GetCode();
	}
	else
	{
		do
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		} while (exporter->IsExporting(status));

		if (!status)
		{
			errorString_ = QString(exporter->GetStatus().GetErrorString());
			errorCode_ = exporter->GetStatus().GetCode();
		}
	}

	exporter->Destroy();

	return status;
}

bool OpenFBXInterface::loadScene(FbxScene* scene, QString filename)
{
	bool status = true;

	FbxImporter* importer = FbxImporter::Create(fbxManager_, "");

	if (importer->Initialize(filename.toUtf8().data(), -1, fbxIOSettings_) == false)
	{
		errorString_ = QString(importer->GetStatus().GetErrorString());
		errorCode_ = importer->GetStatus().GetCode();

		if (errorCode_ == FbxStatus::EStatusCode::eFailure)
		{
			QString sErrorMessage = QString("OpenFbxInterface()::LoadScene(): FbxImporter::Initialize(%1) failed. [EStatusCode=%2]").arg(filename).arg(errorCode_);

			dzApp->warning("ERROR: DazToHue: " + sErrorMessage + " - retrying after 1 second wait....");

			int sleepTimeMs = 1000;

			_sleep(sleepTimeMs);

			if (importer->Initialize(filename.toUtf8().data(), -1, fbxIOSettings_) == false)
			{
				errorString_ = QString(importer->GetStatus().GetErrorString());
				errorCode_ = importer->GetStatus().GetCode();
				importer->Destroy();
				return false;
			}
			errorCode_ = 0;
			errorString_ = "";
		}
		else
		{
			importer->Destroy();
			return false;
		}
	}

	if (importer->IsFBX() == false)
	{
		errorCode_ = -1;
		errorString_ = QString("OpenFBXInterface: loaded scene file has unrecognized FBX file format.");
		importer->Destroy();
		return false;
	}

	status = importer->Import(scene);
	if (!status)
	{
		errorString_ = QString(importer->GetStatus().GetErrorString());
		errorCode_ = importer->GetStatus().GetCode();
	}

	importer->Destroy();
	return status;
}

FbxScene* OpenFBXInterface::createScene(QString sceneName)
{
	FbxScene* newScene = FbxScene::Create(fbxManager_, sceneName.toUtf8().data());

	return newScene;
}

FbxGeometry* OpenFBXInterface::findGeometry(FbxScene* scene, QString geometryName)
{
	int numGeometry = scene->GetGeometryCount();

	for (int i = 0; i < numGeometry; i++)
	{
		FbxGeometry* geo = scene->GetGeometry(i);
		FbxNode* node = geo->GetNode();
		auto raw_name = node->GetName();
		QString geoName(raw_name);
		if (geoName == geometryName)
		{
			return geo;
		}
	}

	return nullptr;
}

FbxNode* OpenFBXInterface::findNode(FbxScene* scene, QString nodeName)
{
	FbxString name(nodeName.toUtf8().data());
	auto result = scene->FindNodeByName(name);
	return result;
}
