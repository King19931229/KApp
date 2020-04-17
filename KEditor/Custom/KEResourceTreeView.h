#pragma once
#include <QTreeView>
#include <QMouseEvent>
class KEResourceTreeView : public QTreeView
{
public:
	KEResourceTreeView(QWidget *parent = Q_NULLPTR)
		: QTreeView(parent)
	{
	}

	void focusInEvent(QFocusEvent *event) override
	{
		event->ignore();
	}	
};