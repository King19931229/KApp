#include "KEResourceTreeView.h"
#include "Browser/KEFileSystemTreeItem.h"
#include "Browser/KEFileSystemModel.h"

void KEResourceTreeView::OnFileChange(const QString& path)
{

}

void KEResourceTreeView::OnDirectoryChange(const QString& path)
{
	KEFileSystemModel* fstModel = dynamic_cast<KEFileSystemModel*>(model());
	ASSERT_RESULT(fstModel);

	QModelIndex ret;
	if (fstModel->FindIndex(QModelIndex(), path.toStdString(), ret))
	{
		KEFileSystemTreeItem* item = nullptr;
		if (ret.isValid())
		{
			item = static_cast<KEFileSystemTreeItem*>(ret.internalPointer());
		}
		else
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

		for (const std::string& dir : m_ExpanedPaths)
		{
			if (fstModel->FindIndex(QModelIndex(), dir, ret))
			{
				expand(ret);
			}
		}
	}
}

void KEResourceTreeView::OnExpanded(const QModelIndex &index)
{
	if (index.isValid())
	{
		KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
		ASSERT_RESULT(item);

		m_ExpanedPaths.insert(item->GetFullPath());

		if (m_Watcher)
		{
			SetupWatcher(item->GetFullPath());
		}
	}
}

void KEResourceTreeView::OnCollapsed(const QModelIndex &index)
{
	if (index.isValid())
	{
		KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
		ASSERT_RESULT(item);

		m_ExpanedPaths.erase(item->GetFullPath());

		KEFileSystemModel* fstModel = dynamic_cast<KEFileSystemModel*>(model());
		ASSERT_RESULT(fstModel);

		fstModel->BeginResetModel();
		item->Clear();
		fstModel->EndResetModel();

		if (m_Watcher)
		{
			UninstallWatcher(item->GetFullPath());
		}
	}
}

void KEResourceTreeView::SetupWatcher(const std::string& path)
{
	m_Watcher->addPath(path.c_str());
}

void KEResourceTreeView::UninstallWatcher(const std::string& path)
{
	m_Watcher->removePath(path.c_str());
}

void KEResourceTreeView::setModel(QAbstractItemModel *model)
{
	QTreeView::setModel(model);

	SAFE_DELETE(m_Watcher);
	m_ExpanedPaths.clear();

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