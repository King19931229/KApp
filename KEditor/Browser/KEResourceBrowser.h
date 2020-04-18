#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QStandardItemModel>
#include "ui_KEResourceBrowser.h"

#include "KBase/Interface/IKFileSystem.h"

#include "Browser/KEFileSystemTreeItem.h"
#include "Browser/KEFileSystemModel.h"
#include "Browser/KEResourcePathModel.h"
#include "Browser/KEResourceItemWidget.h"
#include "Browser/KEResourceTreeWidget.h"

struct KEFileSystemComboData : public QObjectUserData
{
	IKFileSystemPtr system;
};
Q_DECLARE_METATYPE(KEFileSystemComboData);

class KEResourceBrowser : public QMainWindow
{
	Q_OBJECT
public:
	KEResourceBrowser(QWidget *parent = Q_NULLPTR);
	~KEResourceBrowser();

	QSize TreeWidgetSize() const;
	QSize ItemWidgetSize() const;

	QSize sizeHint() const override;
	void resizeEvent(QResizeEvent* event) override;

	bool Init();
	bool UnInit();
	void RefreshPathView(KEFileSystemTreeItem* item);
	void RefreshTreeView(KEFileSystemTreeItem* item);
	void RefreshItemView(KEFileSystemTreeItem* item);
protected:
	QWidget* m_MainWindow;
	KEFileSystemTreeItem* m_RootItem;

	QDockWidget* m_TreeDockWidget;
	QDockWidget* m_ItemDockWidget;

	KEResourceTreeWidget* m_TreeWidget;
	KEResourceItemWidget* m_ItemWidget;

	KEFileSystemModel* m_TreeModel;
	KEFileSystemModel* m_ItemModel;
	KEResourcePathModel* m_PathModel;

	float m_TreeWidgetRatio;
	float m_ItemWidgetRatio;
	bool m_Initing;
protected Q_SLOTS:
	void OnComboIndexChanged(int index);
	void OnTreeViewClicked(QModelIndex index);
	void OnTreeViewBack(bool);
	void OnPathViewClicked(QModelIndex index);
private:
	Ui::KEResourceBrowser ui;
};
