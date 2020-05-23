#include "KEResourcePathModel.h"
#include <stack>

KEResourcePathModel::KEResourcePathModel(QObject *parent)
	: QAbstractItemModel(parent),
	m_PathItem(nullptr),
	m_TreeItem(nullptr)
{
}

KEResourcePathModel::~KEResourcePathModel()
{
	SAFE_DELETE(m_PathItem);
}

bool KEResourcePathModel::FindIndex(QModelIndex parent, const std::string& path, QModelIndex& ret)
{
	KEResourcePathItem* item = nullptr;
	if (parent.isValid())
	{
		item = static_cast<KEResourcePathItem*>(parent.internalPointer());
	}
	else
	{
		item = m_PathItem;
	}

	if (item->GetSystemFullPath() == path)
	{
		ret = parent;
		return true;
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
		if (KStringUtil::StartsWith(path, item->GetSystemFullPath()))
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

void KEResourcePathModel::SetPath(const std::string& path)
{
	ASSERT_RESULT(m_TreeItem);
	SAFE_DELETE(m_PathItem);

	m_FullPath = path;

	if (m_TreeItem)
	{
		KEFileSystemTreeItem* item = m_TreeItem->FindItem(path);

		if (item)
		{
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
					m_PathItem = pathItem;
				}
				prePathItem = pathItem;
			}
		}
		else
		{
			m_PathItem = KNEW KEResourcePathItem(nullptr, m_TreeItem, 0);
		}
	}
}

void KEResourcePathModel::SetTreeItem(KEFileSystemTreeItem* item)
{
	m_TreeItem = item;
}

KEFileSystemTreeItem* KEResourcePathModel::GetTreeItem()
{
	return m_TreeItem;
}


KEResourcePathItem* KEResourcePathModel::GetPathItem()
{
	return m_PathItem;
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
		return QVariant(QString::fromLocal8Bit(item->GetName().c_str()));
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
		item = m_PathItem;
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
	KEResourcePathItem* parentItem = childItem->GetParent();

	if (parentItem == m_PathItem)
		return QModelIndex();

	return createIndex(0, 0, parentItem);
}

int KEResourcePathModel::rowCount(const QModelIndex &parent) const
{	
	KEResourcePathItem *item = nullptr;

	if (!parent.isValid())
		item = m_PathItem;
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