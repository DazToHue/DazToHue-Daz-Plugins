#include <unordered_set>
#include <stdexcept>

#include "../daz/daz_helpers.h"
#include "../dth/dth_static_helpers.h"

#include "alembic_exporter.h"
#include "sagan/decoder/alembic_node_decoder.h"

DthAlembicExporter::DthAlembicExporter(QString exportDirectory, QString characterName, DzNode* selectedRootNode, DazHelpers& dazHelpers, DthWriter* dthWriter, DzProgress& exportProgress, const Sagan::OutputTransformer* outputTransformer, DthLogger* dthLogger) : exportDirectory_(exportDirectory), characterName_(characterName), selectedRootNode_(selectedRootNode), dazHelpers_(dazHelpers), dthWriter_(dthWriter), exportProgress_(exportProgress), SaganExporter(outputTransformer), dthLogger_(dthLogger)
{
}

DthAlembicExporter::~DthAlembicExporter()
{
}

void DthAlembicExporter::doRomExport()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting Alembic ROM"));

	exportProgress_.setCurrentInfo("Exporting alembic ROM");

	alembicExportPath_ = exportDirectory_ + "/" + characterName_ + ".abc";

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Creating Alembic archive"));

	try
	{
		m_Archive = std::make_unique<Alembic::Abc::OArchive >(Alembic::AbcCoreOgawa::WriteArchive(), alembicExportPath_.toLatin1().constData());
		m_TimeSampling = std::make_shared< Alembic::Abc::TimeSampling>((double)dzScene->getTimeStep() / 4800, 0.0);
	}
	catch (...)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHERROR, QString("Could not create alembic archive"));
		throw std::runtime_error("Could not create alembic archive. Make sure the Alembic file is not locked by another application such as Houdini.");
	}

	// Decode nodes
	Sagan::AlembicNodeDecoder alembicNodeDecoder(this, dazHelpers_, dthWriter_);
	alembicNodeDecoder.setShapeNameFormatter(DthStaticHelpers::getFormattedShapeNameAsString);
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Decoding nodes"));
	alembicNodeDecoder.decodeSelected(selectedRootNode_);

	int startFrame = dzScene->getPlayRange().getStart() / dzScene->getTimeStep();
	int endFrame = dzScene->getPlayRange().getEnd() / dzScene->getTimeStep();

	// Initialise progress display
	DzProgress alembicProgress = DzProgress("", endFrame, false, true);
	alembicProgress.setCloseOnFinish(false);

	// Export frames
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting alembic frames"));

	for (int currentFrame = startFrame; currentFrame <= endFrame; currentFrame++)
	{
		alembicProgress.setCurrentInfo("Exporting alembic frame " + QString::number(currentFrame));

		dzScene->setFrame(currentFrame);

		QApplication::processEvents();

		// DS6 defers evaluation past setFrame(); without this every frame reads
		// a stale mesh (see updateGeometryCaches). The groom-poses loop below
		// deliberately does NOT do this - forcing an SBH node hangs DS6.
		alembicNodeDecoder.updateGeometryCaches();

		alembicNodeDecoder.writeObjects((currentFrame == startFrame ? true : false));

		alembicProgress.step();
	}

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Finished exporting alembic frames"));

	m_Archive.reset();

	dzScene->setFrame(startFrame);

	QApplication::processEvents();

	if (dthWriter_ != nullptr) dthWriter_->setAlembicRomPath(alembicExportPath_);

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Writing alembic archive"));

	alembicProgress.finish();

}

void DthAlembicExporter::doGroomPosesExport()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting alembic groom frames"));

	exportProgress_.setCurrentInfo("Exporting alembic groom frames");

	alembicExportPath_ = exportDirectory_ + "/" + characterName_ + "_grooms.abc";

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Creating Alembic archive"));

	try
	{
		m_Archive = std::make_unique<Alembic::Abc::OArchive >(Alembic::AbcCoreOgawa::WriteArchive(), alembicExportPath_.toLatin1().constData());
		m_TimeSampling = std::make_shared< Alembic::Abc::TimeSampling>((double)dzScene->getTimeStep() / 4800, 0.0);
	}
	catch (...)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHERROR, QString("Could not create alembic archive"));
		throw std::runtime_error("Could not create alembic archive. Make sure the Alembic file is not locked by another application such as Houdini.");
	}

	// Decode nodes
	Sagan::AlembicNodeDecoder alembicNodeDecoder(this, dazHelpers_, dthWriter_);
	alembicNodeDecoder.setShapeNameFormatter(DthStaticHelpers::getFormattedShapeNameAsString);
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Decoding nodes"));
	alembicNodeDecoder.decodeSelected(selectedRootNode_);

	int startFrame = 0;
	int endFrame = 1;

	// Initialise progress display
	DzProgress alembicProgress = DzProgress("", endFrame, false, true);
	alembicProgress.setCloseOnFinish(false);

	// Export frames
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting alembic frames"));

	for (int currentFrame = startFrame; currentFrame <= endFrame; currentFrame++)
	{
		alembicProgress.setCurrentInfo("Exporting alembic frame " + QString::number(currentFrame));

		dzScene->setFrame(currentFrame);

		QApplication::processEvents();

		alembicNodeDecoder.writeObjects((currentFrame == startFrame ? true : false));

		alembicProgress.step();
	}

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Finished exporting alembic frames"));

	m_Archive.reset();

	dzScene->setFrame(startFrame);

	QApplication::processEvents();

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Writing alembic archive"));

	alembicProgress.finish();
}