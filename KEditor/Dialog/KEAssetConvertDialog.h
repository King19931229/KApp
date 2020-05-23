#pragma once
#include <QDialog>
#include "ui_KEAssetConvertDialog.h"
#include "KBase/Interface/IKFileSystem.h"

class KEAssetConvertDialog : public QDialog
{
	Q_OBJECT
protected:
	Ui_KEAssetConvertDialog ui;
	IKFileSystem* m_FileSys;
public:
	KEAssetConvertDialog();
	~KEAssetConvertDialog();

	void SetAssetPath(const std::string& path);
	void SetMeshPath(const std::string& path);
	void SetFileSystem(IKFileSystem* fileSys);

	std::string GetAssetPath() const;
	std::string GetMeshPath() const;

	bool Process();
};