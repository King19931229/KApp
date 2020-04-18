#pragma once
#include <QColumnView>

class KEResourcePathView : public QColumnView
{
public:
	KEResourcePathView(QWidget *parent = Q_NULLPTR)
		: QColumnView(parent)
	{
	}
};