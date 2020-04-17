#pragma once
#include "ui_KEResourceItemWidget.h"

class KEResourceItemWidget : public QWidget
{
protected:
	QWidget* m_Browser;
public:
	Ui::KEResourceItemWidget ui;
	KEResourceItemWidget(QWidget *parent = Q_NULLPTR)
		: QWidget(parent),
		m_Browser(parent)
	{
		ui.setupUi(this);
	}
	~KEResourceItemWidget()
	{
	}

	QSize sizeHint() const override
	{
		QSize masterSize = m_Browser->size();
		return QSize(masterSize.width() * 4 / 5, masterSize.height());
	}
};