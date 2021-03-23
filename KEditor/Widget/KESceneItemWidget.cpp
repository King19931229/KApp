#include "KESceneItemWidget.h"
#include "KEditorGlobal.h"
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

bool KESceneItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
	{
		return false;
	}

	if (hasIndex(index.row(), index.column()))
	{
		if (role == Qt::EditRole)
		{
			QString newName = value.toString();
			if (!newName.isEmpty())
			{
				KEEntity* item = static_cast<KEEntity*>(index.internalPointer());
				item->soul->SetName(newName.toStdString());

				KESceneItemWidget* sceneWidget = (KESceneItemWidget*)QObject::parent();
				sceneWidget->UpdateView();
			}
			return true;
		}
	}

	return QAbstractListModel::setData(index, value, role);
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

	beginResetModel();
	endResetModel();
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

	// https://wiki.qt.io/How_to_use_QAbstractListModel
	beginResetModel();
	endResetModel();
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

	m_Model = KNEW KESceneItemModel(this);

	m_ProxyModel = KNEW QSortFilterProxyModel(this);
	m_ProxyModel->setSourceModel(m_Model);

	m_ProxyModel->setDynamicSortFilter(true);
	m_ProxyModel->setFilterRole(Qt::DisplayRole);
	m_ProxyModel->setSortRole(Qt::DisplayRole);

	ui.m_ItemView->setModel(m_ProxyModel);

	// SIGNAL SLOT 可以不带const修饰符与引用修饰符 但是最好都带上 或者都不带上
	// 不要只带const修饰符或者引用修饰符而不带其中另一个
	// 尤其不要只带引用而缺少const修饰符
	// 这里坑很多 最好用非宏版本连接信号槽
	QObject::connect(ui.m_ItemView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
		this, SLOT(OnSelectionChanged(const QItemSelection &, const QItemSelection &)));

	QObject::connect(ui.m_SearchEdit, SIGNAL(textChanged(const QString &)), this, SLOT(OnSearchTextChanged(const QString &)));
}

KESceneItemWidget::~KESceneItemWidget()
{
	ui.m_ItemView->setModel(nullptr);
	m_ProxyModel->setSourceModel(nullptr);
	SAFE_DELETE(m_ProxyModel);
	SAFE_DELETE(m_Model);
}

void KESceneItemWidget::OnSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	QItemSelection sourceSelected = selected.empty() ? QItemSelection() : m_ProxyModel->mapSelectionToSource(selected);
	QItemSelection sourceDeselected = deselected.empty() ? QItemSelection() : m_ProxyModel->mapSelectionToSource(deselected);

	for (QModelIndex& index : sourceDeselected.indexes())
	{
		KEEntity* entity = (KEEntity*)index.internalPointer();
		KEEntityPtr entityPtr = KEditorGlobal::EntityManipulator.GetEntity(entity->soul->GetID());
		KEditorGlobal::EntitySelector.Remove(entityPtr);
	}

	for (QModelIndex& index : sourceSelected.indexes())
	{
		KEEntity* entity = (KEEntity*)index.internalPointer();
		KEEntityPtr entityPtr = KEditorGlobal::EntityManipulator.GetEntity(entity->soul->GetID());
		KEditorGlobal::EntitySelector.Add(entityPtr);
	}
}

void KESceneItemWidget::OnSearchTextChanged(const QString& newText)
{
	m_ProxyModel->setFilterRegExp(newText);
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
	ui.m_ItemView->setModel(m_ProxyModel);	
	m_ProxyModel->setSourceModel(m_Model);
	m_ProxyModel->sort(0);

	// 重设model selectionModel也被更替 需要重新绑定信号槽
	QObject::connect(ui.m_ItemView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
		this, SLOT(OnSelectionChanged(const QItemSelection &, const QItemSelection &)));
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

bool KESceneItemWidget::Select(KEEntityPtr entity, bool select)
{
	size_t index;
	if (m_Model->GetIndex(entity, index))
	{
		QModelIndex modeIndex = m_Model->index((int)index, 0);
		if (modeIndex.isValid())
		{
			QModelIndex proxyIndex = m_ProxyModel->mapFromSource(modeIndex);
			if (proxyIndex.isValid())
			{
				ui.m_ItemView->selectionModel()->select(proxyIndex,
					select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
			}
		}
	}
	return false;
}

bool KESceneItemWidget::ClearSelection()
{
	ui.m_ItemView->clearSelection();
	return true;
}