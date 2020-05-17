#pragma once
#include <QAbstractItemModel>
#include <QFileSystemWatcher>

#include "KBase/Interface/IKFileSystem.h"
#include "Browser/KEFileSystemTreeItem.h"

class KEResourceItemView;

class KEFileSystemModel : public QAbstractItemModel
{
	Q_OBJECT
protected:
	KEFileSystemTreeItem* m_Item;
	bool m_ViewDir;
public:
	KEFileSystemModel(bool viewDir, QObject *parent = nullptr);
	~KEFileSystemModel();

	bool FindIndex(QModelIndex parent, const std::string& path, QModelIndex& ret);

	void SetItem(KEFileSystemTreeItem* item);
	KEFileSystemTreeItem* GetItem();

	void BeginResetModel();
	void EndResetModel();

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