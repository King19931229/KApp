#include "KEMaterialPropertyTreeModel.h"

KEMaterialPropertyTreeModel::KEMaterialPropertyTreeModel(QObject *parent)
	: QAbstractItemModel(parent),
	m_RootItem(nullptr)
{
}

KEMaterialPropertyTreeModel::~KEMaterialPropertyTreeModel()
{
	SAFE_DELETE(m_RootItem);
}

void KEMaterialPropertyTreeModel::SetMaterial(IKMaterial* material)
{
	SAFE_DELETE(m_RootItem);
	if (material)
	{
		m_RootItem = KNEW KEMaterialPropertyItem(nullptr);
		m_RootItem->InitAsMaterial(material);
	}
}

QVariant KEMaterialPropertyTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole)
	{
		KEMaterialPropertyItem* item = static_cast<KEMaterialPropertyItem*>(index.internalPointer());
		if (item)
		{
			return QVariant(item->GetName().c_str());
		}
	}

	return QVariant();
}

Qt::ItemFlags KEMaterialPropertyTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant KEMaterialPropertyTreeModel::headerData(int section, Qt::Orientation orientation,
	int role) const
{
	return QVariant();
}

QModelIndex KEMaterialPropertyTreeModel::index(int row, int column,
	const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	KEMaterialPropertyItem* item = nullptr;
	if (!parent.isValid())
	{
		item = m_RootItem;
	}
	else
	{
		item = static_cast<KEMaterialPropertyItem*>(parent.internalPointer());
	}

	KEMaterialPropertyItem* childItem = item->GetChild(row);
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

QModelIndex KEMaterialPropertyTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return QModelIndex();
	}

	KEMaterialPropertyItem* childItem = static_cast<KEMaterialPropertyItem*>(index.internalPointer());
	if (childItem)
	{
		KEMaterialPropertyItem *parentItem = childItem->GetParent();

		if (parentItem == m_RootItem)
			return QModelIndex();

		return createIndex(parentItem->GetIndex(), 0, parentItem);
	}

	return QModelIndex();
}

int KEMaterialPropertyTreeModel::rowCount(const QModelIndex &parent) const
{
	KEMaterialPropertyItem *parentItem = nullptr;

	if (!parent.isValid())
		parentItem = m_RootItem;
	else
		parentItem = static_cast<KEMaterialPropertyItem*>(parent.internalPointer());

	if (parentItem)
	{
		return (int)parentItem->GetChildCount();
	}
	else
	{
		return 0;
	}
}

int KEMaterialPropertyTreeModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}