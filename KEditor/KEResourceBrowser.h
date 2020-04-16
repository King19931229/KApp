#pragma once

#include <QHBoxLayout>
#include <QListView>
#include "ui_KEResourceBrowser.h"
#include "KBase/Interface/IKFileSystem.h"
#include "Browser/KEFileSystemTreeItem.h"

struct KEFileSystemComboData : public QObjectUserData
{
	IKFileSystemPtr system;
};
Q_DECLARE_METATYPE(KEFileSystemComboData);

class KEFileSystemModel : public QAbstractItemModel
{
	Q_OBJECT
protected:
	KEFileSystemTreeItem* m_Item;
	bool m_ViewDir;
public:
	KEFileSystemModel(bool viewDir, QObject *parent = nullptr);
	~KEFileSystemModel();

	void SetItem(KEFileSystemTreeItem* item);
	KEFileSystemTreeItem* GetItem();

	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column,
		const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
};

class KEResourceBrowser : public QWidget
{
	Q_OBJECT
public:
	KEResourceBrowser(QWidget *parent = Q_NULLPTR);
	~KEResourceBrowser();

	QSize sizeHint() const override;

	bool Init();
	bool UnInit();
	void RefreshView();
	void RefreshTreeView();
	void RefreshItemView();

protected:
	QWidget* m_MainWindow;
	KEFileSystemTreeItem* m_RootItem;

	KEFileSystemModel* m_TreeModel;
	KEFileSystemModel* m_ItemModel;
protected Q_SLOTS:
	void OnComboIndexChanged(int index);
	void OnTreeViewClicked(QModelIndex index);
	void OnTreeViewBack(bool);
private:
	Ui::KEResourceBrowser ui;
};
