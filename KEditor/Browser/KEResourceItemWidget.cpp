#include "KEResourceItemWidget.h"
#include "KEResourceBrowser.h"

KEResourceItemWidget::KEResourceItemWidget(QWidget *parent)
	: QWidget(parent),
	m_Browser(parent)
{
	ui.setupUi(this);
	ui.m_ItemView->setMouseTracking(false);
	ui.m_ItemView->installEventFilter(&m_Filter);
}

KEResourceItemWidget::~KEResourceItemWidget()
{
}

QSize KEResourceItemWidget::sizeHint() const
{
	KEResourceBrowser* master = (KEResourceBrowser*)m_Browser;
	return master->ItemWidgetSize();
}

void KEResourceItemWidget::resizeEvent(QResizeEvent* event)
{
}