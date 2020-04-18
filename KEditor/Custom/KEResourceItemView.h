#pragma once
#include <QListView>
#include <QMouseEvent>

class KEResourceItemView : public QListView
{
public:
	KEResourceItemView(QWidget *parent = Q_NULLPTR)
		: QListView(parent)
	{
	}

	void mouseMoveEvent(QMouseEvent *event) override
	{
		event->ignore();
	}
};