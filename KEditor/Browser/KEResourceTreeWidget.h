#pragma once
#include "ui_KEResourceTreeWidget.h"
#include "Browser/KEResourceWidgetEventFilter.h"

class KEResourceTreeWidget : public QWidget
{
protected:
	QWidget* m_Browser;
	KEResourceWidgetEventFilter m_Filter;
public:
	Ui::KEResourceTreeWidget ui;
	KEResourceTreeWidget(QWidget *parent = Q_NULLPTR);
	~KEResourceTreeWidget();

	QSize sizeHint() const override;
	void resizeEvent(QResizeEvent* event) override;
};