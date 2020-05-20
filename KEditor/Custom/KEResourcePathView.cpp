#include "KEResourcePathView.h"
#include "Browser/KEResourcePathModel.h"
#include "Browser/KEResourcePathItem.h"

void KEResourcePathView::OnFileChange(const QString& path)
{
}

void KEResourcePathView::OnDirectoryChange(const QString& path)
{
	KEResourcePathModel* pathModel = dynamic_cast<KEResourcePathModel*>(model());
	ASSERT_RESULT(pathModel);

	QModelIndex ret;
	if (pathModel->FindIndex(QModelIndex(), path.toStdString(), ret))
	{
		KEResourcePathItem* item = nullptr;
		if (ret.isValid())
		{
			item = static_cast<KEResourcePathItem*>(ret.internalPointer());
		}
		else
		{
			item = pathModel->GetPathItem();
		}
		ASSERT_RESULT(item);

		KEFileSystemTreeItem* treeItem = pathModel->GetTreeItem();
		ASSERT_RESULT(treeItem);

		pathModel->BeginResetModel();
		treeItem->Clear();
		item->Clear();
		pathModel->SetPath(pathModel->GetPath());
		pathModel->EndResetModel();

		QModelIndex index = pathModel->index(0, 0);
		while (index.isValid())
		{
			setCurrentIndex(index);
			index = pathModel->index(0, 0, index);
		}
	}
}

void KEResourcePathView::SetupWatcher(const std::string& path)
{
	m_Watcher->addPath(QString::fromLocal8Bit(path.c_str()));
}

void KEResourcePathView::setModel(QAbstractItemModel *model)
{
	QColumnView::setModel(model);

	SAFE_DELETE(m_Watcher);

	KEResourcePathModel* pathModel = dynamic_cast<KEResourcePathModel*>(model);
	if (pathModel)
	{
		KEResourcePathItem* item = pathModel->GetPathItem();

		if (item && item->GetTreeItem())
		{
			KEFileSystemTreeItem* treeItem = item->GetTreeItem();
			if (treeItem && treeItem->GetSystem() && treeItem->GetSystem()->GetType() == FST_NATIVE)
			{
				m_Watcher = KNEW QFileSystemWatcher(this);

				connect(m_Watcher, &QFileSystemWatcher::fileChanged, this, &KEResourcePathView::OnFileChange);
				connect(m_Watcher, &QFileSystemWatcher::directoryChanged, this, &KEResourcePathView::OnDirectoryChange);

				while (item)
				{
					SetupWatcher(item->GetFullPath());
					item = item->GetChild();
				}
			}
		}
	}

	if (pathModel)
	{
		QModelIndex index = pathModel->index(0, 0);
		while (index.isValid())
		{
			setCurrentIndex(index);
			index = pathModel->index(0, 0, index);
		}
	}
}