
#include <iostream>
#include <unordered_set>
#include <stdexcept>

#include <QMessageBox>

#include "daz_helpers.h"
#include "daz_static_helpers.h"
#include "../dth/dth_static_helpers.h"
#include "../dth/dth_writer.h"

#include "dzapp.h"
#include <dznode.h>
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzskeleton.h"
#include "dzobject.h"
#include "dzshape.h"
#include "dzmodifier.h"
#include "dzpresentation.h"
#include "dzassetmgr.h"
#include "dzproperty.h"
#include "dznumericnodeproperty.h"
#include <dzimageproperty.h>
#include "dzlayeredtexture.h"
#include "dzsettings.h"
#include "dzmorph.h"
#include "dzgeometry.h"
#include "dzenumproperty.h"
#include <dzfigure.h>
#include <dzbone.h>
#include <dzinstancenode.h>
#include "dzfacegroup.h"
#include "dzelement.h"
#include <dzpropertygroup.h>
#include "dzscript.h"
#include <rapidjson/document.h>

DazHelpers::DazHelpers(DzNode* selectedRootNode, DthLogger* dthLogger) : selectedRootNode_(selectedRootNode), dthLogger_(dthLogger)
{
}

DazHelpers::~DazHelpers()
{
}

void DazHelpers::preprocessScene()
{

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Gathering nodes"));

	candidateNodes_.clear();

	gatherCandidateNodes(selectedRootNode_);

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Found %1 candidate nodes").arg(QString::number(candidateNodes_.count())));

	for (DzNode* node : candidateNodes_)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Candidate node %1").arg(node->getName()));
	}

	processNodes();

	processMaterials();

	if (generateVisibilityMaps() == false)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Error processing visibilty maps"));
		return;
	}

	processSubdivisionLevels();

	// Detach hidden children (the workflow scripts hide hair items) BEFORE any
	// export runs: DzFbxExporter ignores visibility on fitted followers, so a
	// hidden hair item stays in the FBX unless it is unparented - the Alembic
	// is unaffected because its decoder checks isVisible itself. The
	// predecessor tree made this call at the end of processSelected(); it was
	// lost in the refactor (measured 2026-08-17: lash meshes present in the
	// exported FBX, absent from the Alembic). reparentHiddenNodes() restores
	// the children after the export.
	unparentHiddenNodes(selectedRootNode_);
}

void DazHelpers::gatherCandidateNodes(DzNode* rootNode)
{
	if (!rootNode->inherits("DzBone"))
	{
		candidateNodes_.append(rootNode);
	}

	for (int i = 0; i < rootNode->getNumNodeChildren(); ++i)
	{
		DzNode* childNode = rootNode->getNodeChild(i);

		gatherCandidateNodes(childNode);
	}
}

void DazHelpers::processNodes()
{
	if (dthLogger_ != nullptr)
	{
		dthLogger_->log(LogLevel::DTHINFO, QString("Processing node names"));
	}

	renamedNodes.clear();
	existingNodeNames_.clear();

	for (DzNode* node : candidateNodes_)
	{
		if (!node || node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : nullptr;

		if (shape || node->inherits("DzGroupNode") || node->inherits("DzInstanceNode"))
		{
			int nameIndex = 0;
			QString baseName = DthStaticHelpers::getCleanNodeName(node);
			QString nodeName = baseName;

			while (existingNodeNames_.contains(nodeName))
			{
				nodeName = baseName + QString("_%1").arg(++nameIndex);
			}

			if (nodeName != node->getName())
			{
				if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Processed node %1 -> %2").arg(node->getName(), nodeName));

				renamedNodes.insert(node, node->getName());

				node->setName(nodeName);
			}
			else
			{
				if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Processed node %1").arg(node->getName()));
			}

			existingNodeNames_.append(nodeName);
		}
	}
}

void DazHelpers::processMaterials()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Processing material names"));

	renamedMaterials.clear();
	existingMaterialNames_.clear();

	for (DzNode* node : candidateNodes_)
	{
		if (!node || node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : nullptr;

		if (shape)
		{
			if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Processing %1 materials for node %2").arg(QString::number(shape->getNumMaterials()), node->getLabel()));

			for (int i = 0; i < shape->getNumMaterials(); i++)
			{
				DzMaterial* material = shape->getMaterial(i);
				if (!material) continue;

				forceLieUpdate(material);

				if (node->className() != "DzGeometryShellNode")
				{
					int nameIndex = 0;
					QString baseName = material->getName();
					QString materialName = baseName;

					while (existingMaterialNames_.contains(materialName))
					{
						materialName = baseName + QString("_%1").arg(++nameIndex);
					}

					if (materialName != material->getName())
					{
						if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Material %1 -> %2").arg(material->getName(), materialName));

						renamedMaterials.insert(material, material->getName());

						material->setName(materialName);
					}
					else
					{
						if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Processed material %1").arg(material->getName()));
					}

					existingMaterialNames_.append(materialName);
				}
			}
		}
	}
}

void DazHelpers::undoChanges()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Undoing node name changes"));

	QMap<DzNode*, QString>::iterator nodeNameiterator;

	for (nodeNameiterator = renamedNodes.begin(); nodeNameiterator != renamedNodes.end(); ++nodeNameiterator)
	{
		nodeNameiterator.key()->setName(nodeNameiterator.value());
	}

	renamedNodes.clear();

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Undoing material name changes"));

	QMap<DzMaterial*, QString>::iterator materialNameiterator;

	for (materialNameiterator = renamedMaterials.begin(); materialNameiterator != renamedMaterials.end(); ++materialNameiterator)
	{
		materialNameiterator.key()->setName(materialNameiterator.value());
	}

	renamedNodes.clear();
}

void DazHelpers::unparentHiddenNodes(DzNode* rootNode)
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Unparenting hidden nodes"));

	unparentedNodes_.clear();

	for (int i = rootNode->getNumNodeChildren() - 1; i >= 0; --i)
	{
		DzNode* childNode = rootNode->getNodeChild(i);

		if (!childNode->isVisible())
		{
			if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Unparenting %1").arg(childNode->getLabel()));

			rootNode->removeNodeChild(childNode);

			unparentedNodes_.append(childNode);
		}
	}
}

void DazHelpers::reparentHiddenNodes()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Reparenting hidden nodes"));

	for (DzNode* childNode : unparentedNodes_)
	{
		if (childNode)
		{
			if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Reparenting %1").arg(childNode->getLabel()));

			selectedRootNode_->addNodeChild(childNode);
		}
	}
}

void DazHelpers::processSubdivisionLevels()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Processing subdivision levels"));

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DzNode* node = candidateNodes_[i];

		if (node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;

		if (shape || node->inherits("DzGroupNode") || node->inherits("DzInstanceNode"))
		{
			processSubdivisionLevel(shape);
		}
	}

	generateSubdivisionLevelMap();
}

void DazHelpers::processSubdivisionLevel(DzShape* shape)
{
	int currentLod = -1;
	int currentSubd = -1;

	getSubdivisionLevel(shape, currentLod, currentSubd);

	if ((currentLod == 1 && currentSubd == 0) || (currentLod == 0 && currentSubd != 0))
	{
		for (int index = 0; index < shape->getNumProperties(); index++)
		{
			DzProperty* property = shape->getProperty(index);
			DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
			DzEnumProperty* enumProperty = qobject_cast<DzEnumProperty*>(property);
			QString propName = property->getName();

			if (propName == "lodlevel" && enumProperty)
			{
				enumProperty->setValue(0);
			}

			if (propName == "SubDIALevel" && numericProperty)
			{
				numericProperty->setDoubleValue(0);
			}
		}
	}
	else
	{
		hasSubdivisions_ = true;
	}
}

void DazHelpers::getSubdivisionLevel(DzShape* shape, int& lod, int& subd)
{
	lod = -1;
	subd = -1;

	if (shape && shape->getNode()->isVisible())
	{
		for (int index = 0; index < shape->getNumProperties(); index++)
		{
			DzProperty* property = shape->getProperty(index);
			DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
			DzEnumProperty* enumProperty = qobject_cast<DzEnumProperty*>(property);
			QString propName = property->getName();

			if (propName == "lodlevel" && enumProperty)
			{
				lod = enumProperty->getValue();
			}

			if (propName == "SubDIALevel" && numericProperty)
			{
				subd = numericProperty->getDoubleValue();
			}
		}
	}
}

void DazHelpers::generateSubdivisionLevelMap()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Generating subdivision level map"));

	resolutionLevelMap_ = new std::map<std::string, int>();
	subdivisionLevelMap_ = new std::map<std::string, int>();

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DzNode* node = candidateNodes_[i];

		if (node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;

		if (shape)
		{
			std::string name = DthStaticHelpers::getFormattedShapeNameAsString(node);
			int currentLod = -1;
			int currentSubd = -1;
			getSubdivisionLevel(shape, currentLod, currentSubd);
			(*resolutionLevelMap_)[name] = currentLod;
			(*subdivisionLevelMap_)[name] = currentSubd;
		}
	}
}

void DazHelpers::setBaseSubdivisionLevels()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Setting base subdivision levels"));

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DzNode* node = candidateNodes_[i];

		if (node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;

		if (shape && !DazStaticHelpers::isGeograft(node) && !DazStaticHelpers::isGeoshell(node))
		{
			for (int index = 0; index < shape->getNumProperties(); index++)
			{
				DzProperty* property = shape->getProperty(index);
				DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
				DzEnumProperty* enumProperty = qobject_cast<DzEnumProperty*>(property);
				QString propName = property->getName();

				if (propName == "lodlevel" && enumProperty)
				{
					enumProperty->lock(false);
					enumProperty->setValue(0);
					enumProperty->lock(true);
				}

				if (propName == "SubDIALevel" && numericProperty)
				{
					numericProperty->lock(false);
					numericProperty->setDoubleValue(0);
					numericProperty->lock(true);
				}
			}
		}
	}
}

void DazHelpers::setUserDefinedSubdivisionLevels()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Setting user defined subdivision levels"));

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DzNode* node = candidateNodes_[i];

		if (node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;

		if (shape && !DazStaticHelpers::isGeograft(node) && !DazStaticHelpers::isGeoshell(node))
		{
			std::string name = DthStaticHelpers::getFormattedShapeNameAsString(node);
			int nodeResLevel = -1;
			int nodeSubDLevel = -1;

			try
			{
				nodeResLevel = resolutionLevelMap_->at(name);
				nodeSubDLevel = subdivisionLevelMap_->at(name);
				for (int index = 0; index < shape->getNumProperties(); index++)
				{
					DzProperty* property = shape->getProperty(index);
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					DzEnumProperty* enumProperty = qobject_cast<DzEnumProperty*>(property);
					QString propName = property->getName();

					if (propName == "lodlevel" && enumProperty)
					{
						enumProperty->lock(false);
						enumProperty->setValue(nodeResLevel);
						enumProperty->lock(true);
					}

					if (propName == "SubDIALevel" && numericProperty)
					{
						numericProperty->lock(false);
						numericProperty->setDoubleValue(nodeSubDLevel);
						numericProperty->lock(true);
					}
				}
			}
			catch (std::out_of_range)
			{
				nodeSubDLevel = -1;
				QMessageBox::information(0, "DazToHue Exporter", QString::fromStdString("Out of range"));
			}
		}
	}
}

void DazHelpers::lockSubdivisionLevels()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Locking subdivision levels"));

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DzNode* node = candidateNodes_[i];

		if (node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;

		if (shape && !DazStaticHelpers::isGeograft(node) && !DazStaticHelpers::isGeoshell(node))
		{
			for (int index = 0; index < shape->getNumProperties(); index++)
			{
				DzProperty* property = shape->getProperty(index);
				DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
				DzEnumProperty* enumProperty = qobject_cast<DzEnumProperty*>(property);
				QString propName = property->getName();

				if (propName == "lodlevel" && enumProperty)
				{
					enumProperty->lock(true);
				}

				if (propName == "SubDIALevel" && numericProperty)
				{
					numericProperty->lock(true);
				}
			}
		}
	}
}

void DazHelpers::unlockSubdivisionLevels()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Unlocking subdivision levels"));

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DzNode* node = candidateNodes_[i];

		if (node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;

		if (shape && !DazStaticHelpers::isGeograft(node) && !DazStaticHelpers::isGeoshell(node))
		{
			for (int index = 0; index < shape->getNumProperties(); index++)
			{
				DzProperty* property = shape->getProperty(index);
				DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
				DzEnumProperty* enumProperty = qobject_cast<DzEnumProperty*>(property);
				QString propName = property->getName();

				if (propName == "lodlevel" && enumProperty)
				{
					enumProperty->lock(false);
				}

				if (propName == "SubDIALevel" && numericProperty)
				{
					numericProperty->lock(false);
				}
			}
		}
	}
}

bool DazHelpers::hasSubdivisions()
{
	return hasSubdivisions_;
}

std::map<std::string, int>* DazHelpers::getSubdivisionLevelMap()
{
	return subdivisionLevelMap_;
}

void DazHelpers::enableInteractiveUpdates()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Enabling interactive updates"));

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DazStaticHelpers::enableInteractiveUpdates(candidateNodes_[i]);
	}
}

void DazHelpers::disableInteractiveUpdates()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Disabling interactive updates"));

	for (int i = 0; i < candidateNodes_.length(); i++)
	{
		DazStaticHelpers::disableInteractiveUpdates(candidateNodes_[i]);
	}
}

bool DazHelpers::generateVisibilityMaps()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Generating visibility maps"));

	const QString& qJSON = DazStaticHelpers::executeJsonScript(":/dth_scripts/visibility");

	const auto json = DthStaticHelpers::toStdString(qJSON);

	rapidjson::Document document;

	if (document.Parse(json.c_str()).HasParseError())
	{
		throw std::runtime_error("Couldn't parse geo visibility JSON");
		return false;
	}

	if (document["status"] == "failed")
	{
		QString message = "Anomalies were detected on some nodes in the scene.<br>If you continue with the export Daz Studio will most likely crash.<br>The anomalies were found on the following nodes:<ul>";
		const auto& problemNodeLabels = document["problemNodes"].GetArray();
		for (const auto& problemNodeLabel : problemNodeLabels)
		{
			const QString label = problemNodeLabel.GetString();
			message = message + "<li>" + label + "</li>";
		}

		message = message + "</ul>Do you want to proceed?";
		QMessageBox::StandardButton reply = QMessageBox::question(0, "DazToHue Exporter", message, QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::No)
		{
			return false;
		}
	}

	hiddenMaterials_.clear();
	hiddenFacegroups_.clear();
	hiddenFaceIds_.clear();

	const auto& objects = document["visibility"].GetArray();

	for (const auto& object : objects)
	{
		const QString label = object["label"].GetString();
		{
			const auto& hiddenMaterials = object["hiddenMaterials"];
			const auto& materials = hiddenMaterials.GetArray();

			for (const auto& material : materials)
			{
				const QString name = material.GetString();
				hiddenMaterials_[label].insert(name);
			}
		}
		{
			const auto& hiddenFacegroups = object["hiddenFacegroups"];
			const auto& facegroups = hiddenFacegroups.GetArray();

			for (const auto& facegroup : facegroups)
			{

				const QString name = facegroup.GetString();
				hiddenFacegroups_[label].insert(name);
			}
		}
		{
			const auto& hiddenFaces = object["hiddenFaces"];
			const auto& faces = hiddenFaces.GetArray();

			for (const auto& face : faces)
			{
				const int faceIndex = face.GetInt();
				hiddenFaceIds_[label].insert(faceIndex);
			}
		}
	}

	return true;
}

DzNode* DazHelpers::getNodeByLabel(const QString& label)
{
	DzNodeListIterator it = dzScene->nodeListIterator();

	while (it.hasNext())
	{
		DzNode* node = it.next();

		if (node->getLabel() == label) return node;
	}

	return nullptr;
}

bool DazHelpers::forceLieUpdate(DzMaterial* material)
{
	if (!material) return false;

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Forcing LIE update for material %1").arg(material->getName()));

	auto propertyListIterator = material->propertyListIterator();

	while (propertyListIterator.hasNext())
	{
		DzProperty* property = propertyListIterator.next();

		DzNumericProperty* pNumericProperty = qobject_cast<DzNumericProperty*>(property);
		DzImageProperty* pImageProperty = qobject_cast<DzImageProperty*>(property);

		DzTexture* pTexture = NULL;
		if (pNumericProperty)
		{
			pTexture = pNumericProperty->getMapValue();
		}
		else if (pImageProperty)
		{
			pTexture = pImageProperty->getValue();
		}

		DzLayeredTexture* pLayeredTexture = NULL;
		if (pTexture && pTexture->inherits("DzLayeredTexture"))
		{
			pLayeredTexture = qobject_cast<DzLayeredTexture*>(pTexture);
			if (pLayeredTexture)
			{
				pLayeredTexture->getPreviewPixmap(1, 1);
			}
		}
	}

	return true;
}

void DazHelpers::getFigures(DzNode* rootNode, QList<DzNode*>& figures)
{
	if (rootNode->isVisible())
	{
		if (DzFigure* Figure = qobject_cast<DzFigure*>(rootNode))
		{
			figures.append(rootNode);
		}
	}

	for (int childIndex = 0; childIndex < rootNode->getNumNodeChildren(); childIndex++)
	{
		DzNode* childNode = rootNode->getNodeChild(childIndex);
		getFigures(childNode, figures);
	}
}

DzPropertyList DazHelpers::getAllNodeProperties(DzNode* rootNode)
{
	DzPropertyList aPropertyList;
	DzPropertyGroupList aPropertyGroup_ToDoList;
	DzPropertyGroupTree* pGroupTree = rootNode->getPropertyGroups();
	if (pGroupTree)
	{
		DzPropertyGroup* propertyGroup = pGroupTree->getFirstChild();
		if (propertyGroup)
			aPropertyGroup_ToDoList.append(propertyGroup);
	}

	while (aPropertyGroup_ToDoList.isEmpty() == false)
	{
		DzPropertyGroup* propertyGroup = aPropertyGroup_ToDoList.takeFirst();
		if (propertyGroup)
		{
			DzPropertyListIterator propertyListIterator = propertyGroup->getProperties();
			while (propertyListIterator.hasNext())
			{
				DzProperty* pProperty = propertyListIterator.next();
				if (pProperty)
				{
					if (!aPropertyList.contains(pProperty))
						aPropertyList.append(pProperty);
				}
			}

			DzPropertyGroup* firstChild = nullptr;
			DzPropertyGroup* parentGroup = propertyGroup->getParent();
			if (parentGroup)
			{
				firstChild = parentGroup->getFirstChild();
			}
			else
			{
				firstChild = pGroupTree->getFirstChild();
			}
			if (firstChild == propertyGroup)
			{
				DzPropertyGroup* siblingGroup = propertyGroup->getNextSibling();
				while (siblingGroup)
				{
					if (!aPropertyGroup_ToDoList.contains(siblingGroup))
						aPropertyGroup_ToDoList.append(siblingGroup);
					siblingGroup = siblingGroup->getNextSibling();
				}
			}

			DzPropertyGroup* childGroup = propertyGroup->getFirstChild();
			while (childGroup)
			{
				if (!aPropertyGroup_ToDoList.contains(childGroup))
					aPropertyGroup_ToDoList.append(childGroup);
				childGroup = childGroup->getNextSibling();
			}
		}
	}

	return aPropertyList;
}

DazHelpers::GeograftNames DazHelpers::getGeograftNames()
{
	DazHelpers::GeograftNames geograftNames;

	DzNodeListIterator it = dzScene->nodeListIterator();

	while (it.hasNext())
	{
		DzNode* node = it.next();
		const auto figure = dynamic_cast<const DzFigure*>(node);

		if (figure)
		{
			const auto graftFigureCount = figure->getNumGraftFigures();

			for (std::remove_cv_t< decltype(graftFigureCount) > j = 0; j < graftFigureCount; j++)
			{
				const auto graftFigure = figure->getGraftFigure(j);
				const auto graftFigureLabel = graftFigure->getName();

				if (!geograftNames.contains(graftFigure->getName()))
				{
					geograftNames.insert(graftFigureLabel);
				}
			}
		}
	}

	return geograftNames;
}

bool DazHelpers::isMaterialHidden(QString nodeLabel, QString materialName)
{
	bool ishidden = false;

	if (hiddenMaterials_.contains(nodeLabel))
	{
		const auto& hiddenMaterials = hiddenMaterials_[nodeLabel];

		if (hiddenMaterials.contains(materialName))
		{
			ishidden = true;
		}
	}

	return ishidden;
}

DazHelpers::HiddenFaces DazHelpers::getHiddenFaces(const DzNode* node)
{
	DazHelpers::HiddenFaces hiddenFaces;

	if (hiddenFaceIds_.contains(node->getLabel()))
	{
		hiddenFaces = hiddenFaceIds_[node->getLabel()];
	}

	return hiddenFaces;
}

DzBoneList DazHelpers::getAllBones(DzNode* Node)
{
	DzBoneList aBoneList;

	if (Node == nullptr)
		return aBoneList;

	// Get skeleton and initial bone list
	DzSkeleton* pSkeleton = Node->getSkeleton();
	if (pSkeleton == nullptr)
		return aBoneList;
	// The out-parameter overload, NOT the returning one: getAllBones() returns
	// QObjectList on SDK4 and DzBoneList on SDK6, but this overload is
	// identical in both - so no compat shim is needed here.
	DzBoneList oBoneList;
	pSkeleton->getAllBones(oBoneList);

	// Create bone name lookup
	QMap<QString, bool> aBoneNameLookup;
	for (auto item : oBoneList)
	{
		DzBone* boneItem = qobject_cast<DzBone*>(item);
		if (boneItem)
		{
			QString sKey = boneItem->getName();
			aBoneNameLookup.insert(sKey, false);
			aBoneList.append(boneItem);
		}
	}

	// Add additional follower bones if any
	// 1. Walk through entire scene
	for (auto node : dzScene->getNodeList())
	{
		if (!node)
			continue;

		// 2. If inherits skeleton, Check to see if it follows pSkeleton
		DzSkeleton* skeletonNode = qobject_cast<DzSkeleton*>(node);
		if (node->inherits("DzSkeleton") && skeletonNode)
		{
			// 3. if 2, Compare bones to pSkeleton to see if it is not in pSkeleton
			DzSkeleton* followTarget = skeletonNode->getFollowTarget();
			if (followTarget == pSkeleton)
			{
				// 4. If 3, Add any bones that are not already in aBoneNameLookup
				for (auto oFollowerBone : skeletonNode->getAllBones())
				{
					DzBone* boneFollowerBone = qobject_cast<DzBone*>(oFollowerBone);
					if (boneFollowerBone)
					{
						QString sFollowerBoneName = boneFollowerBone->getName();
						if (!aBoneNameLookup.contains(sFollowerBoneName))
						{
							aBoneList.append(boneFollowerBone);
							aBoneNameLookup.insert(sFollowerBoneName, false);
						}
					}
				}
			}
		}
	}

	return aBoneList;
}