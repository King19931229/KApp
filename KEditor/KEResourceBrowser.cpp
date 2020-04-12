#include "KEResourceBrowser.h"
#include "KEditorConfig.h"
#include "KBase/Interface/IKFileSystem.h"

#include <assert.h>

KEFileSystemTreeModel::KEFileSystemTreeModel(QObject *parent)
	: QAbstractItemModel(parent),
	m_Item(nullptr)
{
}

KEFileSystemTreeModel::~KEFileSystemTreeModel()
{
}

void KEFileSystemTreeModel::SetItem(KEFileSystemTreeItem* item)
{
	m_Item = item;
}

KEFileSystemTreeItem* KEFileSystemTreeModel::GetItem()
{
	return m_Item;
}

QVariant KEFileSystemTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole)
	{
		KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
		return QVariant(item->name.c_str());
	}

	return QVariant();
}

Qt::ItemFlags KEFileSystemTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant KEFileSystemTreeModel::headerData(int section, Qt::Orientation orientation,
	int role) const
{
	return QVariant();
}

QModelIndex KEFileSystemTreeModel::index(int row, int column,
	const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	KEFileSystemTreeItem* item = nullptr;
	if (!parent.isValid())
	{
		item = m_Item;
	}
	else
	{
		item = static_cast<KEFileSystemTreeItem*>(parent.internalPointer());
	}

	KEFileSystemTreeItem* childItem = item->GetChild(static_cast<size_t>(row));
	if (childItem)
	{
		return createIndex(row, column, childItem);
	}
	else
	{
		return QModelIndex();
	}
}

QModelIndex KEFileSystemTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return QModelIndex();
	}

	KEFileSystemTreeItem* childItem = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
	KEFileSystemTreeItem *parentItem = childItem->parent;

	if (parentItem == m_Item)
		return QModelIndex();

	return createIndex(parentItem->index, 0, parentItem);
}

int KEFileSystemTreeModel::rowCount(const QModelIndex &parent) const
{
	KEFileSystemTreeItem *parentItem = nullptr;

	if (!parent.isValid())
		parentItem = m_Item;
	else
		parentItem = static_cast<KEFileSystemTreeItem*>(parent.internalPointer());

	return (int)parentItem->children.size();
}

int KEFileSystemTreeModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

KEResourceBrowser::KEResourceBrowser(QWidget *parent)
	: QWidget(parent),
	m_RootItem(nullptr)
{
	m_MainWindow = parent;
	ui.setupUi(this);
}

KEResourceBrowser::~KEResourceBrowser()
{
	SAFE_DELETE(m_RootItem);
}

QSize KEResourceBrowser::sizeHint() const
{
	assert(m_MainWindow);
	int width = m_MainWindow->width();
	int height = m_MainWindow->height() * 2 / 5;
	return QSize(width, height);
}

bool KEResourceBrowser::Init()
{
	IKFileSystemPtr resSystem = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);

	KFileSystemPtrList systems;
	resSystem->GetAllSubFileSystem(systems);

	QObject::connect(ui.m_SystemCombo, SIGNAL(currentIndexChanged(int)),
		this, SLOT(OnComboIndexChanged(int)));

	QObject::connect(ui.m_TreeView, SIGNAL(doubleClicked(QModelIndex)),
		this, SLOT(OnTreeViewClicked(QModelIndex)));

	QObject::connect(ui.m_TreeBack, SIGNAL(clicked(bool)),
		this, SLOT(OnTreeViewBack(bool)));

	for (IKFileSystemPtr fileSys : systems)
	{
		std::string root;
		fileSys->GetRoot(root);

		QVariant userData;

		KEFileSystemComboData comboData;
		comboData.system = fileSys;
		userData.setValue(comboData);

		ui.m_SystemCombo->addItem(root.c_str(), userData);		
	}

	return true;
}

void KEResourceBrowser::OnComboIndexChanged(int index)
{
	QVariant userData = ui.m_SystemCombo->itemData(index);
	KEFileSystemComboData comboData = userData.value<KEFileSystemComboData>();

	IKFileSystemPtr system = comboData.system;

	const char* type = KFileSystem::FileSystemTypeToString(system->GetType());
	ui.m_SystemLabel->setText(type);

	std::string root;
	system->GetRoot(root);

	std::string fullPath;
	system->FullPath(".", root, fullPath);

	SAFE_DELETE(m_RootItem);
	m_RootItem = new KEFileSystemTreeItem(system, root, fullPath, nullptr, 0, true);

	m_TreeModel.SetItem(m_RootItem);
	ui.m_TreeView->setModel(nullptr);
	ui.m_TreeView->setModel(&m_TreeModel);
}

void KEResourceBrowser::OnTreeViewClicked(QModelIndex index)
{
	KEFileSystemTreeItem* item = static_cast<KEFileSystemTreeItem*>(index.internalPointer());
	m_TreeModel.SetItem(item);
	ui.m_TreeView->setModel(nullptr);
	ui.m_TreeView->setModel(&m_TreeModel);
}

void KEResourceBrowser::OnTreeViewBack(bool)
{
	KEFileSystemTreeItem* item = m_TreeModel.GetItem();
	if (item)
	{
		KEFileSystemTreeItem* parent = item->parent;
		if (parent)
		{
			m_TreeModel.SetItem(parent);
			ui.m_TreeView->setModel(nullptr);
			ui.m_TreeView->setModel(&m_TreeModel);
		}
	}
}

bool KEResourceBrowser::UnInit()
{
	ui.m_SystemCombo->clear();
	return true;
}