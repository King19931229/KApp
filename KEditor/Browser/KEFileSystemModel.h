#pragma once
#include <QAbstractItemModel>
#include "KBase/Interface/IKFileSystem.h"
#include "Browser/KEFileSystemTreeItem.h"

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