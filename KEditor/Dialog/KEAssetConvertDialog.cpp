#include "KEAssetConvertDialog.h"
#include "KEditorGlobal.h"
#include "KBase/Publish/KFileTool.h"
#include <QFileDialog>

KEAssetConvertDialog::KEAssetConvertDialog()
{
	ui.setupUi(this);

	QObject::connect(ui.m_AssetPathBtn, &QPushButton::clicked, [this](bool checked)
	{
		QString path = QFileDialog::getOpenFileName(this, tr("Asset"), ".", tr("Asset(*.*)"));
		std::string trimPath;
		KFileTool::TrimPath(path.toStdString(), trimPath, false);
		ui.m_AssetPathEdit->setText(trimPath.c_str());
	});

	QObject::connect(ui.m_MeshPathBtn, &QPushButton::clicked, [this](bool checked)
	{
		QString path = QFileDialog::getSaveFileName(this, tr("Mesh"), ".", tr("Asset(*.mesh)"));
		std::string trimPath;
		KFileTool::TrimPath(path.toStdString(), trimPath, false);
		ui.m_MeshPathEdit->setText(trimPath.c_str());
	});
}

KEAssetConvertDialog::~KEAssetConvertDialog()
{
}

void KEAssetConvertDialog::SetAssetPath(const std::string& path)
{
	ui.m_AssetPathEdit->setText(path.c_str());
}

void KEAssetConvertDialog::SetMeshPath(const std::string& path)
{
	ui.m_MeshPathEdit->setText(path.c_str());
}

std::string KEAssetConvertDialog::GetAssetPath() const
{
	return ui.m_AssetPathEdit->text().toStdString();
}

std::string KEAssetConvertDialog::GetMeshPath() const
{
	return ui.m_MeshPathEdit->text().toStdString();
}

bool KEAssetConvertDialog::Process()
{
	std::string assetPath = GetAssetPath();
	std::string meshPath = GetMeshPath();
	if (!assetPath.empty() && !meshPath.empty())
	{
		return KEditorGlobal::ResourcePorter.Convert(assetPath, meshPath);
	}
	return false;
}