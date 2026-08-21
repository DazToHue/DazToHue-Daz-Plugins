#include <stdexcept>
#include <memory>

#include <QMessageBox>
#include <QFileDialog>

#include "dzapp.h"
#include "dzelement.h"
#include <dzprogress.h>

#include "dth_exporter.h"
#include "dth/dth_settings.h"
#include "dth/dth_writer.h"
#include "dth/dth_logger.h"
#include "dth/dth_static_helpers.h"
#include "daz/daz_helpers.h"
#include "daz/daz_static_helpers.h"
#include "fbx/fbx_exporter.h"
#include "alembic/sagan/output_transformer/houdini_alembic_output_transformer.h"
#include "alembic/alembic_exporter.h"

namespace
{
	/**
		Undo everything preprocessScene() did to the user's scene.

		This runs on the FAILURE path as well as the success path: preparation
		unparents hidden nodes and locks SubD properties, and an export that
		threw halfway through would otherwise hand the scene back mangled.
		Never throws - it is called from catch blocks.
	*/
	void restoreSceneState(DazHelpers& dazHelpers, DthLogger& dthLogger)
	{
		try
		{
			dazHelpers.undoChanges();
			dazHelpers.reparentHiddenNodes();
			dazHelpers.unlockSubdivisionLevels();
		}
		catch (...)
		{
			dthLogger.log(LogLevel::DTHERROR, QString("Could not fully restore the scene after a failed export - undo manually before exporting again"));
		}
	}

	/**
		Abort before anything is touched if the export cannot possibly land.

		A stale output file that could not be removed is held open by another
		application (a Houdini session on the previous .abc is the measured
		cause), and every writer that follows would fail on it anyway - the
		Alembic archive first. Failing here means no scene mutation, no file
		handle, and no half-written corpse to clean up.
	*/
	void abortIfOutputLocked(const QStringList& lockedPaths, DthLogger& dthLogger)
	{
		if (lockedPaths.isEmpty()) return;

		const QString message = QString("Cannot export - %1 is locked by another application (a Houdini session holding the previous export open is the usual cause). Close it and export again.").arg(lockedPaths.join(", "));
		dthLogger.log(LogLevel::DTHERROR, message);

		throw std::runtime_error(message.toUtf8().constData());
	}
}

DthExporter::DthExporter()
{
	settings_ = dthSettings_.getSettings();
}

DthExporter::~DthExporter()
{
}

void DthExporter::doExport(QString exportDirectory, QString characterName, QString referenceFrames, bool saveSettings)
{
	selectedRootNode_ = dzScene->getPrimarySelection();

	if (!readyToExport(exportDirectory, characterName, "", referenceFrames))
	{
		return;
	}

	// Create the export directory if it doesn't exist
	DthStaticHelpers::createDirectory(exportDirectory);

	// Initialise progress display
	// TODO - Figure out accurate number of progress steps
	int referenceFrameCount = DthStaticHelpers::countNumberString(referenceFrames.toStdString());
	DzProgress exportProgress = DzProgress("", 7 + referenceFrameCount, false, true);
	exportProgress.setCloseOnFinish(false);

	// Initialise logger
	DthLogger dthLogger(exportDirectory, characterName, selectedRootNode_);

	dthLogger.log(LogLevel::DTHINFO, QString("**** doExport triggered ****"));

	// Delete this set's previous output before anything opens a file: a
	// pre-existing set must have zero influence on the export content
	// (dth-character-studio measured 2.0.2 rewriting one as a static ROM and
	// still deletes the set itself before every doExport - 2.1.1 owns that
	// guarantee, so the workaround can retire on >= 2.1.1). This runs before
	// DthWriter's constructor takes the .dth file handle.
	QStringList staleRemovalFailures;
	const int staleRemoved = DthStaticHelpers::removeStaleRomExportSet(exportDirectory, characterName, &staleRemovalFailures);
	if (staleRemoved > 0) dthLogger.log(LogLevel::DTHINFO, QString("Removed %1 stale output file(s) from a previous export").arg(staleRemoved));
	for (const QString& failedPath : staleRemovalFailures)
	{
		dthLogger.log(LogLevel::DTHWARNGING, QString("Could not remove stale output file %1 - it may be locked by another application").arg(failedPath));
	}
	abortIfOutputLocked(staleRemovalFailures, dthLogger);

	// Initialise helpers and writer
	DazHelpers dazHelpers(selectedRootNode_, &dthLogger);
	DthWriter dthWriter(exportDirectory, characterName, selectedRootNode_, dazHelpers, exportProgress);

	// Initialise exporters
	dthLogger.log(LogLevel::DTHINFO, QString("Initialising FBX exporter"));
	DthFbxExporter fbxExporter(exportDirectory, characterName, selectedRootNode_, dazHelpers, &dthWriter, exportProgress, &dthLogger);
	Sagan::HoudiniAlembicOutputTransformer houdiniAlembicOutputTransformer;
	dthLogger.log(LogLevel::DTHINFO, QString("Initialising Alembic exporter"));
	DthAlembicExporter alembicExporter(exportDirectory, characterName, selectedRootNode_, dazHelpers, &dthWriter, exportProgress, &houdiniAlembicOutputTransformer, &dthLogger);

	try
	{
		// Pre-process scene
		exportProgress.setCurrentInfo("Preprocessing scene");
		dazHelpers.preprocessScene();
		exportProgress.step();

		// Export alembic ROM
		dazHelpers.enableInteractiveUpdates();
		alembicExporter.doRomExport();
		dazHelpers.disableInteractiveUpdates();
		exportProgress.step();

		// Export fbx ROM files
		fbxExporter.exportRoms();
		exportProgress.step();
		fbxExporter.exportExperimentalRomAnimation();
		exportProgress.step();

		// Export reference frames
		fbxExporter.exportSkeletonReferenceFrames(referenceFrames);
		exportProgress.step();

		// Write DTH file LAST: it is the manifest that says the export landed,
		// and it is only created once everything it points at exists.
		dthLogger.log(LogLevel::DTHINFO, QString("Writing DTH file"));
		exportProgress.setCurrentInfo("Writing DTH file");
		dthWriter.writeFile();
		exportProgress.step();
	}
	catch (const std::exception& e)
	{
		dthLogger.log(LogLevel::DTHERROR, QString("doExport failed - %1").arg(QString::fromUtf8(e.what())));
		restoreSceneState(dazHelpers, dthLogger);
		exportProgress.finish();
		throw;
	}
	catch (...)
	{
		dthLogger.log(LogLevel::DTHERROR, QString("doExport failed with an unrecognised error"));
		restoreSceneState(dazHelpers, dthLogger);
		exportProgress.finish();
		throw std::runtime_error("doExport failed with an unrecognised error");
	}

	restoreSceneState(dazHelpers, dthLogger);

	exportProgress.finish();

	// Save Settings
	if (saveSettings)
	{
		this->saveSettings(exportDirectory, characterName);
	}

	dthLogger.log(LogLevel::DTHINFO, QString("**** doExport finished ****"));
}

void DthExporter::doExportAnimation(QString exportDirectory, QString characterName, QString animationName, bool saveSettings)
{
	selectedRootNode_ = dzScene->getPrimarySelection();

	if (!readyToExport(exportDirectory, characterName, animationName, "", true))
	{
		return;
	}

	// Create the export directory if it doesn't exist
	DthStaticHelpers::createDirectory(exportDirectory);

	// Initialise logger
	DthLogger dthLogger(exportDirectory, characterName, selectedRootNode_);

	dthLogger.log(LogLevel::DTHINFO, QString("**** doExportAnimation triggered ****"));

	// Same zero-influence guarantee as doExport: a leftover animation FBX
	// must not survive into (or shape) this run's output - and a locked one
	// means this run cannot land, so stop before anything else is touched.
	QStringList staleRemovalFailures;
	DthStaticHelpers::removeStaleAnimationExport(exportDirectory, characterName, animationName, &staleRemovalFailures);
	abortIfOutputLocked(staleRemovalFailures, dthLogger);

	// Initialise progress display
	DzProgress exportProgress = DzProgress("", 6, false, true);
	exportProgress.setCloseOnFinish(false);

	// Initialise tools
	DazHelpers dazTools(selectedRootNode_, nullptr);

	// Initialise exporters
	DthFbxExporter fbxExporter(exportDirectory, characterName, selectedRootNode_, dazTools, nullptr, exportProgress, &dthLogger);

	// Export fbx animation. Nothing here preprocesses the scene, so there is
	// no restore counterpart to run - but the failure still belongs in this
	// export's own log, next to the run that produced it.
	try
	{
		fbxExporter.exportAnimationOnly(animationName);
		exportProgress.step();
	}
	catch (const std::exception& e)
	{
		dthLogger.log(LogLevel::DTHERROR, QString("doExportAnimation failed - %1").arg(QString::fromUtf8(e.what())));
		exportProgress.finish();
		throw;
	}
	catch (...)
	{
		dthLogger.log(LogLevel::DTHERROR, QString("doExportAnimation failed with an unrecognised error"));
		exportProgress.finish();
		throw std::runtime_error("doExportAnimation failed with an unrecognised error");
	}

	exportProgress.finish();

	// Save Settings
	if (saveSettings)
	{
		this->saveSettings(exportDirectory, characterName);
	}

	dthLogger.log(LogLevel::DTHINFO, QString("**** doExportAnimation finished ****"));
}

void DthExporter::doExportAlembicGroomPoses(QString exportDirectory, QString characterName, bool saveSettings)
{
	selectedRootNode_ = dzScene->getPrimarySelection();

	if (!readyToExport(exportDirectory, characterName, "", "", false))
	{
		return;
	}

	// Create the export directory if it doesn't exist
	DthStaticHelpers::createDirectory(exportDirectory);

	// Initialise logger
	DthLogger dthLogger(exportDirectory, characterName, selectedRootNode_);

	dthLogger.log(LogLevel::DTHINFO, QString("**** doExportAlembicGroomPoses triggered ****"));

	// Same zero-influence guarantee as doExport, for this entry point's own
	// output file - and the same abort-before-touching-anything if it is locked.
	QStringList staleRemovalFailures;
	DthStaticHelpers::removeStaleGroomExport(exportDirectory, characterName, &staleRemovalFailures);
	abortIfOutputLocked(staleRemovalFailures, dthLogger);

	// Initialise progress display
	DzProgress exportProgress = DzProgress("", 4, false, true);
	exportProgress.setCloseOnFinish(false);

	// Initialise helpers
	DazHelpers dazHelpers(selectedRootNode_, nullptr);

	// Initialise exporters
	Sagan::HoudiniAlembicOutputTransformer houdiniAlembicOutputTransformer;
	DthAlembicExporter alembicExporter(exportDirectory, characterName, selectedRootNode_, dazHelpers, nullptr, exportProgress, &houdiniAlembicOutputTransformer, &dthLogger);

	try
	{
		// Pre-process scene
		dazHelpers.preprocessScene();
		exportProgress.step();

		// Export alembic groom poses
		alembicExporter.doGroomPosesExport();

		exportProgress.step();
	}
	catch (const std::exception& e)
	{
		dthLogger.log(LogLevel::DTHERROR, QString("doExportAlembicGroomPoses failed - %1").arg(QString::fromUtf8(e.what())));
		restoreSceneState(dazHelpers, dthLogger);
		exportProgress.finish();
		throw;
	}
	catch (...)
	{
		dthLogger.log(LogLevel::DTHERROR, QString("doExportAlembicGroomPoses failed with an unrecognised error"));
		restoreSceneState(dazHelpers, dthLogger);
		exportProgress.finish();
		throw std::runtime_error("doExportAlembicGroomPoses failed with an unrecognised error");
	}

	restoreSceneState(dazHelpers, dthLogger);

	exportProgress.finish();

	// Save Settings
	if (saveSettings)
	{
		this->saveSettings(exportDirectory, characterName);
	}

	dthLogger.log(LogLevel::DTHINFO, QString("**** doExportAlembicGroomPoses finished ****"));
}

bool DthExporter::readyToExport(QString exportDirectory, QString characterName, QString animationName, QString referenceFrames, bool exportingAnimationOnly)
{
	if (selectedRootNode_ == nullptr || selectedRootNode_->getNodeParent())
	{
		QMessageBox::information(0, "DazToHue Exporter", "Please select the root node in the scene");
		return false;
	}

	if (!selectedRootNode_->isVisible())
	{
		QMessageBox::information(0, "DazToHue Exporter", "The root node must be visible");
		return false;
	}

	if (exportDirectory == "")
	{
		QMessageBox::information(0, "DazToHue Exporter", "Please select an export diectory");
		return false;
	}

	if (characterName == "")
	{
		QMessageBox::information(0, "DazToHue Exporter", "Please enter a name for the character");
		return false;
	}

	if (exportingAnimationOnly && animationName == "")
	{
		QMessageBox::information(0, "DazToHue Exporter", "Please enter a name for the animation");
		return false;
	}

	return true;
}

void DthExporter::saveSettings(QString exportDirectory, QString characterName)
{
	// Save Settings
	settings_.lastExportPath = exportDirectory;
	settings_.lastCharacterName = characterName;
	dthSettings_.saveSettings(settings_);
}

#include "moc_dth_exporter.cpp"