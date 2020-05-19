#pragma once
#include <QAbstractItemModel>
#include "KBase/Interface/IKFileSystem.h"
#include "Browser/KEFileSystemTreeItem.h"
#include "Browser/KEResourcePathItem.h"

class KEResourcePathModel : public QAbstractItemModel
{
	Q_OBJECT
protected:
	KEFileSystemTreeItem* m_TreeItem;
	KEResourcePathItem* m_PathItem;
	std::string m_FullPath;
public:
	KEResourcePathModel(QObject *parent = nullptr);
	~KEResourcePathModel();

	bool FindIndex(QModelIndex parent, const std::string& path, QModelIndex& ret);

	void BeginResetModel();
	void EndResetModel();

	void SetPath(const std::string& path);
	inline const std::string& GetPath() const { return m_FullPath; }

	void SetTreeItem(KEFileSystemTreeItem* item);
	KEFileSystemTreeItem* GetTreeItem();

	KEResourcePathItem* GetPathItem();

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