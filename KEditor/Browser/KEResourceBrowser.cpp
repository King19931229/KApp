#include "KEResourceBrowser.h"
#include "KEditorConfig.h"
#include "KBase/Interface/IKFileSystem.h"

#include <assert.h>

KEResourceBrowser::KEResourceBrowser(QWidget *parent)
	: QMainWindow(parent),
	m_MainWindow(parent),
	m_RootItem(nullptr),
	m_TreeWidget(nullptr),
	m_ItemWidget(nullptr),
	m_TreeDockWidget(nullptr),
	m_ItemDockWidget(nullptr),
	m_TreeModel(new KEFileSystemModel(true)),
	m_ItemModel(new KEFileSystemModel(false)),
	m_Initing(true)
{
	m_TreeWidgetRatio = 3.0f / 10.0f;
	m_ItemWidgetRatio = 1.0f - m_TreeWidgetRatio;

	ui.setupUi(this);

	m_TreeWidget = new KEResourceTreeWidget(this);
	m_ItemWidget = new KEResourceItemWidget(this);

	m_TreeDockWidget = new QDockWidget(this);
	m_TreeDockWidget->setWidget(m_TreeWidget);
	m_TreeDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

	m_ItemDockWidget = new QDockWidget(this);
	m_ItemDockWidget->setWidget(m_ItemWidget);
	m_ItemDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

	addDockWidget(Qt::LeftDockWidgetArea, m_TreeDockWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_ItemDockWidget);

	m_Initing = false;
}

KEResourceBrowser::~KEResourceBrowser()
{
	SAFE_DELETE(m_ItemWidget);
	SAFE_DELETE(m_TreeWidget);
	SAFE_DELETE(m_ItemDockWidget);
	SAFE_DELETE(m_TreeDockWidget);
	SAFE_DELETE(m_ItemModel);
	SAFE_DELETE(m_TreeModel);
	SAFE_DELETE(m_RootItem);
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
	IKFileSystemPtr resSystem = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);

	KFileSystemPtrList systems;
	resSystem->GetAllSubFileSystem(systems);

	QObject::connect(m_TreeWidget->ui.m_SystemCombo, SIGNAL(currentIndexChanged(int)),
		this, SLOT(OnComboIndexChanged(int)));

	QObject::connect(m_TreeWidget->ui.m_TreeView, SIGNAL(pressed(QModelIndex)),
		this, SLOT(OnTreeViewClicked(QModelIndex)));

	QObject::connect(m_TreeWidget->ui.m_TreeBack, SIGNAL(clicked(bool)),
		this, SLOT(OnTreeViewBack(bool)));

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

void KEResourceBrowser::RefreshTreeView()
{
	m_TreeWidget->ui.m_TreeView->setModel(nullptr);
	m_TreeWidget->ui.m_TreeView->setModel(m_TreeModel);
}

void KEResourceBrowser::RefreshItemView()
{
	m_ItemWidget->ui.m_ItemView->setModel(nullptr);
	m_ItemWidget->ui.m_ItemView->setModel(m_ItemModel);
}

void KEResourceBrowser::RefreshView()
{	
	RefreshTreeView();
	RefreshItemView();
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

		std::string fullPath;
		system->FullPath(".", root, fullPath);

		SAFE_DELETE(m_RootItem);
		m_RootItem = new KEFileSystemTreeItem(system.get(), root, fullPath, nullptr, 0, true);

		m_TreeModel->SetItem(m_RootItem);
		m_ItemModel->SetItem(m_RootItem);
		RefreshView();
	}
}

void KEResourceBrowser::OnTreeViewClicked(QModelIndex index)
{
	KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
	m_ItemModel->SetItem(item);
	RefreshItemView();
}

void KEResourceBrowser::OnTreeViewBack(bool)
{
	KEFileSystemTreeItem* item = m_ItemModel->GetItem();
	if (item)
	{
		KEFileSystemTreeItem* parent = item->GetParent();
		if (parent)
		{
			m_ItemModel->SetItem(parent);
			RefreshItemView();
		}
	}
}

bool KEResourceBrowser::UnInit()
{
	m_TreeWidget->ui.m_SystemCombo->clear();
	return true;
}