#include "KESceneItemWidget.h"
#include <algorithm>

KESceneItemModel::KESceneItemModel(QObject *parent)
	: QAbstractListModel(parent)
{
	m_Compare = [](const KEEntityPtr& lhs, const KEEntityPtr& rhs)->bool
	{
		return lhs->soul->GetID() < rhs->soul->GetID();
	};
}

KESceneItemModel::~KESceneItemModel()
{
}

QVariant KESceneItemModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (hasIndex(index.row(), index.column()))
	{
		if (role == Qt::DisplayRole)
		{
			KEEntity* item = static_cast<KEEntity*>(index.internalPointer());
			return QVariant(item->soul->GetName().c_str());
		}
	}

	return QVariant();
}

bool KESceneItemModel::Init()
{
	return true;
}

bool KESceneItemModel::UnInit()
{
	m_Entities.clear();
	return true;
}

void KESceneItemModel::Add(KEEntityPtr entity)
{
	auto it = std::lower_bound(m_Entities.begin(), m_Entities.end(), entity, m_Compare);
	if (it != m_Entities.end())
	{
		if ((*it)->soul->GetID() == entity->soul->GetID())
		{
			return;
		}
	}
	m_Entities.insert(it, entity);
}

void KESceneItemModel::Erase(KEEntityPtr entity)
{
	auto it = std::lower_bound(m_Entities.begin(), m_Entities.end(), entity, m_Compare);
	if (it != m_Entities.end())
	{
		if ((*it)->soul->GetID() == entity->soul->GetID())
		{
			m_Entities.erase(it);
		}
	}
}

void KESceneItemModel::Clear()
{
	m_Entities.clear();
}

bool KESceneItemModel::GetIndex(KEEntityPtr entity, size_t& index)
{
	auto it = std::lower_bound(m_Entities.begin(), m_Entities.end(), entity, m_Compare);
	if (it != m_Entities.end())
	{
		if ((*it)->soul->GetID() == entity->soul->GetID())
		{
			index = it - m_Entities.begin();
			return true;
		}
	}
	return false;
}

Qt::ItemFlags KESceneItemModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

int KESceneItemModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid())
	{
		return (int)m_Entities.size();
	}
	return 0;
}

int KESceneItemModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QModelIndex KESceneItemModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	if (row < (int)m_Entities.size())
	{
		QModelIndex index = createIndex(row, column, m_Entities[row].get());
		return index;
	}
	else
	{
		return QModelIndex();
	}
}

QModelIndex KESceneItemModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

KESceneItemWidget::KESceneItemWidget(QWidget *parent)
	: QWidget(parent),
	m_Model(nullptr)
{
	ui.setupUi(this);
	m_Model = new KESceneItemModel(this);
	ui.m_ItemView->setModel(m_Model);
}

KESceneItemWidget::~KESceneItemWidget()
{
	ui.m_ItemView->setModel(nullptr);
	SAFE_DELETE(m_Model);
}

bool KESceneItemWidget::Init()
{
	m_Model->Init();
	return true;
}

bool KESceneItemWidget::UnInit()
{
	m_Model->UnInit();
	return true;
}

void KESceneItemWidget::UpdateView()
{
	ui.m_ItemView->setModel(nullptr);
	ui.m_ItemView->setModel(m_Model);
}

void KESceneItemWidget::Add(KEEntityPtr entity)
{
	m_Model->Add(entity);
	UpdateView();
}

void KESceneItemWidget::Remove(KEEntityPtr entity)
{
	m_Model->Erase(entity);
	UpdateView();
}

void KESceneItemWidget::Clear()
{
	m_Model->Clear();
	UpdateView();
}

bool KESceneItemWidget::Select(KEEntityPtr entity)
{
	size_t index;
	if (m_Model->GetIndex(entity, index))
	{
		QModelIndex rootIndex = ui.m_ItemView->rootIndex();
		QModelIndex childIndex = rootIndex.child((int)index, 0);
		if (childIndex.isValid())
		{
			ui.m_ItemView->clicked(childIndex);
			return true;
		}
	}
	return false;
}