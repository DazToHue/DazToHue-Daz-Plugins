
#include <stdexcept>

#include <dzprogress.h>

#include "dth_writer.h"
#include "../dth/dth_static_helpers.h"
#include "../daz/daz_static_helpers.h"

#include <QMessageBox>

#include <dzbone.h>
#include <dzimageproperty.h>
#include <dzstringproperty.h>
#include <dznumericproperty.h>
#include <dzfloatproperty.h>
#include <dzcolorproperty.h>

#include "../version.h"

DthWriter::DthWriter(QString exportDirectory, QString characterName, DzNode* selectedRootNode, DazHelpers& dazTools, DzProgress& exportProgress) : exportDirectory_(exportDirectory), characterName_(characterName), selectedRootNode_(selectedRootNode), dazHelpers_(dazTools), exportProgress_(exportProgress)
{
	dthFilePath_ = exportDirectory_ + "/" + characterName_ + ".dth";
}

DthWriter::~DthWriter()
{
}

void DthWriter::writeFile()
{
	// The .dth is created HERE and nowhere earlier. It is the manifest that
	// says "this export landed", and every consumer - including the calling
	// scripts' restore path - reads its presence that way. Opening it in the
	// constructor left a 0-BYTE .dth behind whenever a later leg failed
	// (measured 2026-08-19..21, alongside a truncated .abc and no FBX), which
	// reads as a successful export to anything that only checks existence.
	dthFile_ = std::make_unique<QFile>(dthFilePath_);
	if (!dthFile_->open(QIODevice::WriteOnly))
	{
		throw std::runtime_error(QString("Could not open %1 for writing - it may be locked by another application.").arg(dthFilePath_).toUtf8().constData());
	}
	dthWriter_ = std::make_unique<DzJsonWriter>(dthFile_.get());

	dthWriter_->startObject(true);
	dthWriter_->addMember("DTH Version", QString::fromStdString(PLUGIN_VERSION_STRING));
	dthWriter_->addMember("Character Name", characterName_);
	dthWriter_->addMember("Fbx Rom Path", fbxRomPath_);
	dthWriter_->addMember("Alembic Rom Path", alembicRomPath_);
	dthWriter_->addMember("Experimental Rom Path", experimentalRomPath_);

	// Can be removed when we drop support for DazToMaya and Sagan
	dthWriter_->addMember("Asset Type", QString("Animation"));
	dthWriter_->addMember("Use Experimental Animation Transfer", false);

	writeAllMaterials(selectedRootNode_);
	writeDiscoveredTextures();
	writeFbxGeoshells();
	writeAlembicGeoshells();
	writeAlembicGeografts();
	writeRigidFollowers();
	writeAllSubdivisions();
	writeReferenceSkeletonFilePaths();
	DzBoneList aBoneList = dazHelpers_.getAllBones(selectedRootNode_);
	writeJointOrientation(aBoneList);

	dthWriter_->finishObject();
	dthFile_->close();
}

void DthWriter::writeAllMaterials(DzNode* node, bool bRecursive)
{
	if (node == nullptr)
		return;

	if (node->className() == "DzGeometryShellNode")
	{
		addFbxGeoshell(DthStaticHelpers::getFormattedShapeName(node));
	}

	if (node->className() == "DzRigidFollowNode")
	{
		for (int i = 0; i < node->getNumNodeChildren(); ++i)
		{
			DzNode* childNode = node->getNodeChild(i);
			if (childNode->inherits("DzBone")) continue;
			DzObject* object = node->getObject();
			DzShape* shape = object ? object->getCurrentShape() : NULL;
			if (shape || node->inherits("DzGroupNode") || node->inherits("DzInstanceNode"))
			{
				addRigidFollower(DthStaticHelpers::getFormattedShapeName(childNode), DazStaticHelpers::getClosestRigidFollowerBoneName(childNode));
			}
		}
	}

	if (!bRecursive)
	{
		dthWriter_->startMemberArray("Materials", true);
	}

	DzObject* object = node->getObject();
	DzShape* shape = object ? object->getCurrentShape() : nullptr;

	if (shape)
	{
		for (int i = 0; i < shape->getNumMaterials(); i++)
		{
			DzMaterial* material = shape->getMaterial(i);
			if (material)
			{

				if (dazHelpers_.isMaterialHidden(node->getLabel(), material->getName()))
				{
					continue;
				}

				auto propertyList = material->propertyListIterator();
				writeStartMaterialBlock(node, material);
				while (propertyList.hasNext())
				{
					writeMaterialProperty(node, material, propertyList.next());
				}
				writeFinishMaterialBlock();

			}
		}
	}

	DzNodeListIterator iterator = node->nodeChildrenIterator();
	while (iterator.hasNext())
	{
		DzNode* child = iterator.next();
		writeAllMaterials(child, true);
	}

	if (!bRecursive)
	{
		dthWriter_->finishArray();
	}
}

void DthWriter::writeStartMaterialBlock(DzNode* node, DzMaterial* material)
{
	if (node == nullptr || material == nullptr)
		return;

	dthWriter_->startObject(true);
	dthWriter_->addMember("Asset Name", node->getName());
	dthWriter_->addMember("Asset Label", node->getLabel());
	dthWriter_->addMember("Material Name", material->getName());
	dthWriter_->addMember("Material Type", material->getMaterialName());

	DzPresentation* presentation = node->getPresentation();
	if (presentation)
	{
		const QString presentationType = presentation->getType();
		dthWriter_->addMember("Value", presentationType);
	}
	else
	{
		dthWriter_->addMember("Value", QString("Unknown"));
	}

	dthWriter_->startMemberArray("Properties", true);
}

void DthWriter::writeFinishMaterialBlock()
{
	dthWriter_->finishArray();
	dthWriter_->finishObject();
}

void DthWriter::writeMaterialProperty(DzNode* node, DzMaterial* material, DzProperty* property)
{
	if (node == nullptr || material == nullptr || property == nullptr)
		return;

	QString name = property->getName();
	QString label = property->getLabel();
	QString textureName = "";
	QString propType = "";
	QString propValue = "";
	double propNumericValue = 0.0;
	bool useNumeric = false;

	DzImageProperty* imageProperty = qobject_cast<DzImageProperty*>(property);
	DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
	DzColorProperty* colourProperty = qobject_cast<DzColorProperty*>(property);
	if (imageProperty)
	{
		if (imageProperty->getValue())
		{
			textureName = imageProperty->getValue()->getFilename();
		}
		propValue = material->getDiffuseColor().name();
		propType = QString("Texture");
	}
	else if (colourProperty)
	{
		if (colourProperty->getMapValue())
		{
			textureName = colourProperty->getMapValue()->getFilename();
		}
		propValue = colourProperty->getColorValue().name();
		propType = QString("Color");
	}
	else if (numericProperty)
	{
		if (numericProperty->getMapValue())
		{
			textureName = numericProperty->getMapValue()->getFilename();
		}
		propType = QString("Double");
		propNumericValue = numericProperty->getDoubleValue();
		useNumeric = true;
	}
	else
	{
		return;
	}

	if (useNumeric)
		writePropertyTexture(name, label, propNumericValue, propType, textureName.toLower());
	else
		writePropertyTexture(name, label, propValue, propType, textureName.toLower());

	return;
}

void DthWriter::writePropertyTexture(QString name, QString label, double value, QString type, QString texture)
{
	texture = resolveTexturePath(texture);

	dthWriter_->startObject(true);
	dthWriter_->addMember("Name", name);
	dthWriter_->addMember("Label", label);
	dthWriter_->addMember("Value", value);
	dthWriter_->addMember("Data Type", type);
	dthWriter_->addMember("Texture", texture);
	dthWriter_->finishObject();

	if (!texture.isEmpty())
	{
		if (!discoveredTextures_.contains(texture))
		{
			discoveredTextures_.append(texture);
		}
	}
}

void DthWriter::writePropertyTexture(QString name, QString label, QString value, QString type, QString texture)
{
	texture = resolveTexturePath(texture);

	dthWriter_->startObject(true);
	dthWriter_->addMember("Name", name);
	dthWriter_->addMember("Label", label);
	dthWriter_->addMember("Value", value);
	dthWriter_->addMember("Data Type", type);
	dthWriter_->addMember("Texture", texture);
	dthWriter_->finishObject();

	if (!texture.isEmpty())
	{
		if (!discoveredTextures_.contains(texture))
		{
			discoveredTextures_.append(texture);
		}
	}
}

QString DthWriter::resolveTexturePath(const QString& texture)
{
	if (texture.isEmpty())
		return texture;

	auto cached = relocatedTexturePaths_.constFind(texture);
	if (cached != relocatedTexturePaths_.constEnd())
		return cached.value();

	QString resolved = DthStaticHelpers::relocateTempTexture(texture, exportDirectory_);
	relocatedTexturePaths_.insert(texture, resolved);
	return resolved;
}

void DthWriter::writeDiscoveredTextures()
{
	dthWriter_->startMemberArray("Discovered Textures", true);

	for (const QString& texture : discoveredTextures_)
	{
		dthWriter_->addItem(texture);
	}

	dthWriter_->finishArray();
}

void DthWriter::writeAllSubdivisions()
{
	std::map<std::string, int>* subdivisions = dazHelpers_.getSubdivisionLevelMap();

	dthWriter_->startMemberArray("Subdivisions", true);

	for (const auto& pair : *subdivisions)
	{
		std::string Name = pair.first;
		int targetValue = pair.second;
		writeSubdivisionProperties(QString::fromStdString(Name), targetValue);
	}

	dthWriter_->finishArray();
}

void DthWriter::writeSubdivisionProperties(const QString& name, int targetValue)
{
	dthWriter_->startObject(true);
	dthWriter_->addMember("Version", 1);
	dthWriter_->addMember("Asset Name", name);
	dthWriter_->addMember("Value", targetValue);
	dthWriter_->finishObject();
}

void DthWriter::writeJointOrientation(DzBoneList& boneList)
{
	dthWriter_->startMemberObject("JointOrientation", true);

	for (DzBone* bone : boneList)
	{
		QString boneName = bone->getName();
		QString rotationOrder = bone->getRotationOrder().toString();
		double xOrientation = bone->getOrientXControl()->getDoubleDefaultValue();
		double yOrientation = bone->getOrientYControl()->getDoubleDefaultValue();
		double zOrientation = bone->getOrientZControl()->getDoubleDefaultValue();
		DzQuat quaternionOrientation = bone->getOrientation(true);

		dthWriter_->startMemberArray(boneName, true);
		dthWriter_->addItem(rotationOrder);
		dthWriter_->addItem(xOrientation);
		dthWriter_->addItem(yOrientation);
		dthWriter_->addItem(zOrientation);
		dthWriter_->addItem(quaternionOrientation.m_w);
		dthWriter_->addItem(quaternionOrientation.m_x);
		dthWriter_->addItem(quaternionOrientation.m_y);
		dthWriter_->addItem(quaternionOrientation.m_z);
		dthWriter_->finishArray();
	}

	dthWriter_->finishObject();

	return;
}

void DthWriter::writeReferenceSkeletonFilePaths()
{
	dthWriter_->startMemberObject("Reference Skeletons", true);

	foreach(const QString & frame, referenceSkeletonFiles_.keys())
	{
		dthWriter_->addMember(frame, referenceSkeletonFiles_[frame]);
	}

	dthWriter_->finishObject();
}

void DthWriter::writeFbxGeoshells()
{
	dthWriter_->startMemberArray("Fbx Geoshells", true);

	for (const QString& geoshell : fbxGeoshells_)
	{
		dthWriter_->addItem(geoshell);
	}

	dthWriter_->finishArray();
}

void DthWriter::writeAlembicGeoshells()
{
	dthWriter_->startMemberArray("Alembic Geoshells", true);

	for (const QString& geoshell : alembicGeoshells_)
	{
		dthWriter_->addItem(geoshell);
	}

	dthWriter_->finishArray();
}

void DthWriter::writeAlembicGeografts()
{
	dthWriter_->startMemberArray("Alembic Geografts", true);

	for (const QString& geograft : alembicGeografts_)
	{
		dthWriter_->addItem(geograft);
	}

	dthWriter_->finishArray();
}

void DthWriter::writeRigidFollowers()
{
	dthWriter_->startMemberArray("Rigid Followers", true);

	foreach(const QString & follower, rigidFollowers_.keys())
	{
		dthWriter_->startObject(false);
		dthWriter_->addMember(follower, rigidFollowers_[follower]);
		dthWriter_->finishObject();
	}

	dthWriter_->finishArray();
}

void DthWriter::setFbxRomPath(QString path)
{
	fbxRomPath_ = path;
}

void DthWriter::setExperimentalRomPath(QString path)
{
	experimentalRomPath_ = path;
}

void DthWriter::setAlembicRomPath(QString path)
{
	alembicRomPath_ = path;
}

void DthWriter::addReferenceSkeletonPath(QString frame, QString path)
{
	referenceSkeletonFiles_.insert(frame, path);
}

void DthWriter::addFbxGeoshell(QString name)
{
	fbxGeoshells_.append(name);
}

void DthWriter::addRigidFollower(QString followerName, QString parentBoneName)
{
	rigidFollowers_.insert(followerName, parentBoneName);
}

void DthWriter::addAlembicGeoshell(QString name)
{
	alembicGeoshells_.append(name);
}

void DthWriter::addAlembicGeograft(QString name)
{
	alembicGeografts_.append(name);
}