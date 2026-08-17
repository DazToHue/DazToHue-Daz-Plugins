#pragma once

#include <QTCore/QObject>
#include <QTCore/QString>
#include <fbxsdk.h>

class OpenFBXInterface
{

public:
	static OpenFBXInterface* GetInterface()
	{
		if (singleton_ == nullptr)
		{
			singleton_ = new OpenFBXInterface();
		}
		return singleton_;
	}

	OpenFBXInterface();
	~OpenFBXInterface();

	bool loadScene(FbxScene* scene, QString filename);
	bool saveScene(FbxScene* scene, QString filename, int fileFormat = -1, bool embedMedia = false);
	FbxScene* createScene(QString sceneName);
	bool loadScene(QString filename) { return loadScene(defaultScene_, filename); };
	bool saveScene(QString filename, int fileFormat = -1, bool embedMedia = false) { return saveScene(defaultScene_, filename, fileFormat, embedMedia); };
	FbxManager* getManager() { return fbxManager_; }
	FbxIOSettings* GetSettigns() { return fbxIOSettings_; }
	QString GetErrorString() { return errorString_; }
	int GetErrorCode() { return errorCode_; }
	FbxGeometry* findGeometry(FbxScene* scene, QString geometryName);
	FbxNode* findNode(FbxScene* scene, QString nodeName);

protected:

	static OpenFBXInterface* singleton_;
	FbxManager* fbxManager_;
	FbxIOSettings* fbxIOSettings_;
	FbxScene* defaultScene_;
	QString errorString_;
	int errorCode_;

};
