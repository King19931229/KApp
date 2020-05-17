#pragma once
#include <QTreeView>
#include <QMouseEvent>
#include <QFileSystemWatcher>
#include "KEditorConfig.h"

class KEResourceTreeView : public QTreeView
{
protected:
	QFileSystemWatcher* m_Watcher;

	void OnFileChange(const QString& path);
	void OnDirectoryChange(const QString& path);
	bool FindIndex(QModelIndex parent, const std::string& path, QModelIndex& ret);

	void SetupWatcher(const std::string& path);
public:
	KEResourceTreeView(QWidget *parent = Q_NULLPTR)
		: QTreeView(parent),
		m_Watcher(nullptr)
	{
	}
	
	~KEResourceTreeView()
	{
		SAFE_DELETE(m_Watcher);
	}

	void mouseMoveEvent(QMouseEvent *event) override
	{
		event->ignore();
	}

	void setModel(QAbstractItemModel *model) override;
};