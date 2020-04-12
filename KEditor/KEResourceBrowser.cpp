#include "KEResourceBrowser.h"
#include "KEditorConfig.h"
#include "KBase/Interface/IKFileSystem.h"

#include <assert.h>

KEFileSystemTreeModel::KEFileSystemTreeModel(QObject *parent)
	: QAbstractItemModel(parent),
	m_RootItem(nullptr)
{
}

KEFileSystemTreeModel::~KEFileSystemTreeModel()
{
	SAFE_DELETE(m_RootItem);
}

bool KEFileSystemTreeModel::Init(IKFileSystemPtr fileSys, const std::string& name)
{
	m_FileSys = fileSys;
	SAFE_DELETE(m_RootItem);

	std::string fullPath;
	m_FileSys->FullPath(".", name, fullPath);

	m_RootItem = new FileSystemTreeItem(fileSys, name, fullPath, nullptr, 0, true);
	return true;
}

QVariant KEFileSystemTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole)
	{
		FileSystemTreeItem* item = static_cast<FileSystemTreeItem*>(index.internalPointer());
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

	FileSystemTreeItem* item = nullptr;
	if (!parent.isValid())
	{
		item = m_RootItem;
	}
	else
	{
		item = static_cast<FileSystemTreeItem*>(parent.internalPointer());
	}

	FileSystemTreeItem* childItem = item->GetChild(static_cast<size_t>(row));
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

	FileSystemTreeItem* childItem = static_cast<FileSystemTreeItem*>(index.internalPointer());
	FileSystemTreeItem *parentItem = childItem->parent;

	if (parentItem == m_RootItem)
		return QModelIndex();

	return createIndex(parentItem->index, 0, parentItem);
}

int KEFileSystemTreeModel::rowCount(const QModelIndex &parent) const
{
	FileSystemTreeItem *parentItem = nullptr;

	if (!parent.isValid())
		parentItem = m_RootItem;
	else
		parentItem = static_cast<FileSystemTreeItem*>(parent.internalPointer());

	return (int)parentItem->children.size();
}

int KEFileSystemTreeModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

KEResourceBrowser::KEResourceBrowser(QWidget *parent)
	: QWidget(parent)
{
	m_MainWindow = parent;
	ui.setupUi(this);
}

KEResourceBrowser::~KEResourceBrowser()
{
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

	QObject::connect(ui.m_SystemCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(OnComboIndexChanged(int)));

	for (IKFileSystemPtr fileSys : systems)
	{
		std::string root;
		fileSys->GetRoot(root);

		QVariant userData;

		FileSystemComboData comboData;
		comboData.system = fileSys;
		userData.setValue(comboData);

		ui.m_SystemCombo->addItem(root.c_str(), userData);		
	}

	return true;
}

void KEResourceBrowser::OnComboIndexChanged(int index)
{
	QVariant userData = ui.m_SystemCombo->itemData(index);
	FileSystemComboData comboData = userData.value<FileSystemComboData>();

	IKFileSystemPtr system = comboData.system;

	const char* type = KFileSystem::FileSystemTypeToString(system->GetType());
	ui.m_SystemLabel->setText(type);

	std::string root;
	system->GetRoot(root);

	m_TreeModel.Init(system, root);

	ui.m_SystemView->setModel(nullptr);
	ui.m_SystemView->setModel(&m_TreeModel);
}

bool KEResourceBrowser::UnInit()
{
	return true;
}