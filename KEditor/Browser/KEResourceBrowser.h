#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include "ui_KEResourceBrowser.h"

#include "KBase/Interface/IKFileSystem.h"

#include "Browser/KEFileSystemTreeItem.h"
#include "Browser/KEFileSystemModel.h"
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
	void RefreshView();
	void RefreshTreeView();
	void RefreshItemView();
protected:
	QWidget* m_MainWindow;
	KEFileSystemTreeItem* m_RootItem;

	QDockWidget* m_TreeDockWidget;
	QDockWidget* m_ItemDockWidget;

	KEResourceTreeWidget* m_TreeWidget;
	KEResourceItemWidget* m_ItemWidget;

	KEFileSystemModel* m_TreeModel;
	KEFileSystemModel* m_ItemModel;

	float m_TreeWidgetRatio;
	float m_ItemWidgetRatio;
	bool m_Initing;
protected Q_SLOTS:
	void OnComboIndexChanged(int index);
	void OnTreeViewClicked(QModelIndex index);
	void OnTreeViewBack(bool);
private:
	Ui::KEResourceBrowser ui;
};
