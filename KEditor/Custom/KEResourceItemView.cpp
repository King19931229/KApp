#include "KEResourceItemView.h"
#include "KEditorGlobal.h"
#include "Browser/KEFileSystemTreeItem.h"
#include "Browser/KEFileSystemModel.h"
#include "Window/KEMaterialEditWindow.h"
#include "KBase/Publish/KFileTool.h"

KEResourceItemView::KEResourceItemView(QWidget *parent)
	: QListView(parent),
	m_Watcher(nullptr),
	m_RootItem(nullptr)
{
	setAcceptDrops(true);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	// 连接信号槽的时候别带变量名
	connect(this, SIGNAL(doubleClicked(const QModelIndex &)),
		this, SLOT(OnOpenItemEditor(const QModelIndex &)));
}

KEResourceItemView::~KEResourceItemView()
{
	SAFE_DELETE(m_Watcher);
}

void KEResourceItemView::OnOpenItemEditor(const QModelIndex &index)
{
	KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
	IKFileSystem* system = treeItem->GetSystem();

	if (system->GetType() == FST_NATIVE)
	{
		std::string absPath = treeItem->GetSystemFullPath();
		if (KStringUtil::EndsWith(absPath, ".mtl"))
		{
			KEMaterialEditWindow* materialWindow = KNEW KEMaterialEditWindow(KEditorGlobal::MainWindow);
			materialWindow->SetEditTarget(system, treeItem->GetFullPath());
			materialWindow->show();
		}
	}
}

void KEResourceItemView::OnFileChange(const QString& path)
{
}

void KEResourceItemView::OnDirectoryChange(const QString& path)
{
	KEFileSystemModel* fstModel = dynamic_cast<KEFileSystemModel*>(model());
	ASSERT_RESULT(fstModel);

	fstModel->BeginResetModel();

	KEFileSystemTreeItem* changeItem = m_RootItem->FindItem(path.toStdString());
	if (changeItem)
	{
		changeItem->Clear();
	}

	KEFileSystemTreeItem* item = m_RootItem->FindItem(m_FullPath);
	if (item)
	{
		fstModel->SetItem(item);
	}
	else
	{
		fstModel->SetItem(m_RootItem);
	}

	fstModel->EndResetModel();
}

void KEResourceItemView::SetupWatcher(const std::string& path)
{
	m_Watcher->addPath(QString::fromLocal8Bit(path.c_str()));
}

void KEResourceItemView::UninstallWatcher(const std::string& path)
{
	m_Watcher->removePath(QString::fromLocal8Bit(path.c_str()));
}

void KEResourceItemView::mouseMoveEvent(QMouseEvent *event)
{
	QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		QModelIndex index = selectedIndexes[0];
		KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
		QMimeData* mimeData = KNEW QMimeData();

		const std::string& fullPath = treeItem->GetFullPath();

		KEResourceItemDropData* dropData = KNEW KEResourceItemDropData();
		dropData->item = treeItem;

		if (KStringUtil::EndsWith(fullPath, ".mtl"))
		{
			dropData->dropType = KEResourceItemDropData::DT_MATERIAL;
		}
		else
		{
			dropData->dropType = KEResourceItemDropData::DT_MODEL;
		}

		QDrag* drag = KNEW QDrag(this);
		drag->setMimeData(mimeData);

		mimeData->setText(fullPath.c_str());
		mimeData->setUrls({ QUrl(fullPath.c_str()) });
		mimeData->setUserData(0, dropData);

		Qt::DropAction action = drag->exec(Qt::MoveAction);
		event->ignore();
	}
	QListView::mouseMoveEvent(event);
}

void KEResourceItemView::setModel(QAbstractItemModel *model)
{
	QListView::setModel(model);

	SAFE_DELETE(m_Watcher);

	KEFileSystemModel* fstModel = dynamic_cast<KEFileSystemModel*>(model);
	if (fstModel)
	{
		m_Watcher = KNEW QFileSystemWatcher(this);

		connect(m_Watcher, &QFileSystemWatcher::fileChanged, this, &KEResourceItemView::OnFileChange);
		connect(m_Watcher, &QFileSystemWatcher::directoryChanged, this, &KEResourceItemView::OnDirectoryChange);

		KEFileSystemTreeItem* item = fstModel->GetItem();

		if (item)
		{
			m_RootItem = item->FindRoot();
			m_FullPath = item->GetSystemFullPath();
		}
		else
		{
			m_RootItem = nullptr;
			m_FullPath = "";
		}

		if (item && item->GetSystem() && item->GetSystem()->GetType() == FST_NATIVE)
		{
			while (item)
			{
				SetupWatcher(item->GetSystemFullPath());
				item = item->GetParent();
			}
		}
	}
}

void KEResourceItemView::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasFormat("text/uri-list"))
		event->acceptProposedAction();
}

void KEResourceItemView::dragMoveEvent(QDragMoveEvent *event)
{
	event->setDropAction(Qt::MoveAction);
	event->accept();
}

void KEResourceItemView::dragLeaveEvent(QDragLeaveEvent *event)
{
}

void KEResourceItemView::dropEvent(QDropEvent *event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
		return;

	for (const QUrl& url : urls)
	{
		std::string fullSrcPath = url.toLocalFile().toStdString();

		std::string fileName;
		std::string folderName;
		std::string fullDestPath;

		if (KFileTool::IsFile(fullSrcPath))
		{
			if (KFileTool::FileName(fullSrcPath, fileName) &&
				KFileTool::PathJoin(m_FullPath, fileName, fullDestPath))
			{
				KFileTool::CopyFile(fullSrcPath, fullDestPath);
			}
		}
		else
		{
			if (KFileTool::FolderName(fullSrcPath, folderName) &&
				KFileTool::PathJoin(m_FullPath, folderName, fullDestPath))
			{
				KFileTool::CopyFolder(fullSrcPath, fullDestPath);
			}
		}
	}
}