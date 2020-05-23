#include "KEResourceTreeWidget.h"
#include "KEResourceBrowser.h"
#include "KEFileSystemTreeItem.h"
#include <QMenu>
#include <QDesktopServices>
#include <QMessageBox>
#include "KBase/Publish/KFileTool.h"

KEResourceTreeWidget::KEResourceTreeWidget(QWidget *parent)
	: QWidget(parent),
	m_Browser(parent)
{
	ui.setupUi(this);
	ui.m_TreeView->setMouseTracking(false);
	ui.m_TreeView->installEventFilter(&m_Filter);

	ui.m_TreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.m_TreeView, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(ShowContextMenu(const QPoint&)));
}

KEResourceTreeWidget::~KEResourceTreeWidget()
{
}

void KEResourceTreeWidget::ShowContextMenu(const QPoint& pos)
{
	if (!((ui.m_TreeView->selectionModel()->selectedIndexes()).empty()))
	{
		QMenu *menu = KNEW QMenu(ui.m_TreeView);

		QAction *openFileExternalAction = menu->addAction("Open Folder Location");
		connect(openFileExternalAction, &QAction::triggered, this, &KEResourceTreeWidget::OnOpenFolderLocation);

		menu->exec(QCursor::pos());
	}
}

void KEResourceTreeWidget::OnOpenFolderLocation()
{
	QModelIndexList selectedIndexes = ui.m_TreeView->selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		QModelIndex index = selectedIndexes[0];
		KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
		IKFileSystem* system = treeItem->GetSystem();

		if (system->GetType() == FST_NATIVE)
		{
			std::string absPath = treeItem->GetSystemFullPath();
			bool ok = QDesktopServices::openUrl(QUrl(QString::fromLocal8Bit(absPath.c_str()), QUrl::TolerantMode));
			if (!ok)
			{
				std::string failureMessage = std::string("Folder ") + absPath + " open failure";
				QMessageBox::critical(this, "Folder open failure", QString::fromLocal8Bit(failureMessage.c_str()));
			}
		}
	}
}

QSize KEResourceTreeWidget::sizeHint() const
{
	KEResourceBrowser* master = (KEResourceBrowser*)m_Browser;
	return master->TreeWidgetSize();
}

void KEResourceTreeWidget::resizeEvent(QResizeEvent* event)
{
}