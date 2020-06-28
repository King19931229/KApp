#include "KEResourceItemWidget.h"
#include "KEResourceBrowser.h"
#include "KEFileSystemTreeItem.h"
#include "KEditorGlobal.h"
#include "Dialog/KEAssetConvertDialog.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KStringUtil.h"
#include <QMenu>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>
#include <tuple>

KEResourceItemWidget::KEResourceItemWidget(QWidget *parent)
	: QWidget(parent),
	m_Browser(parent)
{
	ui.setupUi(this);
	ui.m_ItemView->installEventFilter(&m_Filter);
	ui.m_ItemView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.m_ItemView, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(ShowContextMenu(const QPoint&)));

	//ui.m_ItemView->setViewMode(QListView::IconMode);
	//ui.m_ItemView->setGridSize(QSize(100, 100));
	//ui.m_ItemView->setIconSize(QSize(80, 80));
	//ui.m_ItemView->setResizeMode(QListView::Adjust);
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

		QAction *openEditorAction = menu->addAction("Open Editor");
		connect(openEditorAction, &QAction::triggered, this, &KEResourceItemWidget::OnOpenItemEditor);

		QAction *deleteFileAction = menu->addAction("Delete File");
		connect(deleteFileAction, &QAction::triggered, this, &KEResourceItemWidget::OnDeleteFile);

		QAction *convertIntoMeshAction = menu->addAction("Convert Into Mesh");
		connect(convertIntoMeshAction, &QAction::triggered, this, &KEResourceItemWidget::ConvertIntoMesh);

		menu->exec(QCursor::pos());
	}
}

void KEResourceItemWidget::OnOpenItemEditor()
{
	QModelIndexList selectedIndexes = ui.m_ItemView->selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		for (QModelIndex& index : selectedIndexes)
		{
			ui.m_ItemView->OnOpenItemEditor(index);
		}
	}
}

void KEResourceItemWidget::OnOpenFileExternal()
{
	QModelIndexList selectedIndexes = ui.m_ItemView->selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		for (QModelIndex& index : selectedIndexes)
		{
			KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
			IKFileSystem* system = treeItem->GetSystem();

			if (system->GetType() == FST_NATIVE)
			{
				std::string absPath = treeItem->GetSystemFullPath();
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
		for (QModelIndex& index : selectedIndexes)
		{
			KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
			IKFileSystem* system = treeItem->GetSystem();

			if (system->GetType() == FST_NATIVE)
			{
				std::string absPath = treeItem->GetSystemFullPath();
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

void KEResourceItemWidget::OnDeleteFile()
{
	QModelIndexList selectedIndexes = ui.m_ItemView->selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		std::vector<std::tuple<IKFileSystem*, std::string>> deleteItems;
		deleteItems.reserve(selectedIndexes.size());

		for (QModelIndex& index : selectedIndexes)
		{
			KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
			deleteItems.push_back(std::make_tuple(treeItem->GetSystem(), treeItem->GetFullPath()));
		}

		for (const std::tuple<IKFileSystem*, std::string>& item : deleteItems)
		{
			IKFileSystem* system = std::get<0>(item);
			const std::string& path = std::get<1>(item);
			system->RemoveFile(path);
		}
	}
}

void KEResourceItemWidget::ConvertIntoMesh()
{
	QModelIndexList selectedIndexes = ui.m_ItemView->selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		if (selectedIndexes.size() > 1)
		{
			QMessageBox::critical(this, "Don't select more than one item", "Don't select more than one item");
			return;
		}

		QModelIndex index = selectedIndexes[0];
		KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
		IKFileSystem* system = treeItem->GetSystem();

		if (system->GetType() == FST_NATIVE)
		{
			std::string assetPath = treeItem->GetSystemFullPath();
			std::string meshPath;
			if (KFileTool::ReplaceExt(assetPath, ".mesh", meshPath))
			{
				KEAssetConvertDialog dialog;
				dialog.SetAssetPath(assetPath);
				dialog.SetMeshPath(meshPath);
				dialog.SetFileSystem(system);
				if (dialog.exec())
				{
					if (dialog.Process())
					{
						QMessageBox::information(this, "Convert mesh success", "Convert mesh success");
					}
					else
					{
						QMessageBox::critical(this, "Convert mesh failure", "Convert mesh failure");
					}
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

void KEResourceItemWidget::mousePressEvent(QMouseEvent* event)
{
	OnOpenItemEditor();
}