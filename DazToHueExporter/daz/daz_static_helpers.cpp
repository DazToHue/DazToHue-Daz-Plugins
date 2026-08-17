#include <stdexcept>

#include "daz_static_helpers.h"

#include <QMessageBox>
#include <QFileDialog>

#include "dzscene.h"
#include <dznode.h>
#include <dzfigure.h>
#include "dzpropertygroup.h"
#include "dzintproperty.h"
#include "dzscript.h"

#include "compat/dth_compat.h"

namespace DazStaticHelpers
{
	bool isRootNode(DzNode* pNode)
	{

		if (pNode->getNodeParent() == nullptr)
		{
			return true;
		}
		else
		{
			return false;
		}

	}

	bool isGeograft(DzNode* node)
	{
		if (node->inherits("DzFigure"))
		{
			const DzFigure* figure = dynamic_cast<const DzFigure*>(node);
			if (figure->isGraftFollowing())
			{
				DzSkeleton* target = figure->getFollowTarget();
				if (target && target->isVisible() && DthCompat::isVisibleInRender(target))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool isGeoshell(DzNode* node)
	{
		return node->className() == "DzGeometryShellNode";
	}

	bool isRigidFollower(DzNode* node)
	{
		return node->className() == "DzRigidFollowNode";
	}

	QString getClosestRigidFollowerBoneName(DzNode* node)
	{
		DzNode* currentNode = node;

		while (currentNode)
		{
			if (currentNode->className() == "DzBone")
			{
				return currentNode->getName();
			}
			currentNode = currentNode->getNodeParent();
		}

		return "";
	}

	bool isValidFrame(int frameNumber)
	{
		int startFrame = dzScene->getPlayRange().getStart() / dzScene->getTimeStep();
		int endFrame = dzScene->getPlayRange().getEnd() / dzScene->getTimeStep();

		if (frameNumber >= startFrame && frameNumber <= endFrame)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool isStrandBasedHair(DzNode* node)
	{
		const auto propertyGroupTree = node->getPropertyGroups();
		const auto lineTesselationGroup = propertyGroupTree->findChild("General/Line Tessellation");
		if (lineTesselationGroup)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	void enableInteractiveUpdates(DzNode* node)
	{
		if (node == nullptr) return;
		if (DzObject* Object = node->getObject())
		{
			for (int index = 0; index < Object->getNumModifiers(); index++)
			{
				DzModifier* modifier = Object->getModifier(index);
				if (modifier)
				{
					for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
					{
						DzProperty* property = modifier->getProperty(propindex);
						QString propName = property->getName();
						QString propLabel = property->getLabel();
						if (propName == "Interactive Update")
						{
							if (DzIntProperty* numericProperty = qobject_cast<DzIntProperty*>(property))
							{
								numericProperty->setValue(1);
								return;
							}
						}
					}
				}
			}
		}
	}

	void disableInteractiveUpdates(DzNode* node)
	{
		if (node == nullptr) return;
		if (DzObject* Object = node->getObject())
		{
			for (int index = 0; index < Object->getNumModifiers(); index++)
			{
				DzModifier* modifier = Object->getModifier(index);
				if (modifier)
				{
					for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
					{
						DzProperty* property = modifier->getProperty(propindex);
						QString propName = property->getName();
						QString propLabel = property->getLabel();
						if (propName == "Interactive Update")
						{
							if (DzIntProperty* numericProperty = qobject_cast<DzIntProperty*>(property))
							{
								numericProperty->setValue(0);
								return;
							}
						}
					}
				}
			}
		}
	}

	QString getScriptSource(QString scriptPath)
	{
		QString scriptSource;

		QFile file(scriptPath);
		if (file.exists())
		{
			if (!file.open(QIODevice::ReadOnly))
			{
				throw std::runtime_error("Failed to open script source");
			}

			QByteArray fileContentBytes = file.readAll();

			file.close();

			scriptSource = fileContentBytes;

			return scriptSource;
		}
		else
		{
			throw std::runtime_error("Failed to open script source");
		}

		return scriptSource;
	}

	QString executeJsonScript(QString scriptPath)
	{

		const QString scriptSource = DazStaticHelpers::getScriptSource(scriptPath);

		// The script runs the same way on both generations; how it is invoked
		// differs, and that difference lives in compat/dth_compat.h.
		QString json;
		if (!DthCompat::runJsonScript(scriptSource, json))
		{
			throw std::runtime_error("Could not execute script");
		}

		return json;

	}

}