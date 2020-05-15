#pragma once
#include <QTreeView>
#include <QMouseEvent>

class KEReflectObjectTreeView : public QTreeView
{
public:
	KEReflectObjectTreeView(QWidget *parent = Q_NULLPTR)
		: QTreeView(parent)
	{
	}

	void mouseMoveEvent(QMouseEvent *event) override
	{
		event->ignore();
	}
};