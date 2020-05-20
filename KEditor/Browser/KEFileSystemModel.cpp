#include "KEFileSystemModel.h"
#include "Custom/KEResourceTreeView.h"

#include <assert.h>

KEFileSystemModel::KEFileSystemModel(bool viewDir, QObject *parent)
	: QAbstractItemModel(parent),
	m_ViewDir(viewDir),
	m_Item(nullptr)
{	
}

KEFileSystemModel::~KEFileSystemModel()
{
}

bool KEFileSystemModel::FindIndex(QModelIndex parent, const std::string& path, QModelIndex& ret)
{
	if (parent.isValid())
	{
		KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(parent.internalPointer());
		if (item->GetFullPath() == path)
		{
			ret = parent;
			return true;
		}

	}
	else
	{
		KEFileSystemTreeItem* item = m_Item;
		if (item->GetFullPath() == path)
		{
			ret = parent;
			return true;
		}
	}

	QModelIndex child;

	if (parent.isValid())
	{
		child = parent.child(0, 0);
	}
	else
	{
		child = index(0, 0);
	}

	while (child.isValid())
	{
		KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(child.internalPointer());
		if (KStringUtil::StartsWith(path, item->GetFullPath()))
		{
			if (FindIndex(child, path, ret))
			{
				return true;
			}
		}
		child = child.sibling(child.row() + 1, child.column());
	}

	return false;
}

void KEFileSystemModel::SetItem(KEFileSystemTreeItem* item)
{
	m_Item = item;
}

KEFileSystemTreeItem* KEFileSystemModel::GetItem()
{
	return m_Item;
}

void KEFileSystemModel::BeginResetModel()
{
	beginResetModel();
}

void KEFileSystemModel::EndResetModel()
{
	endResetModel();
}

QVariant KEFileSystemModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole)
	{
		KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
		if (item)
		{
			return QVariant(QString::fromLocal8Bit(item->GetName().c_str()));
		}
	}

	return QVariant();
}

Qt::ItemFlags KEFileSystemModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant KEFileSystemModel::headerData(int section, Qt::Orientation orientation,
	int role) const
{
	return QVariant();
}

QModelIndex KEFileSystemModel::index(int row, int column,
	const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	KEFileSystemTreeItem* item = nullptr;
	if (!parent.isValid())
	{
		item = m_Item;
	}
	else
	{
		item = static_cast<KEFileSystemTreeItem*>(parent.internalPointer());
	}

	KEFileSystemTreeItem* childItem = m_ViewDir ? item->GetDir(static_cast<size_t>(row)) : item->GetFile(static_cast<size_t>(row));
	if (childItem)
	{
		QModelIndex index = createIndex(row, column, childItem);
		return index;
	}
	else
	{
		return QModelIndex();
	}
}

QModelIndex KEFileSystemModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return QModelIndex();
	}

	KEFileSystemTreeItem* childItem = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
	if (childItem)
	{
		KEFileSystemTreeItem *parentItem = childItem->GetParent();
		if (parentItem == m_Item)
			return QModelIndex();
		return createIndex(parentItem->GetIndex(), 0, parentItem);
	}

	return QModelIndex();
}

int KEFileSystemModel::rowCount(const QModelIndex &parent) const
{
	KEFileSystemTreeItem *parentItem = nullptr;

	if (!parent.isValid())
		parentItem = m_Item;
	else
		parentItem = static_cast<KEFileSystemTreeItem*>(parent.internalPointer());

	if (parentItem)
	{
		return (int)(m_ViewDir ? parentItem->GetNumDir() : parentItem->GetNumFile());
	}
	else
	{
		return 0;
	}
}

int KEFileSystemModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}