#pragma once
#include <QListView>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMimeData>
#include <QDrag>
#include <QFileSystemWatcher>
#include "KEditorConfig.h"

class KEFileSystemTreeItem;
struct KEResourceItemDropData : public QObjectUserData
{
	KEFileSystemTreeItem* item;
	KEResourceItemDropData()
	{
		item = nullptr;
	}
	~KEResourceItemDropData()
	{
		item = nullptr;
	}
};

class KEResourceItemView : public QListView
{
	Q_OBJECT
protected:
	QFileSystemWatcher* m_Watcher;
	KEFileSystemTreeItem* m_RootItem;
	std::string m_FullPath;

	void OnFileChange(const QString& path);
	void OnDirectoryChange(const QString& path);
	void SetupWatcher(const std::string& path);
	void UninstallWatcher(const std::string& path);
public:
	KEResourceItemView(QWidget *parent = Q_NULLPTR);
	~KEResourceItemView();

	void mouseMoveEvent(QMouseEvent *event) override;
	void setModel(QAbstractItemModel *model) override;

	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

public Q_SLOTS:
	void OnOpenItemEditor(const QModelIndex &index);
};