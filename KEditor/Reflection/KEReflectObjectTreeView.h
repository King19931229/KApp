#pragma once
#include <QTreeView>
#include <QMouseEvent>

class KEReflectObjectTreeView : public QTreeView
{
protected:
	void SetupIndexWidget(QModelIndex index)
	{
		while (index.isValid())
		{
			KEReflectionObjectItem* item = static_cast<KEReflectionObjectItem*>(index.internalPointer());
			if (item && item->GetType() == KEReflectionObjectItem::OBJECT_MEMBER_TYPE_PROPERTY)
			{
				KEPropertyBaseView::BasePtr view = item->GetPropertyView();
				if (view)
				{
					// 这里考虑的原因比较复杂 总之就是不能够再调用listener 否则对多选属性面板与效率上都是问题
					auto guard = view->CreateListenerMuteGuard();

					// Qt这套setIndexWidget规则一定会持有对象
					// 导致这套持有机制非常恶心
					QWidget* wholeWidget = KNEW QWidget();

					QLabel* label = KNEW QLabel(wholeWidget);
					label->setText(item->GetName().c_str());

					QWidget* widget = view->AllocWidget();
					widget->setParent(wholeWidget);
					assert(widget);

					QHBoxLayout* layout = KNEW QHBoxLayout(wholeWidget);
					layout->addWidget(label);
					layout->addWidget(widget);

					wholeWidget->setLayout(layout);

					setIndexWidget(index, wholeWidget);
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
			SetupIndexWidget(model->index(0, 0));
		}
	}
};