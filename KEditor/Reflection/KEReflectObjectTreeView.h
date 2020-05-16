#pragma once
#include <QTreeView>
#include <QMouseEvent>

#include "KEReflectObjectTreeModel.h"

class KEReflectObjectTreeView : public QTreeView
{
protected:
	std::vector<KEPropertyBaseView::BasePtr> m_ViewHolders;

	void SetupIndexWidget(QModelIndex index)
	{
		while (index.isValid())
		{
			KEReflectionObjectItem* item = static_cast<KEReflectionObjectItem*>(index.internalPointer());
			if (item && item->GetType() == KEReflectionObjectItem::OBJECT_MEMBER_TYPE_PROPERTY)
			{
				KEPropertyBaseView::BasePtr view = item->CreateView();
				if (view)
				{				
					// Qt这套规则setIndexWidget一定会持有对象
					// 导致这套持有机制非常恶心
					m_ViewHolders.push_back(view);
					QWidget* widget = view->MoveWidget();
					assert(widget);
					setIndexWidget(index, widget);
				}
			}

			QModelIndex childIndex = index.child(0, 0);
			while (childIndex.isValid())
			{
				SetupIndexWidget(childIndex);
				childIndex = childIndex.sibling(childIndex.row() + 1, 0);
			}

			index = index.sibling(index.row() + 1, 0);
			SetupIndexWidget(index);
		}
	}
public:
	KEReflectObjectTreeView(QWidget *parent = Q_NULLPTR)
		: QTreeView(parent)
	{
	}

	~KEReflectObjectTreeView()
	{
	}

	void mouseMoveEvent(QMouseEvent *event) override
	{
		event->ignore();
	}

	void setModel(QAbstractItemModel *model) override
	{
		QTreeView::setModel(model);

		if (model)
		{
			m_ViewHolders.clear();
			SetupIndexWidget(model->index(0, 0));
		}
	}
};