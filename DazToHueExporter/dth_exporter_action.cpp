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

/**
	The script-engine boundary. Every Q_INVOKABLE below runs its export
	through here, and NO C++ exception may cross back out of it.

	Measured on DS4: six occurrences in this machine's log.txt between
	2026-08-18 and 2026-08-21. When the Alembic leg threw - a locked .abc, so
	`Could not create alembic archive` - the exception unwound through the
	script engine's invocation frame and killed the engine outright:

		dzscript.cpp(1192): Unhandled error while executing script.
		QScriptEngine::popContext() doesn't match with pushContext()

	The calling .dsa died at the C++ level, so its own try/catch never ran and
	neither did its restore path (renaming the .dthprev backups back). Catching
	here and re-raising as a SCRIPT error is what lets that catch run.
*/
void DazToHueExporterAction::runGuardedExport(const QString& entryPointName, const std::function<void()>& exportBody)
{
	QString failure;

	try
	{
		exportBody();
		return;
	}
	catch (const std::exception& e)
	{
		failure = QString::fromUtf8(e.what());
	}
	catch (...)
	{
		failure = "an unrecognised error";
	}

	// Note: a C++ catch does not contain a hard fault (an access violation
	// under /EHsc is not a C++ exception), so a failure that leaves NO trace
	// here is still possible - the exporter's own per-frame logging is what
	// locates that case.
	const QString message = QString("DazToHue Exporter: %1 failed - %2").arg(entryPointName).arg(failure);
	dzApp->log(message);

	if (!DthCompat::raiseScriptError(this, message))
	{
		// Nothing scripted is listening. Say so, rather than let a silent
		// return read as a successful export.
		dzApp->log(QString("DazToHue Exporter: the failure above could not be raised to a caller - check the export log and the absence of a .dth file"));
	}
}

void DazToHueExporterAction::doExport(QString exportDirectory, QString characterName, QString referenceFrames, bool saveSettings)
{
	runGuardedExport("doExport", [&]()
	{
		DthExporter dthExporter = DthExporter();
		dthExporter.doExport(exportDirectory, characterName, referenceFrames, saveSettings);
	});
}

void DazToHueExporterAction::doExportAnimation(QString exportDirectory, QString characterName, QString animationName, bool saveSettings)
{
	runGuardedExport("doExportAnimation", [&]()
	{
		DthExporter dthExporter = DthExporter();
		dthExporter.doExportAnimation(exportDirectory, characterName, animationName, saveSettings);
	});
}

void DazToHueExporterAction::doExportAlembicGroomPoses(QString exportDirectory, QString characterName, bool saveSettings)
{
	runGuardedExport("doExportAlembicGroomPoses", [&]()
	{
		DthExporter dthExporter = DthExporter();
		dthExporter.doExportAlembicGroomPoses(exportDirectory, characterName, saveSettings);
	});
}

#include "moc_dth_exporter_action.cpp"
