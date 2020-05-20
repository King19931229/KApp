#include "KEResourceBrowser.h"
#include "KEditorConfig.h"
#include "KBase/Interface/IKFileSystem.h"

#include <stack>
#include <assert.h>

KEResourceBrowser::KEResourceBrowser(QWidget *parent)
	: QMainWindow(parent),
	m_MainWindow(parent),
	m_TreeWidget(nullptr),
	m_ItemWidget(nullptr),
	m_TreeDockWidget(nullptr),
	m_ItemDockWidget(nullptr),
	m_TreeModel(KNEW KEFileSystemModel(true)),
	m_ItemModel(KNEW KEFileSystemModel(false)),
	m_PathModel(KNEW KEResourcePathModel()),
	m_Initing(true)
{
	m_TreeWidgetRatio = 3.0f / 10.0f;
	m_ItemWidgetRatio = 1.0f - m_TreeWidgetRatio;

	ui.setupUi(this);

	m_TreeWidget = KNEW KEResourceTreeWidget(this);
	m_ItemWidget = KNEW KEResourceItemWidget(this);

	m_TreeDockWidget = KNEW QDockWidget(this);
	m_TreeDockWidget->setWidget(m_TreeWidget);
	m_TreeDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

	m_ItemDockWidget = KNEW QDockWidget(this);
	m_ItemDockWidget->setWidget(m_ItemWidget);
	m_ItemDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

	addDockWidget(Qt::LeftDockWidgetArea, m_TreeDockWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_ItemDockWidget);

	m_ItemWidget->ui.m_PathView->setModel(m_PathModel);
	m_ItemWidget->ui.m_ItemView->setModel(m_ItemModel);
	m_TreeWidget->ui.m_TreeView->setModel(m_TreeModel);

	m_Initing = false;
}

KEResourceBrowser::~KEResourceBrowser()
{
	SAFE_DELETE(m_ItemWidget);
	SAFE_DELETE(m_TreeWidget);
	SAFE_DELETE(m_ItemDockWidget);
	SAFE_DELETE(m_TreeDockWidget);
	SAFE_DELETE(m_PathModel);
	SAFE_DELETE(m_ItemModel);
	SAFE_DELETE(m_TreeModel);
	ASSERT_RESULT(m_RootItemMap.empty());
}

QSize KEResourceBrowser::TreeWidgetSize() const
{
	QSize ret = QSize((int)(width() * m_TreeWidgetRatio), height());
	return ret;
}

QSize KEResourceBrowser::ItemWidgetSize() const
{
	QSize ret = QSize((int)(width() * m_ItemWidgetRatio), height());
	return ret;
}

QSize KEResourceBrowser::sizeHint() const
{
	assert(m_MainWindow);
	int width = m_MainWindow->width();
	int height = m_MainWindow->height() * 2 / 5;
	return QSize(width, height);
}

void KEResourceBrowser::resizeEvent(QResizeEvent* event)
{
	QSize treeSize = m_TreeWidget->sizeHint();
	QSize itemSize = m_ItemWidget->sizeHint();
	resizeDocks({ m_TreeDockWidget , m_ItemDockWidget }, { treeSize.width(), itemSize.width() }, Qt::Horizontal);
}

bool KEResourceBrowser::Init()
{
	UnInit();

	IKFileSystemPtr resSystem = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);

	KFileSystemPtrList systems;
	resSystem->GetAllSubFileSystem(systems);

	QObject::connect(m_TreeWidget->ui.m_SystemCombo, SIGNAL(currentIndexChanged(int)),
		this, SLOT(OnComboIndexChanged(int)));

	QObject::connect(m_TreeWidget->ui.m_TreeView, SIGNAL(clicked(QModelIndex)),
		this, SLOT(OnTreeViewClicked(QModelIndex)));

	QObject::connect(m_TreeWidget->ui.m_TreeBack, SIGNAL(clicked(bool)),
		this, SLOT(OnTreeViewBack(bool)));

	QObject::connect(m_ItemWidget->ui.m_PathView, SIGNAL(clicked(QModelIndex)),
		this , SLOT(OnPathViewClicked(QModelIndex)));

	for (IKFileSystemPtr fileSys : systems)
	{
		std::string root;
		fileSys->GetRoot(root);

		QVariant userData;

		KEFileSystemComboData comboData;
		comboData.system = fileSys;
		userData.setValue(comboData);

		m_TreeWidget->ui.m_SystemCombo->addItem(root.c_str(), userData);
	}

	return true;
}

void KEResourceBrowser::RefreshTreeView(KEFileSystemTreeItem* item)
{
	m_TreeModel->SetItem(item);
	m_TreeWidget->ui.m_TreeView->setModel(nullptr);
	m_TreeWidget->ui.m_TreeView->setModel(m_TreeModel);	
}

void KEResourceBrowser::RefreshPathView(KEFileSystemTreeItem* item)
{
	const std::string& fullPath = item->GetFullPath();

	m_PathModel->SetPath(fullPath);
	m_ItemWidget->ui.m_PathView->setModel(nullptr);
	m_ItemWidget->ui.m_PathView->setModel(m_PathModel);
}

void KEResourceBrowser::RefreshItemView(KEFileSystemTreeItem* item)
{
	m_ItemWidget->ui.m_ItemView->setModel(nullptr);
	m_ItemModel->SetItem(item);
	m_ItemWidget->ui.m_ItemView->setModel(m_ItemModel);
}

void KEResourceBrowser::OnComboIndexChanged(int index)
{
	QVariant userData = m_TreeWidget->ui.m_SystemCombo->itemData(index);
	KEFileSystemComboData comboData = userData.value<KEFileSystemComboData>();

	IKFileSystemPtr system = comboData.system;

	if (system)
	{
		const char* type = KFileSystem::FileSystemTypeToString(system->GetType());
		m_TreeWidget->ui.m_SystemLabel->setText(type);

		std::string root;
		system->GetRoot(root);

		FileSystemItem fsItem;

		auto it = m_RootItemMap.find(index);
		if (it == m_RootItemMap.end())
		{
			KEFileSystemTreeItem* tree = KNEW KEFileSystemTreeItem(system.get(), root, root, nullptr, 0, true);
			KEFileSystemTreeItem* item = KNEW KEFileSystemTreeItem(system.get(), root, root, nullptr, 0, true);
			KEFileSystemTreeItem* path = KNEW KEFileSystemTreeItem(system.get(), root, root, nullptr, 0, true);

			fsItem.tree = tree;
			fsItem.item = item;
			fsItem.path = path;

			m_RootItemMap[index] = fsItem;
		}
		else
		{
			fsItem = it->second;
		}

		m_PathModel->SetTreeItem(fsItem.path);

		RefreshTreeView(fsItem.tree);
		RefreshItemView(fsItem.item);
		RefreshPathView(fsItem.path);
	}
}

void KEResourceBrowser::OnTreeViewClicked(QModelIndex index)
{
	std::string fullPath;

	{
		KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
		fullPath = item->GetFullPath();
	}

	{
		// 把根节点拿出来
		KEFileSystemTreeItem* item = m_ItemModel->GetItem()->FindRoot();
		// 找到对应路径下的Item
		item = item->FindItem(fullPath);
		RefreshItemView(item);
	}

	{
		// 把根节点拿出来
		KEFileSystemTreeItem* item = m_PathModel->GetTreeItem()->FindRoot();
		// 找到对应路径下的Item
		item = item->FindItem(fullPath);
		RefreshPathView(item);
	}
}

void KEResourceBrowser::OnPathViewClicked(QModelIndex index)
{
	KEResourcePathItem* pathItem = static_cast<KEResourcePathItem*>(index.internalPointer());
	KEFileSystemTreeItem* treeItem = pathItem->GetTreeItem();

	while(true)
	{
		QModelIndex treeIndex = m_TreeWidget->ui.m_TreeView->currentIndex();
		QModelIndex parentIndex = treeIndex.parent();
		m_TreeWidget->ui.m_TreeView->setCurrentIndex(parentIndex);
		if (!parentIndex.isValid())
		{
			break;
		}
		KEFileSystemTreeItem* parentTreeItem = static_cast<KEFileSystemTreeItem*>(parentIndex.internalPointer());
		if (parentTreeItem == treeItem)
		{
			break;
		}
	}

	RefreshPathView(treeItem);
	RefreshItemView(treeItem);
}

void KEResourceBrowser::OnTreeViewBack(bool)
{
	KEFileSystemTreeItem* item = m_ItemModel->GetItem();
	if (item)
	{
		KEFileSystemTreeItem* parent = item->GetParent();
		if (parent)
		{
			{
				QModelIndex treeIndex = m_TreeWidget->ui.m_TreeView->currentIndex();
				QModelIndex parentIndex = treeIndex.parent();
				m_TreeWidget->ui.m_TreeView->setCurrentIndex(parentIndex);
			}

			RefreshPathView(parent);
			RefreshItemView(parent);
		}
	}
}

bool KEResourceBrowser::UnInit()
{
	m_TreeWidget->ui.m_SystemCombo->clear();
	for (auto& pair : m_RootItemMap)
	{
		FileSystemItem& item = pair.second;
		SAFE_DELETE(item.tree);
		SAFE_DELETE(item.item);
		SAFE_DELETE(item.path);
	}
	m_RootItemMap.clear();
	return true;
}