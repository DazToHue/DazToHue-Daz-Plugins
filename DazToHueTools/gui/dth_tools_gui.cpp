#include <stdexcept>
#include <memory>

#include <QMessageBox>
#include <QtCore/QString>
#include <QtGui/QDesktopServices>

#include "gui/dth_tools_gui.h"

#include "dzapp.h"
#include "dzscene.h"
#include "dznode.h"
#include "dzobject.h"
#include "dzshape.h"
#include "dzbone.h"
#include "dzproperty.h"
#include "dzintproperty.h"
#include "dzfloatproperty.h"

DazToHueToolsGui::DazToHueToolsGui(QWidget* parent) : QWidget(parent)
{

	setupUi(this);

	// No item delegates needed anymore! 
	twNodeVisibility->setColumnCount(1);
	twNodeVisibility->setHeaderLabels(QStringList() << "Scene Hierarchy Nodes");

	// The string-based SIGNAL/SLOT form, NOT Qt5's pointer-to-member syntax:
	// the latter does not exist in Qt 4.8, and it additionally cannot name
	// these two signals at all because both are declared protected
	// (QTreeWidget::itemChanged, DzScene::primarySelectionChanged). The macro
	// form works unchanged on Qt4 and Qt6.
	connect(twNodeVisibility, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
		this, SLOT(handleVisibilityToggleChanged(QTreeWidgetItem*, int)));

	if (dzScene)
	{
		connect(dzScene, SIGNAL(primarySelectionChanged(DzNode*)),
			this, SLOT(handleDazSelectionChanged(DzNode*)));
	}

	populateSampleNodes();

}

void DazToHueToolsGui::populateSampleNodes()
{
	// Safety check: ensure Daz global scene environment is active
	if (!dzScene) return;

	twNodeVisibility->blockSignals(true);
	twNodeVisibility->clear();

	// 1. Get whatever the user clicked on in the Daz UI
	DzNode* selectedNode = dzScene->getPrimarySelection();
	if (!selectedNode)
	{
		twNodeVisibility->blockSignals(false);
		return; // Empty scene or nothing selected
	}

	// 2. Trace all the way up to find the absolute root node parent
	DzNode* rootNode = selectedNode;
	while (rootNode->getNodeParent() != nullptr)
	{
		rootNode = rootNode->getNodeParent();
	}

	// 3. Skip root asset completely if it happens to be a bone
	if (qobject_cast<DzBone*>(rootNode))
	{
		twNodeVisibility->blockSignals(false);
		return;
	}

	// 4. Create the main parent tree item
	QTreeWidgetItem* rootItem = new QTreeWidgetItem(twNodeVisibility);
	rootItem->setText(0, rootNode->getLabel());
	rootItem->setFlags(rootItem->flags() | Qt::ItemIsUserCheckable);

	// Sync UI checkstate directly to real Daz asset visibility status
	rootItem->setCheckState(0, rootNode->isVisible() ? Qt::Checked : Qt::Unchecked);

	// Store the raw pointer inside Qt data fields so we can track it down on toggle click
	rootItem->setData(0, static_cast<int>(Qt::UserRole), QVariant::fromValue(static_cast<void*>(rootNode)));

	// 5. Recursively process child structures (skeletons, geometry components, etc.)
	addNonBoneChildrenRecursively(rootNode, rootItem);

	twNodeVisibility->expandAll();
	twNodeVisibility->blockSignals(false);
}

void DazToHueToolsGui::addNonBoneChildrenRecursively(DzNode* dazParentNode, QTreeWidgetItem* treeParentItem)
{
	int childCount = dazParentNode->getNumNodeChildren();
	for (int i = 0; i < childCount; ++i)
	{
		DzNode* childNode = dazParentNode->getNodeChild(i);
		if (!childNode) continue;

		// 💡 THE FIX: Check if this node holds actual geometric mesh objects 
		// (This is true for Eyelashes, Eyes, Hair Caps, and Props, but false for standard skeleton bones)
		bool hasGeometry = (childNode->getObject() != nullptr);

		if (!hasGeometry)
		{
			// It's a plain structural bone (like Hip, Spine, LeftCollar).
			// Skip making a row for it, but dig deeper to find meshes parented under it.
			addNonBoneChildrenRecursively(childNode, treeParentItem);
		}
		else
		{
			// It has mesh data! Create a valid row in our toggle tree.
			QTreeWidgetItem* childItem = new QTreeWidgetItem(treeParentItem);
			childItem->setText(0, childNode->getLabel());
			childItem->setFlags(childItem->flags() | Qt::ItemIsUserCheckable);
			childItem->setCheckState(0, childNode->isVisible() ? Qt::Checked : Qt::Unchecked);

			// Save the DzNode pointer into user data
			childItem->setData(0, static_cast<int>(Qt::UserRole), QVariant::fromValue(static_cast<void*>(childNode)));

			// Continue searching down from this mesh node for any sub-attachments
			addNonBoneChildrenRecursively(childNode, childItem);
		}
	}
}

void DazToHueToolsGui::handleVisibilityToggleChanged(QTreeWidgetItem* item, int column)
{
	if (column != 0 || !item || !dzScene) return;

	// Retrieve the raw DzNode pointer we buried inside Qt::UserRole earlier
	void* rawNodePtr = item->data(column, static_cast<int>(Qt::UserRole)).value<void*>();
	if (!rawNodePtr) return;

	DzNode* targetNode = static_cast<DzNode*>(rawNodePtr);
	bool shouldBeVisible = (item->checkState(column) == Qt::Checked);

	// Update Daz Core context seamlessly if state changed
	if (targetNode && targetNode->isVisible() != shouldBeVisible)
	{
		targetNode->setVisible(shouldBeVisible);

		// Forces Daz 3D Viewport engine to redraw its buffers instantly
		//dzScene->statusChanged();
	}
}

void DazToHueToolsGui::handleDazSelectionChanged(DzNode* newSelectedNode)
{
	// If the user clears their selection entirely, ignore it or call clear()
	if (!newSelectedNode) return;

	// 💡 Performance Guard: 
	// Check if the freshly clicked node is already part of the tree.
	// If it is, do not clear and rebuild the entire widget.
	for (int i = 0; i < twNodeVisibility->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* topItem = twNodeVisibility->topLevelItem(i);
		void* rawNodePtr = topItem->data(0, static_cast<int>(Qt::UserRole)).value<void*>();

		if (rawNodePtr)
		{
			DzNode* rootInTree = static_cast<DzNode*>(rawNodePtr);

			// Find out if the newly selected node belongs to our active root tree path
			DzNode* traceRoot = newSelectedNode;
			while (traceRoot->getNodeParent() != nullptr)
			{
				traceRoot = traceRoot->getNodeParent();
			}

			// If the root match is identical, the item is already mapped in our UI view
			if (rootInTree == traceRoot)
			{
				return;
			}
		}
	}

	// If it belongs to a completely different asset figure context, rebuild the layout panel
	populateSampleNodes();
}

#include "moc_dth_tools_gui.cpp"