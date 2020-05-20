#include "KEResourceItemWidget.h"
#include "KEResourceBrowser.h"
#include "KEFileSystemTreeItem.h"
#include <QMenu>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>
#include "KBase/Publish/KFileTool.h"

KEResourceItemWidget::KEResourceItemWidget(QWidget *parent)
	: QWidget(parent),
	m_Browser(parent)
{
	ui.setupUi(this);
	ui.m_ItemView->setMouseTracking(false);
	ui.m_ItemView->installEventFilter(&m_Filter);

	ui.m_ItemView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.m_ItemView, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(ShowContextMenu(const QPoint&)));

	ui.m_ItemView->setDragEnabled(true);
	//ui.m_ItemView->setAcceptDrops(true);
	//ui.m_ItemView->setDragDropMode(QAbstractItemView::DragDrop);

	//ui.m_ItemView->setViewMode(QListView::IconMode);
	//ui.m_ItemView->setGridSize(QSize(100, 100));
	//ui.m_ItemView->setIconSize(QSize(80, 80));	
	ui.m_ItemView->setResizeMode(QListView::Adjust);
}

KEResourceItemWidget::~KEResourceItemWidget()
{
}

void KEResourceItemWidget::ShowContextMenu(const QPoint& pos)
{
	if (!((ui.m_ItemView->selectionModel()->selectedIndexes()).empty()))
	{
		QMenu *menu = KNEW QMenu(ui.m_ItemView);

		QAction *openFileExternalAction = menu->addAction("Open File External");
		connect(openFileExternalAction, &QAction::triggered, this, &KEResourceItemWidget::OnOpenFileExternal);

		QAction *openFileLocationAction = menu->addAction("Open File Location");
		connect(openFileLocationAction, &QAction::triggered, this, &KEResourceItemWidget::OnOpenFileLocation);

		menu->exec(QCursor::pos());
	}
}

void KEResourceItemWidget::OnOpenFileExternal()
{
	QModelIndexList selectedIndexes = ui.m_ItemView->selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		QModelIndex index = selectedIndexes[0];
		KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
		IKFileSystem* system = treeItem->GetSystem();

		if (system->GetType() == FST_NATIVE)
		{
			std::string absPath;
			if (KFileTool::AbsPath(treeItem->GetFullPath(), absPath))
			{
				bool ok = QDesktopServices::openUrl(QUrl(QString::fromLocal8Bit(absPath.c_str())));
				if (!ok)
				{
					std::string failureMessage = std::string("File ") + absPath + " open failure";
					QMessageBox::critical(this, "File open failure", QString::fromLocal8Bit(failureMessage.c_str()));
				}
			}
		}
	}
}

void KEResourceItemWidget::OnOpenFileLocation()
{
	QModelIndexList selectedIndexes = ui.m_ItemView->selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		QModelIndex index = selectedIndexes[0];
		KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
		IKFileSystem* system = treeItem->GetSystem();

		if (system->GetType() == FST_NATIVE)
		{
			std::string absPath;
			if (KFileTool::AbsPath(treeItem->GetFullPath(), absPath))
			{
#ifdef _WIN32
				QProcess process;
				QString filePath = QString::fromLocal8Bit(absPath.c_str());
				filePath.replace("/", "\\");
				QString cmd = QString("explorer.exe /select,\"%1\"").arg(filePath);
				bool ok = process.startDetached(cmd);
#else
				KFileTool::ParentFolder(absPath, absPath);
				bool ok = QDesktopServices::openUrl(QUrl(QString::fromLocal8Bit(absPath.c_str())));
#endif
				if (!ok)
				{
					std::string failureMessage = std::string("File ") + absPath + " location open failure";
					QMessageBox::critical(this, "File location open failure", QString::fromLocal8Bit(failureMessage.c_str()));
				}
			}
		}
	}
}

QSize KEResourceItemWidget::sizeHint() const
{
	KEResourceBrowser* master = (KEResourceBrowser*)m_Browser;
	return master->ItemWidgetSize();
}

void KEResourceItemWidget::resizeEvent(QResizeEvent* event)
{
}