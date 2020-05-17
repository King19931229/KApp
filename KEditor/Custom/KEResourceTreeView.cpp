#include "KEResourceTreeView.h"
#include "Browser/KEFileSystemTreeItem.h"
#include "Browser/KEFileSystemModel.h"

void KEResourceTreeView::OnFileChange(const QString& path)
{

}

void KEResourceTreeView::OnDirectoryChange(const QString& path)
{
	QModelIndex ret;
	if (FindIndex(QModelIndex(), path.toStdString(), ret))
	{
		KEFileSystemModel* fstModel = dynamic_cast<KEFileSystemModel*>(model());
		KEFileSystemTreeItem* item = nullptr;

		ASSERT_RESULT(fstModel);

		if (ret.isValid())
		{
			item = static_cast<KEFileSystemTreeItem*>(ret.internalPointer());
		}
		else if (fstModel)
		{
			item = fstModel->GetItem();
		}

		ASSERT_RESULT(item);

		// https://forum.qt.io/topic/52590/deleting-all-items-in-a-tree-view/4
		if (item)
		{
			fstModel->BeginResetModel();
			item->Clear();
			fstModel->EndResetModel();
		}
	}
}

bool KEResourceTreeView::FindIndex(QModelIndex parent, const std::string& path, QModelIndex& ret)
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
	else // Root
	{
		KEFileSystemModel* fstModel = dynamic_cast<KEFileSystemModel*>(model());
		if (fstModel)
		{
			KEFileSystemTreeItem* item = fstModel->GetItem();
			if (item->GetFullPath() == path)
			{
				ret = parent;
				return true;
			}
		}
	}

	QModelIndex child = parent.child(0, 0);
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

void KEResourceTreeView::SetupWatcher(const std::string& path)
{
	ASSERT_RESULT(m_Watcher->addPath(path.c_str()));
}

void KEResourceTreeView::setModel(QAbstractItemModel *model)
{
	QTreeView::setModel(model);

	SAFE_DELETE(m_Watcher);

	KEFileSystemModel* fstModel = dynamic_cast<KEFileSystemModel*>(model);
	if (fstModel)
	{
		m_Watcher = KNEW QFileSystemWatcher(this);

		connect(m_Watcher, &QFileSystemWatcher::fileChanged, this, &KEResourceTreeView::OnFileChange);
		connect(m_Watcher, &QFileSystemWatcher::directoryChanged, this, &KEResourceTreeView::OnDirectoryChange);

		KEFileSystemTreeItem* item = fstModel->GetItem();
		if (item && item->GetSystem() && item->GetSystem()->GetType() == FST_NATIVE)
		{
			SetupWatcher(item->GetFullPath());
		}
	}
}