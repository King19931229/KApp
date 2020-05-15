#include "KEReflectObjectTreeModel.h"
#include "KEditorConfig.h"

KEReflectObjectTreeModel::KEReflectObjectTreeModel(QObject *parent)
	: QAbstractItemModel(parent),
	m_RootItem(nullptr)
{
}

KEReflectObjectTreeModel::~KEReflectObjectTreeModel()
{
	SAFE_DELETE(m_RootItem);
}

void KEReflectObjectTreeModel::SetReflection(KReflectionObjectBase* reflection)
{
	SAFE_DELETE(m_RootItem);
	m_RootItem = new KEReflectionObjectItem(nullptr, reflection);
}

QVariant KEReflectObjectTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (hasIndex(index.row(), index.column()))
	{
		if (role == Qt::DisplayRole)
		{
			KEReflectionObjectItem* item = static_cast<KEReflectionObjectItem*>(index.internalPointer());
			return QVariant(item->GetName().c_str());
		}
	}

	return QVariant();
}

Qt::ItemFlags KEReflectObjectTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant KEReflectObjectTreeModel::headerData(int section, Qt::Orientation orientation,
	int role) const
{
	return QVariant();
}

QModelIndex KEReflectObjectTreeModel::index(int row, int column,
	const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	KEReflectionObjectItem* item = nullptr;
	if (!parent.isValid())
	{
		item = m_RootItem;
	}
	else
	{
		item = static_cast<KEReflectionObjectItem*>(parent.internalPointer());
	}

	KEReflectionObjectItem* childItem = item->GetChild(row);
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

QModelIndex KEReflectObjectTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return QModelIndex();
	}

	KEReflectionObjectItem* childItem = static_cast<KEReflectionObjectItem*>(index.internalPointer());
	if (hasIndex(childItem->GetIndex(), 0))
	{
		KEReflectionObjectItem *parentItem = childItem->GetParent();

		if (!parentItem || parentItem == m_RootItem)
			return QModelIndex();

		return createIndex(parentItem->GetIndex(), 0, parentItem);
	}

	return QModelIndex();
}

int KEReflectObjectTreeModel::rowCount(const QModelIndex &parent) const
{
	KEReflectionObjectItem *parentItem = nullptr;

	if (!parent.isValid())
		parentItem = m_RootItem;
	else
		parentItem = static_cast<KEReflectionObjectItem*>(parent.internalPointer());

	if (parentItem)
	{
		return (int)parentItem->GetChildCount();
	}
	else
	{
		return 0;
	}
}

int KEReflectObjectTreeModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}