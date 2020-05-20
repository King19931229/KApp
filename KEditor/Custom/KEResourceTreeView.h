#pragma once
#include <QTreeView>
#include <QMouseEvent>
#include <QFileSystemWatcher>
#include "KEditorConfig.h"
#include <unordered_set>

class KEResourceTreeView : public QTreeView
{
protected:
	QFileSystemWatcher* m_Watcher;
	std::unordered_set<std::string> m_ExpanedPaths;

	void OnFileChange(const QString& path);
	void OnDirectoryChange(const QString& path);

	void SetupWatcher(const std::string& path);
	void UninstallWatcher(const std::string& path);

	void OnExpanded(const QModelIndex &index);
	void OnCollapsed(const QModelIndex &index);

	void ExpandIntoPreviousResult();
public:
	KEResourceTreeView(QWidget *parent = Q_NULLPTR)
		: QTreeView(parent),
		m_Watcher(nullptr)
	{
		connect(this, &KEResourceTreeView::expanded, this, &KEResourceTreeView::OnExpanded);
		connect(this, &KEResourceTreeView::collapsed, this, &KEResourceTreeView::OnCollapsed);
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