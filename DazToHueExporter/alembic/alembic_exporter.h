#pragma once

#include <tuple>
#include <array>
#include <memory>
#include <optional>
#include <vector>
#include <map>
#include <unordered_set>

#include <QtCore/QSet>
#include <QtCore/QVector>

#include "sagan/sagan_exporter.h"
#include "sagan/output_transformer/output_transformer.h"
#include "sagan/output_transformer/houdini_alembic_output_transformer.h"
#include "sagan/decoder/alembic_node_decoder.h"

#include "../daz/daz_helpers.h"
#include "../dth/dth_writer.h"
#include "../dth/dth_logger.h"

#include <dzprogress.h>

class DthAlembicExporter final : public Sagan::SaganExporter
{

public:

	DthAlembicExporter(QString exportDirectory, QString characterName, DzNode* selectedRootNode, DazHelpers& dazHelpers, DthWriter* dthWriter, DzProgress& exportProgress, const Sagan::OutputTransformer* outputTransformer, DthLogger* dthLogger_);
	~DthAlembicExporter();

	void doRomExport();
	void doGroomPosesExport();

private:

	/** Create (or re-create) the output archive at alembicExportPath_. */
	void createArchive();

	/**
		Decode the scene and hand back a decoder whose index maps are known to
		match the meshes the frame loop will read. Re-decodes into a fresh
		archive while they disagree; throws if they will not settle.

		It settles by pumping the event loop only - forcing evaluation is not
		on the table (PR #8), so this waits for the scene rather than
		compelling it, and says so honestly when waiting is not enough.
	*/
	std::unique_ptr<Sagan::AlembicNodeDecoder> decodeSettledMeshes();

	DazHelpers& dazHelpers_;
	DthWriter* dthWriter_ = nullptr;
	QString exportDirectory_;
	QString characterName_;
	DzNode* selectedRootNode_;
	DthLogger* dthLogger_ = nullptr;
	DzProgress& exportProgress_;
	QString alembicExportPath_;

};