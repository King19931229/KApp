#pragma once
#include "ui_KEResourceTreeWidget.h"
#include <QWidget>

class KEResourceTreeWidget : public QWidget
{
protected:
	QWidget* m_Browser;
public:
	Ui::KEResourceTreeWidget ui;
	KEResourceTreeWidget(QWidget *parent = Q_NULLPTR)
		: QWidget(parent),
		m_Browser(parent)
	{
		ui.setupUi(this);
	}
	~KEResourceTreeWidget()
	{
	}

	QSize sizeHint() const override
	{
		QSize masterSize = m_Browser->size();
		return QSize(masterSize.width() * 1 / 5, masterSize.height());
	}
};