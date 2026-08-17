#include <memory>
#include <stdexcept>

#include "dth_exporter_action.h"
#include "gui/dth_exporter_gui.h"

#include <QMessageBox>

#include "dzapp.h"
#include "dzscene.h"
#include "dznode.h"
#include "dzmainwindow.h"

#include "dth/dth_exporter.h"

void entryPoint(DzMainWindow* mw)
{

	try
	{

		auto dlg = std::make_unique<DthExporterGui>(mw);
		dlg->exec();

	}
	catch (std::runtime_error& e)
	{

		QMessageBox::critical(mw, "Export not successful", e.what(), QMessageBox::Close);

	}
	catch (...)
	{

		QMessageBox::critical(mw, "Export not successful", "This is not ever supposed to happen, but it did. Sorry.", QMessageBox::Close);

	}

}

DazToHueExporterAction::DazToHueExporterAction() : DzAction(tr("DazToHue Exporter"), tr("DazToHue Exporter"))
{
	// Set the object name for the action to register it with the help and interactive lesson systems
	setObjectName(DazToHueExporterAction::metaObject()->className());

	// Setup action icon
	QImage iconImage(":/dth_images/icon");
	QPixmap basePixmap = QPixmap::fromImage(iconImage);
	QIcon icon;
	icon.addPixmap(basePixmap, QIcon::Normal, QIcon::Off);
	QAction::setIcon(icon);
}

void DazToHueExporterAction::executeAction()
{
	DzMainWindow* mw = dzApp->getInterface();

	if (!mw)
	{

		QMessageBox::warning(0, tr("Error"), tr("The main window has not been created yet."), QMessageBox::Ok);
		return;

	}

	if (dzScene->getPrimarySelection() == nullptr || dzScene->getPrimarySelection()->getNodeParent())
	{
		QMessageBox::information(0, "DazToHue Exporter", "Please select the root node in the scene");
		return;
	}

	entryPoint(mw);
}

void DazToHueExporterAction::doExport(QString exportDirectory, QString characterName, QString referenceFrames, bool saveSettings)
{
	DthExporter dthExporter = DthExporter();
	dthExporter.doExport(exportDirectory, characterName, referenceFrames, saveSettings);

}

void DazToHueExporterAction::doExportAnimation(QString exportDirectory, QString characterName, QString animationName, bool saveSettings)
{
	DthExporter dthExporter = DthExporter();
	dthExporter.doExportAnimation(exportDirectory, characterName, animationName, saveSettings);

}

void DazToHueExporterAction::doExportAlembicGroomPoses(QString exportDirectory, QString characterName, bool saveSettings)
{
	DthExporter dthExporter = DthExporter();
	dthExporter.doExportAlembicGroomPoses(exportDirectory, characterName, saveSettings);

}

#include "moc_dth_exporter_action.cpp"
