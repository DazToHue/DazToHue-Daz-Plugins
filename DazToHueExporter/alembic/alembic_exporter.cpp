#include <unordered_set>
#include <stdexcept>

#include "../daz/daz_helpers.h"
#include "../dth/dth_static_helpers.h"
#include "../dth/dth_fault_probe.h"

#include "alembic_exporter.h"
#include "sagan/decoder/alembic_node_decoder.h"

DthAlembicExporter::DthAlembicExporter(QString exportDirectory, QString characterName, DzNode* selectedRootNode, DazHelpers& dazHelpers, DthWriter* dthWriter, DzProgress& exportProgress, const Sagan::OutputTransformer* outputTransformer, DthLogger* dthLogger) : exportDirectory_(exportDirectory), characterName_(characterName), selectedRootNode_(selectedRootNode), dazHelpers_(dazHelpers), dthWriter_(dthWriter), exportProgress_(exportProgress), SaganExporter(outputTransformer), dthLogger_(dthLogger)
{
}

DthAlembicExporter::~DthAlembicExporter()
{
}

void DthAlembicExporter::createArchive()
{
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
}

std::unique_ptr<Sagan::AlembicNodeDecoder> DthAlembicExporter::decodeSettledMeshes()
{
	// Decode builds each exportable's visible2OriginalVertexIndices ONCE and
	// holds it const; the frame loop re-reads the LIVE facet mesh every frame.
	// Those two only agree while the mesh keeps the vertex count it was
	// decoded with - and doExport's preprocessing drops every mesh to Base
	// resolution first, with the re-cooks landing ASYNCHRONOUSLY. Any re-cook
	// arriving after decode leaves the map addressing a mesh that no longer
	// exists.
	//
	// Measured 2026-08-24 (2.1.5's fault probe, then 2.1.6's guard): a scene
	// saved with viewport subdivision 1 on 8 nodes failed 4 of 4 exports, each
	// on a different node and frame - 'Genesis 9 Tear' read decode-time index
	// 503 into a mesh the drop to Base had left with 280 vertices. Scenes
	// saved at Base never failed. The 2.1.7 guard in getOptimizedMeshVertices
	// turns that read into an honest error rather than an access violation; it
	// stays exactly as it is, and remains the backstop if anything below is
	// ever wrong.
	//
	// So settle first, then verify, and only then write: decode, let the
	// pending cooks land, and compare every decoded mesh against the live one.
	// The frame loop must start from index maps built for the meshes its
	// frames will actually read.
	const int maxDecodeAttempts = 3;

	for (int attempt = 1; ; attempt++)
	{
		auto decoder = std::make_unique<Sagan::AlembicNodeDecoder>(this, dazHelpers_, dthWriter_);
		decoder->setShapeNameFormatter(DthStaticHelpers::getFormattedShapeNameAsString);

		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Decoding nodes"));
		decoder->decodeSelected(selectedRootNode_);

		// Give whatever is still cooking a chance to land before asking.
		// Pumping is ALL this may do: forcing evaluation is not available -
		// PR #8 reverted the per-frame forceCacheUpdate() after it crashed the
		// plugin on a simple naked figure, and that revert stands. So this
		// WAITS for the scene rather than compelling it, which is why it is
		// bounded and why it reports honestly instead of insisting.
		QApplication::processEvents();
		QApplication::processEvents();

		const QStringList unstable = decoder->getUnstableMeshes();

		if (unstable.isEmpty())
		{
			if (attempt > 1 && dthLogger_ != nullptr)
			{
				dthLogger_->log(LogLevel::DTHINFO, QString("Meshes settled after %1 decode attempts").arg(attempt));
			}

			return decoder;
		}

		if (dthLogger_ != nullptr)
		{
			dthLogger_->log(LogLevel::DTHWARNGING, QString("%1 mesh(es) changed while decoding: %2").arg(unstable.count()).arg(unstable.join(", ")));
		}

		if (attempt >= maxDecodeAttempts)
		{
			throw std::runtime_error(QString("the scene would not settle: after %1 attempts to prepare it, mesh %2 was still changing size (%3). This almost always means the scene is saved with viewport resolution above Base: the export switches every mesh to Base and those re-cooks are still landing. In Daz, set Resolution Level to Base on the figure and its followers (Parameters > Mesh Resolution), save the scene, and export again.")
				.arg(maxDecodeAttempts).arg(unstable.first()).arg(unstable.join(", ")).toUtf8().constData());
		}

		// Re-decode. initObject() creates an OXform/OPolyMesh per node INSIDE
		// the archive at decode time, so decoding twice into one archive would
		// collide on names - the second decode needs a fresh archive. Nothing
		// has been written yet (the first sample is set in the frame loop), so
		// throwing this one away costs only the file handle. Order matters:
		// the objects go before the archive that owns them.
		if (dthLogger_ != nullptr)
		{
			dthLogger_->log(LogLevel::DTHINFO, QString("Re-decoding against the settled meshes (attempt %1 of %2)").arg(attempt + 1).arg(maxDecodeAttempts));
		}

		m_AlembicMeshObjects.clear();
		m_exportableMeshObjects.clear();
		m_Archive.reset();

		createArchive();
	}
}

void DthAlembicExporter::doRomExport()
{
	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting Alembic ROM"));

	exportProgress_.setCurrentInfo("Exporting alembic ROM");

	alembicExportPath_ = exportDirectory_ + "/" + characterName_ + ".abc";

	createArchive();

	int startFrame = dzScene->getPlayRange().getStart() / dzScene->getTimeStep();
	int endFrame = dzScene->getPlayRange().getEnd() / dzScene->getTimeStep();

	// Initialise progress display
	DzProgress alembicProgress = DzProgress("", endFrame, false, true);
	alembicProgress.setCloseOnFinish(false);

	int currentFrame = startFrame;

	// Decoding and the frame loop are contained for the same reason the
	// archive creation above is: whatever throws in here - Alembic, Ogawa, a
	// bad_alloc - must not unwind past the plugin. A mid-loop death was
	// measured (2026-08-20) that reached "Exporting alembic frames", wrote 11
	// MB of an expected 337 MB, and logged NOTHING; the frame markers below
	// plus this catch are what turn a repeat into a located failure.
	try
	{
		// Decode, settle, verify - the frame loop below only ever reads index
		// maps that still match their meshes.
		DthFaultProbe::setStage("alembic ROM decode");
		auto decoderPtr = decodeSettledMeshes();
		Sagan::AlembicNodeDecoder& alembicNodeDecoder = *decoderPtr;

		// Export frames
		DthFaultProbe::setStage("alembic ROM");

		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting alembic frames %1 to %2").arg(startFrame).arg(endFrame));

		for (currentFrame = startFrame; currentFrame <= endFrame; currentFrame++)
		{
			DthFaultProbe::setFrame(currentFrame);

			alembicProgress.setCurrentInfo("Exporting alembic frame " + QString::number(currentFrame));

			// A marker every 10 frames: enough to locate a silent death, few
			// enough to keep the log readable on a 240-frame ROM.
			if (dthLogger_ != nullptr && (currentFrame == startFrame || currentFrame % 10 == 0))
			{
				dthLogger_->log(LogLevel::DTHINFO, QString("Alembic frame %1").arg(currentFrame));
			}

			dzScene->setFrame(currentFrame);

			QApplication::processEvents();

			alembicNodeDecoder.writeObjects((currentFrame == startFrame ? true : false));

			alembicProgress.step();
		}
	}
	catch (const std::exception& e)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHERROR, QString("Alembic ROM export failed at frame %1 - %2").arg(currentFrame).arg(QString::fromUtf8(e.what())));
		m_Archive.reset();
		alembicProgress.finish();
		throw std::runtime_error(QString("Alembic ROM export failed at frame %1 - %2").arg(currentFrame).arg(QString::fromUtf8(e.what())).toUtf8().constData());
	}
	catch (...)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHERROR, QString("Alembic ROM export failed at frame %1 with an unrecognised error").arg(currentFrame));
		m_Archive.reset();
		alembicProgress.finish();
		throw std::runtime_error(QString("Alembic ROM export failed at frame %1 with an unrecognised error").arg(currentFrame).toUtf8().constData());
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

	createArchive();

	int startFrame = 0;
	int endFrame = 1;

	// Initialise progress display
	DzProgress alembicProgress = DzProgress("", endFrame, false, true);
	alembicProgress.setCloseOnFinish(false);

	int currentFrame = startFrame;

	// Contained for the same reason as doRomExport()'s loop above.
	try
	{
		// Same decode-then-read-live shape as the ROM leg, reached through the
		// same preprocessing (doExportAlembicGroomPoses calls preprocessScene,
		// so the drop to Base and its async re-cooks apply here too), and the
		// same const index maps: getSBHVertices() is declared but never
		// called, so grooms are read through getOptimizedMeshVertices() like
		// everything else and carry the identical stale-index hazard. Hence
		// the same settle - but WITHOUT the forced refresh, because this leg
		// deliberately never force-cooks (it hangs DS6 on strand-based hair).
		// If the meshes are already stable this costs one processEvents() and
		// a vertex-count comparison.
		DthFaultProbe::setStage("alembic groom poses decode");
		auto decoderPtr = decodeSettledMeshes();
		Sagan::AlembicNodeDecoder& alembicNodeDecoder = *decoderPtr;

		// Export frames
		DthFaultProbe::setStage("alembic groom poses");

		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting alembic frames"));

		for (currentFrame = startFrame; currentFrame <= endFrame; currentFrame++)
		{
			alembicProgress.setCurrentInfo("Exporting alembic frame " + QString::number(currentFrame));

			dzScene->setFrame(currentFrame);

			QApplication::processEvents();

			alembicNodeDecoder.writeObjects((currentFrame == startFrame ? true : false));

			alembicProgress.step();
		}
	}
	catch (const std::exception& e)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHERROR, QString("Alembic groom pose export failed at frame %1 - %2").arg(currentFrame).arg(QString::fromUtf8(e.what())));
		m_Archive.reset();
		alembicProgress.finish();
		throw std::runtime_error(QString("Alembic groom pose export failed at frame %1 - %2").arg(currentFrame).arg(QString::fromUtf8(e.what())).toUtf8().constData());
	}
	catch (...)
	{
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHERROR, QString("Alembic groom pose export failed at frame %1 with an unrecognised error").arg(currentFrame));
		m_Archive.reset();
		alembicProgress.finish();
		throw std::runtime_error(QString("Alembic groom pose export failed at frame %1 with an unrecognised error").arg(currentFrame).toUtf8().constData());
	}

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Finished exporting alembic frames"));

	m_Archive.reset();

	dzScene->setFrame(startFrame);

	QApplication::processEvents();

	if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Writing alembic archive"));

	alembicProgress.finish();
}