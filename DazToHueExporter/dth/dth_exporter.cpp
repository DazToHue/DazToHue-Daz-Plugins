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
	DazHelpers dazHelpers(selectedRootNode_, &dthLogger);
	DthWriter dthWriter(exportDirectory, characterName, selectedRootNode_, dazHelpers, exportProgress);

	dthLogger.log(LogLevel::DTHINFO, QString("**** doExport triggered ****"));

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