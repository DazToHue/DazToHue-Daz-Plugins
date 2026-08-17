#pragma once

#include "dzapp.h"
#include "dzobject.h"
#include "dzmodifier.h"

namespace DazStaticHelpers
{

	bool isRootNode(DzNode* pNode);
	bool isGeograft(DzNode* node);
	bool isGeoshell(DzNode* node);
	bool isRigidFollower(DzNode* node);
	QString getClosestRigidFollowerBoneName(DzNode* node);
	bool isValidFrame(int frameNumber);
	bool isStrandBasedHair(DzNode* node);
	void enableInteractiveUpdates(DzNode* node);
	void disableInteractiveUpdates(DzNode* node);
	QString getScriptSource(QString scriptPath);
	QString executeJsonScript(QString scriptPath);

}
