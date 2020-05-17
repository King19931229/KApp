#pragma once
#include <QColumnView>
#include <QFileSystemWatcher>
#include "KEditorConfig.h"

class KEResourcePathView : public QColumnView
{
protected:
	QFileSystemWatcher* m_Watcher;

	void OnFileChange(const QString& path);
	void OnDirectoryChange(const QString& path);

	void SetupWatcher(const std::string& path);
	void OnExpanded(const QModelIndex &index);
public:
	KEResourcePathView(QWidget *parent = Q_NULLPTR)
		: QColumnView(parent),
		m_Watcher(nullptr)
	{
		connect(this, &KEResourcePathView::activated, this, &KEResourcePathView::OnExpanded);
	}

	~KEResourcePathView()
	{
		SAFE_DELETE(m_Watcher);
	}

	void setModel(QAbstractItemModel *model) override;
};