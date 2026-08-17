#include <stdexcept>
#include <memory>

#include <QMessageBox>
#include <QFileDialog>

#include "dzapp.h"
#include "dzelement.h"

#include "dth_exporter_gui.h"
#include "dth/dth_exporter.h"
#include "dth/dth_settings.h"

#include "version.h"

DthExporterGui::DthExporterGui(QWidget* parent) : QDialog(parent)
{

	setupUi(this);

	lblVersion->setText(QString::fromStdString("Version ") + PLUGIN_VERSION_STRING);

	settings = dthSettings.getSettings();

	QDir lastExportPathDir(settings.lastExportPath);
	QDir defaultExportPathDir(settings.defaultExportPath);

	if (settings.lastExportPath != "" && lastExportPathDir.exists())
	{
		txtExportDirectory->setText(settings.lastExportPath);
	}
	else if (settings.defaultExportPath != "" && defaultExportPathDir.exists())
	{
		txtExportDirectory->setText(settings.defaultExportPath);
	}
	else
	{
		txtExportDirectory->setText(QDir::homePath() + "/Desktop");
	}

	if (settings.lastCharacterName != "")
	{
		txtCharacterName->setText(settings.lastCharacterName);
	}
	else if (settings.defaultCharacterName != "")
	{
		txtCharacterName->setText(settings.defaultCharacterName);
	}
	else
	{
		txtCharacterName->setText("Character");
	}

}

void DthExporterGui::handleBrowseClicked()
{

	const auto dir = txtExportDirectory->text();

	QString exportDirectory = QFileDialog::getExistingDirectory(nullptr, "Select or Create a Directory", dir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::Option(QFileDialog::DontUseNativeDialog));
	if (!exportDirectory.isEmpty())
	{
		txtExportDirectory->setText(exportDirectory);
	}

}

void DthExporterGui::handleExportClicked()
{
	DthExporter dthExporter = DthExporter();
	dthExporter.doExport(txtExportDirectory->text(), txtCharacterName->text(), txtReferenceFrames->text(), true);

	accept();

}

void DthExporterGui::handleExportAnimationOnlyClicked()
{
	DthExporter dthExporter = DthExporter();
	dthExporter.doExportAnimation(txtExportDirectory->text(), txtCharacterName->text(), txtAnimationName->text(), true);

	accept();

}

void DthExporterGui::handleExportAlembicGroomPosesClicked()
{
	DthExporter dthExporter = DthExporter();
	dthExporter.doExportAlembicGroomPoses(txtExportDirectory->text(), txtCharacterName->text(), true);

	accept();

}

void DthExporterGui::handleCloseClicked()
{

	reject();

}

#include "moc_dth_exporter_gui.cpp"