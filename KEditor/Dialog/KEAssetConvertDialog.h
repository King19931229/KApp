#pragma once
#include <QDialog>
#include "ui_KEAssetConvertDialog.h"

class KEAssetConvertDialog : public QDialog
{
	Q_OBJECT
protected:
	Ui_KEAssetConvertDialog ui;
public:
	KEAssetConvertDialog();
	~KEAssetConvertDialog();

	void SetAssetPath(const std::string& path);
	void SetMeshPath(const std::string& path);

	std::string GetAssetPath() const;
	std::string GetMeshPath() const;

	bool Process();
};