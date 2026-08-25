#pragma once

#include <tuple>
#include <array>
#include <optional>
#include <vector>
#include <map>
#include <unordered_set>
#include <iostream>

#include <QtCore/QSet>
#include <QtCore/QVector>
#include <QtCore/QString>

#include "dzapp.h"
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzaction.h"
#include "dzskeleton.h"
#include "dzobject.h"
#include "dzshape.h"
#include "dzmodifier.h"
#include "dzpresentation.h"
#include "dzassetmgr.h"
#include "dzproperty.h"
#include "dznumericnodeproperty.h"
#include "dzsettings.h"
#include "dzmorph.h"
#include "dzgeometry.h"
#include "dzenumproperty.h"
#include <dzprogress.h>
#include <dzfigure.h>

#include "../dth/dth_logger.h"

class DthWriter;

class DazHelpers
{
	using HiddenMaterials = QMap<QString, QSet<QString>>;
	using HiddenFacegroups = QMap<QString, QSet<QString>>;
	using HiddenFaceIds = QMap<QString, QSet<int>>;
	using HiddenFaces = QSet<int>;
	using GeograftNames = QSet<QString>;

public:

	DazHelpers(DzNode* selectedRootNode, DthLogger* dthLogger);
	~DazHelpers();

	void preprocessScene();
	void gatherCandidateNodes(DzNode* rootNode);
	void processNodes();
	void processMaterials();
	void unparentHiddenNodes(DzNode* rootNode);
	void reparentHiddenNodes();
	void processSubdivisionLevels();
	void processSubdivisionLevel(DzShape* shape);
	void generateSubdivisionLevelMap();
	void getSubdivisionLevel(DzShape* shape, int& lod, int& subd);
	void setBaseSubdivisionLevels();
	void setUserDefinedSubdivisionLevels();
	void lockSubdivisionLevels();
	void unlockSubdivisionLevels();
	bool hasSubdivisions();
	void enableInteractiveUpdates();
	void disableInteractiveUpdates();
	bool generateVisibilityMaps();
	void undoChanges();
	std::map<std::string, int>* getSubdivisionLevelMap();
	DzBoneList getAllBones(DzNode* rootNode);
	DzNode* getNodeByLabel(const QString& label);
	bool forceLieUpdate(DzMaterial* pMaterial);
	void getFigures(DzNode* rootNode, QList<DzNode*>& figures);
	DzPropertyList getAllNodeProperties(DzNode* rootNode);
	GeograftNames getGeograftNames();
	bool isMaterialHidden(QString nodeLabel, QString materialName);
	HiddenFaces getHiddenFaces(const DzNode* geoShellNode);

private:

	DzNode* selectedRootNode_;
	DthLogger* dthLogger_ = nullptr;
	QList<DzNode*> candidateNodes_;
	QList<DzNode*> unparentedNodes_;
	QMap<DzNode*, QString> renamedNodes;
	QMap<DzMaterial*, QString> renamedMaterials;
	QStringList existingNodeNames_;
	QStringList existingMaterialNames_;
	std::map<std::string, int>* resolutionLevelMap_ = nullptr;
	std::map<std::string, int>* subdivisionLevelMap_ = nullptr;
	bool hasSubdivisions_ = false;
	HiddenMaterials hiddenMaterials_;
	HiddenFacegroups hiddenFacegroups_;
	HiddenFaceIds hiddenFaceIds_;

};
