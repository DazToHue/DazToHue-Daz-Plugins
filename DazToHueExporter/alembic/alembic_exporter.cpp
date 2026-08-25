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

	int startFrame = dzScene->getPlayRange().getStart() / dzScene->getTimeStep();
	int endFrame = dzScene->getPlayRange().getEnd() / dzScene->getTimeStep();

	// Initialise progress display
	DzProgress alembicProgress = DzProgress("", endFrame, false, true);
	alembicProgress.setCloseOnFinish(false);

	int currentFrame = startFrame;
	QStringList motionSummary;

	// Decoding and the frame loop are contained for the same reason the
	// archive creation above is: whatever throws in here - Alembic, Ogawa, a
	// bad_alloc - must not unwind past the plugin. A mid-loop death was
	// measured (2026-08-20) that reached "Exporting alembic frames", wrote 11
	// MB of an expected 337 MB, and logged NOTHING; the frame markers below
	// plus this catch are what turn a repeat into a located failure.
	try
	{
		// Decode nodes
		Sagan::AlembicNodeDecoder alembicNodeDecoder(this, dazHelpers_, dthWriter_);
		alembicNodeDecoder.setShapeNameFormatter(DthStaticHelpers::getFormattedShapeNameAsString);
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Decoding nodes"));
		alembicNodeDecoder.decodeSelected(selectedRootNode_);

		// Export frames
		DthFaultProbe::setStage("alembic ROM");

		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Exporting alembic frames %1 to %2").arg(startFrame).arg(endFrame));

		// Staleness accounting for the loop below. setFrame() +
		// processEvents() is a REQUEST to evaluate, not a guarantee: measured
		// 2026-08-25 (Ita, DS4, 2.1.9), a 484-frame walk ran ~10x fast,
		// sampled the rest pose 484 times, wrote a 23.7 MB statue archive and
		// reported success - which then cost the previous good export its
		// .dthprev backups. Same scene, same build, same session shape
		// exported correctly an hour earlier; whatever suppresses evaluation
		// is a session state, not scene data (the saved ROM has 3,978 fully
		// keyed channels). So the loop measures whether geometry actually
		// changed, re-asks Daz to evaluate when it did not, and the gate
		// after the loop refuses to report a statue as success.
		int staleFrames = 0;
		bool previousFrameStale = false;

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

			// The previous frame's sample came out byte-identical for every
			// mesh, so evaluation is not keeping up with setFrame(). Ask for
			// it directly (update+finalize on the exported nodes) before this
			// frame is sampled. This runs ONLY on that evidence - it is not
			// the reverted always-on forcing of #2/#8.
			if (previousFrameStale)
			{
				alembicNodeDecoder.refreshExportedGeometry();
			}

			const int meshesMoved = alembicNodeDecoder.writeObjects((currentFrame == startFrame ? true : false));

			previousFrameStale = (currentFrame > startFrame && meshesMoved == 0);

			if (previousFrameStale)
			{
				staleFrames++;

				if (dthLogger_ != nullptr && (staleFrames == 1 || staleFrames % 50 == 0))
				{
					dthLogger_->log(LogLevel::DTHWARNGING, QString("Frame %1 sampled with NO geometry change on any mesh (%2 stale frame(s) so far) - re-requesting evaluation").arg(currentFrame).arg(staleFrames));
				}
			}

			alembicProgress.step();
		}

		motionSummary = alembicNodeDecoder.getMotionSummary();

		if (staleFrames > 0 && dthLogger_ != nullptr)
		{
			dthLogger_->log(LogLevel::DTHWARNGING, QString("%1 of %2 frames sampled no geometry change on any mesh").arg(staleFrames).arg(endFrame - startFrame));
		}

		// The gate. A multi-frame ROM in which NOTHING ever moved is a statue
		// - the walk ran, the scene never followed - and reporting it as
		// success is what destroyed a good export's backups on 2026-08-25.
		// Throwing here reaches the caller through the same containment as
		// every other failure: logged, script-visible, no .dth written (the
		// manifest is written last), so the studio's export-landed guard
		// fails the run and the backups survive.
		const QStringList frozenMeshes = alembicNodeDecoder.getFrozenMeshes();

		if (endFrame > startFrame && !frozenMeshes.isEmpty())
		{
			const bool everythingFrozen = frozenMeshes.count() == alembicNodeDecoder.getWrittenMeshCount();

			for (const QString& label : frozenMeshes)
			{
				if (dthLogger_ != nullptr) dthLogger_->log(everythingFrozen ? LogLevel::DTHERROR : LogLevel::DTHWARNGING, QString("Mesh '%1' never changed across the ROM - a statue").arg(label));
			}

			if (everythingFrozen)
			{
				throw std::runtime_error(QString("the ROM walk ran %1 frames but the scene never re-evaluated: every one of the %2 exported meshes is a statue (identical geometry on every frame). The scene's animation data is intact; Daz did not evaluate it during the export. Close and reopen the scene (or restart Daz Studio) and export again.")
					.arg(endFrame - startFrame + 1).arg(frozenMeshes.count()).toUtf8().constData());
			}
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

	// Per-mesh motion, recorded during the bake. A fitted item sitting far
	// below the figure here is the signature of clothing that stopped
	// following the body: measured 2026-08-21, a good ROM and one with
	// part-frozen clothing produced export logs that were BYTE-IDENTICAL,
	// 212 lines each - the difference was only visible by probing the .abc in
	// Houdini. This is that probe's answer, written where the export already
	// writes.
	if (dthLogger_ != nullptr)
	{
		dthLogger_->log(LogLevel::DTHINFO, QString("Alembic ROM motion summary"));

		for (const QString& line : motionSummary)
		{
			dthLogger_->log(LogLevel::DTHINFO, QString("  %1").arg(line));
		}
	}

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

	int startFrame = 0;
	int endFrame = 1;

	// Initialise progress display
	DzProgress alembicProgress = DzProgress("", endFrame, false, true);
	alembicProgress.setCloseOnFinish(false);

	int currentFrame = startFrame;

	// Contained for the same reason as doRomExport()'s loop above.
	try
	{
		// Decode nodes
		Sagan::AlembicNodeDecoder alembicNodeDecoder(this, dazHelpers_, dthWriter_);
		alembicNodeDecoder.setShapeNameFormatter(DthStaticHelpers::getFormattedShapeNameAsString);
		if (dthLogger_ != nullptr) dthLogger_->log(LogLevel::DTHINFO, QString("Decoding nodes"));
		alembicNodeDecoder.decodeSelected(selectedRootNode_);

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