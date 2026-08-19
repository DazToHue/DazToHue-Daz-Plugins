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

	// Initialise logger, helpers and writer
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

	DazHelpers dazHelpers(selectedRootNode_, &dthLogger);
	DthWriter dthWriter(exportDirectory, characterName, selectedRootNode_, dazHelpers, exportProgress);

	// Initialise exporters
	dthLogger.log(LogLevel::DTHINFO, QString("Initialising FBX exporter"));
	DthFbxExporter fbxExporter(exportDirectory, characterName, selectedRootNode_, dazHelpers, &dthWriter, exportProgress, &dthLogger);
	Sagan::HoudiniAlembicOutputTransformer houdiniAlembicOutputTransformer;
	dthLogger.log(LogLevel::DTHINFO, QString("Initialising Alembic exporter"));
	DthAlembicExporter alembicExporter(exportDirectory, characterName, selectedRootNode_, dazHelpers, &dthWriter, exportProgress, &houdiniAlembicOutputTransformer, &dthLogger);

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

	// Write DTH file
	dthLogger.log(LogLevel::DTHINFO, QString("Writing DTH file"));
	exportProgress.setCurrentInfo("Writing DTH file");
	dthWriter.writeFile();
	exportProgress.step();

	dazHelpers.undoChanges();
	dazHelpers.reparentHiddenNodes();
	dazHelpers.unlockSubdivisionLevels();

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

	// Same zero-influence guarantee as doExport: a leftover animation FBX
	// must not survive into (or shape) this run's output.
	DthStaticHelpers::removeStaleAnimationExport(exportDirectory, characterName, animationName);

	// Initialise progress display
	DzProgress exportProgress = DzProgress("", 6, false, true);
	exportProgress.setCloseOnFinish(false);

	// Initialise tools
	DazHelpers dazTools(selectedRootNode_, nullptr);

	// Initialise exporters
	DthFbxExporter fbxExporter(exportDirectory, characterName, selectedRootNode_, dazTools, nullptr, exportProgress, nullptr);

	// Export fbx animation
	fbxExporter.exportAnimationOnly(animationName);
	exportProgress.step();

	exportProgress.finish();

	// Save Settings
	if (saveSettings)
	{
		this->saveSettings(exportDirectory, characterName);
	}
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

	// Same zero-influence guarantee as doExport, for this entry point's own
	// output file.
	DthStaticHelpers::removeStaleGroomExport(exportDirectory, characterName);

	// Initialise progress display
	DzProgress exportProgress = DzProgress("", 4, false, true);
	exportProgress.setCloseOnFinish(false);

	// Initialise helpers
	DazHelpers dazHelpers(selectedRootNode_, nullptr);

	// Initialise exporters
	Sagan::HoudiniAlembicOutputTransformer houdiniAlembicOutputTransformer;
	DthAlembicExporter alembicExporter(exportDirectory, characterName, selectedRootNode_, dazHelpers, nullptr, exportProgress, &houdiniAlembicOutputTransformer, nullptr);

	// Pre-process scene
	dazHelpers.preprocessScene();
	exportProgress.step();

	// Export alembic groom poses
	alembicExporter.doGroomPosesExport();

	exportProgress.step();
	exportProgress.finish();

	// Save Settings
	if (saveSettings)
	{
		this->saveSettings(exportDirectory, characterName);
	}
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