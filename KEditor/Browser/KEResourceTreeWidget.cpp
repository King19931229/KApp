#include "KEResourceTreeWidget.h"
#include "KEResourceBrowser.h"

KEResourceTreeWidget::KEResourceTreeWidget(QWidget *parent)
	: QWidget(parent),
	m_Browser(parent)
{
	ui.setupUi(this);
	ui.m_TreeView->setMouseTracking(false);
	ui.m_TreeView->installEventFilter(&m_Filter);
}

KEResourceTreeWidget::~KEResourceTreeWidget()
{
}

QSize KEResourceTreeWidget::sizeHint() const
{
	KEResourceBrowser* master = (KEResourceBrowser*)m_Browser;
	return master->TreeWidgetSize();
}

void KEResourceTreeWidget::resizeEvent(QResizeEvent* event)
{
}