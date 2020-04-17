#pragma once
#include "ui_KEResourceItemWidget.h"
#include "Browser/KEResourceWidgetEventFilter.h"

class KEResourceItemWidget : public QWidget
{
protected:
	QWidget* m_Browser;
	KEResourceWidgetEventFilter m_Filter;
public:
	Ui::KEResourceItemWidget ui;
	KEResourceItemWidget(QWidget *parent = Q_NULLPTR);
	~KEResourceItemWidget();

	QSize sizeHint() const override;
	void resizeEvent(QResizeEvent* event) override;
};