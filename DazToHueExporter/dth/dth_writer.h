#pragma once

#include <memory>

#include <QtCore/QString>
#include <QFileDialog>

#include <dzjsonwriter.h>

#include "../daz/daz_helpers.h"

class DthWriter
{

public:

	DthWriter(QString exportDirectory, QString characterName, DzNode* selectedRootNode, DazHelpers& dazTools, DzProgress& exportProgress);
	~DthWriter();

	void setFbxRomPath(QString path);
	void setExperimentalRomPath(QString path);
	void setAlembicRomPath(QString path);
	void addReferenceSkeletonPath(QString frame, QString path);
	void writeFile();
	void writeAllMaterials(DzNode* Node, bool bRecursive = false);
	void writeStartMaterialBlock(DzNode* Node, DzMaterial* Material);
	void writeFinishMaterialBlock();
	void writeMaterialProperty(DzNode* Node, DzMaterial* Material, DzProperty* Property);
	void writePropertyTexture(QString sName, QString sLabel, double dValue, QString sType, QString sTexture);
	void writePropertyTexture(QString sName, QString sLabel, QString sValue, QString sType, QString sTexture);
	QString resolveTexturePath(const QString& texture);
	void writeDiscoveredTextures();
	void writeAllSubdivisions();
	void writeSubdivisionProperties(const QString& Name, int targetValue);
	void writeJointOrientation(DzBoneList& aBoneList);
	void writeReferenceSkeletonFilePaths();
	void writeFbxGeoshells();
	void writeAlembicGeoshells();
	void writeAlembicGeografts();
	void writeRigidFollowers();
	void addFbxGeoshell(QString name);
	void addAlembicGeoshell(QString name);
	void addAlembicGeograft(QString name);
	void addRigidFollower(QString followerName, QString parentBoneName);

private:

	QString exportDirectory_;
	QString characterName_;
	DzNode* selectedRootNode_;
	DazHelpers& dazHelpers_;
	DzProgress& exportProgress_;
	std::unique_ptr<QFile> dthFile_;
	std::unique_ptr<DzJsonWriter> dthWriter_;
	QString fbxRomPath_ = "";
	QString alembicRomPath_ = "";
	QString experimentalRomPath_ = "";
	QStringList discoveredTextures_;
	QMap<QString, QString> relocatedTexturePaths_;
	QStringList fbxGeoshells_;
	QStringList alembicGeoshells_;
	QStringList alembicGeografts_;
	QMap<QString, QString> rigidFollowers_;
	QMap<QString, QString> referenceSkeletonFiles_;

};