#include "KEResourcePathModel.h"
#include <stack>

KEResourcePathModel::KEResourcePathModel(QObject *parent)
	: QAbstractItemModel(parent),
	m_Item(nullptr)
{
}

KEResourcePathModel::~KEResourcePathModel()
{
	SAFE_DELETE(m_Item);
}

bool KEResourcePathModel::FindIndex(QModelIndex parent, const std::string& path, QModelIndex& ret)
{
	if (parent.isValid())
	{
		KEResourcePathItem* item = static_cast<KEResourcePathItem*>(parent.internalPointer());
		if (item->GetFullPath() == path)
		{
			ret = parent;
			return true;
		}
	}
	else
	{
		KEResourcePathItem* item = m_Item;
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
		KEResourcePathItem* item = static_cast<KEResourcePathItem*>(child.internalPointer());
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

void KEResourcePathModel::BeginResetModel()
{
	beginResetModel();
}

void KEResourcePathModel::EndResetModel()
{
	endResetModel();
}

void KEResourcePathModel::SetItem(KEFileSystemTreeItem* item)
{
	SAFE_DELETE(m_Item);

	std::vector<KEFileSystemTreeItem*> treeItems;

	std::stack<KEFileSystemTreeItem*> pathStack;
	KEFileSystemTreeItem* current = item;

	while (current)
	{
		pathStack.push(current);
		current = current->GetParent();
	}

	while (!pathStack.empty())
	{
		KEFileSystemTreeItem* top = pathStack.top();
		treeItems.push_back(top);
		pathStack.pop();
	}

	KEResourcePathItem* prePathItem = nullptr;
	int depth = 0;
	for (KEFileSystemTreeItem* item : treeItems)
	{
		KEResourcePathItem* pathItem = KNEW KEResourcePathItem(prePathItem,
			item,
			depth++);

		if (prePathItem)
		{
			prePathItem->SetChild(pathItem);
		}
		else
		{
			m_Item = pathItem;
		}
		prePathItem = pathItem;
	}
}

KEResourcePathItem* KEResourcePathModel::GetPathItem()
{
	return m_Item;
}

QVariant KEResourcePathModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole)
	{
		KEResourcePathItem* item = static_cast<KEResourcePathItem*>(index.internalPointer());
		return QVariant(item->GetName().c_str());
	}

	return QVariant();
}

Qt::ItemFlags KEResourcePathModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled;
}

QVariant KEResourcePathModel::headerData(int section, Qt::Orientation orientation,
	int role) const
{
	return QVariant();
}

QModelIndex KEResourcePathModel::index(int row, int column,
	const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	KEResourcePathItem *item = nullptr;
	if (!parent.isValid())
	{
		item = m_Item;
	}
	else
	{
		item = static_cast<KEResourcePathItem*>(parent.internalPointer());
	}

	KEResourcePathItem* childItem = item->GetChild();
	if (childItem)
	{
		return createIndex(row, column, childItem);
	}
	else
	{
		return QModelIndex();
	}
}

QModelIndex KEResourcePathModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return QModelIndex();
	}

	KEResourcePathItem* childItem = static_cast<KEResourcePathItem*>(index.internalPointer());
	KEResourcePathItem *parentItem = childItem->GetParent();

	if (parentItem == m_Item)
		return QModelIndex();

	return createIndex(0, 0, parentItem);
}

int KEResourcePathModel::rowCount(const QModelIndex &parent) const
{	
	KEResourcePathItem *item = nullptr;

	if (!parent.isValid())
		item = m_Item;
	else
		item = static_cast<KEResourcePathItem*>(parent.internalPointer());

	if (item)
	{
		return 1;
	}
	return 0;
}

int KEResourcePathModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}