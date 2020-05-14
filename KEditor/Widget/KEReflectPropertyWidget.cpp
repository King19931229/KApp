#include "KEReflectPropertyWidget.h"
#include <assert.h>

KEReflectPropertyWidget::KEReflectPropertyWidget(QWidget *parent)
	: QWidget(parent),
	m_MainWindow(parent)
{
}

KEReflectPropertyWidget::~KEReflectPropertyWidget()
{
}

QSize KEReflectPropertyWidget::sizeHint() const
{
	assert(m_MainWindow);
	int width = m_MainWindow->width() / 4;
	int height = m_MainWindow->height();
	return QSize(width, height);
}

bool KEReflectPropertyWidget::Init()
{
	return true;
}

bool KEReflectPropertyWidget::UnInit()
{
	return true;
}