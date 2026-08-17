#pragma once

#include <tuple>
#include <array>
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

	DazHelpers& dazHelpers_;
	DthWriter* dthWriter_ = nullptr;
	QString exportDirectory_;
	QString characterName_;
	DzNode* selectedRootNode_;
	DthLogger* dthLogger_ = nullptr;
	DzProgress& exportProgress_;
	QString alembicExportPath_;

};