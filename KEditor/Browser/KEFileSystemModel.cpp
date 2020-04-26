#include "KEFileSystemModel.h"

KEFileSystemModel::KEFileSystemModel(bool viewDir, QObject *parent)
	: QAbstractItemModel(parent),
	m_ViewDir(viewDir),
	m_Item(nullptr)
{
}

KEFileSystemModel::~KEFileSystemModel()
{
}

void KEFileSystemModel::SetItem(KEFileSystemTreeItem* item)
{
	m_Item = item;
}

KEFileSystemTreeItem* KEFileSystemModel::GetItem()
{
	return m_Item;
}

QVariant KEFileSystemModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (hasIndex(index.row(), index.column()))
	{
		if (role == Qt::DisplayRole)
		{
			KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
			return QVariant(item->GetName().c_str());
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
	if (hasIndex(childItem->GetIndex(), 0))
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