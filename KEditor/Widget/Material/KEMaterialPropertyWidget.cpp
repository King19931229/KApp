#include "KEMaterialPropertyWidget.h"
#include <assert.h>

KEMaterialPropertyWidget::KEMaterialPropertyWidget(QWidget *parent)
	: QMainWindow(parent),
	m_MaterialWindow(parent)
{
}

KEMaterialPropertyWidget::~KEMaterialPropertyWidget()
{
}

QSize KEMaterialPropertyWidget::sizeHint() const
{
	assert(m_MaterialWindow);
	QSize parentSizeHint = m_MaterialWindow->sizeHint();
	int width = parentSizeHint.width() / 3;
	int height = parentSizeHint.height();
	return QSize(width, height);
}

bool KEMaterialPropertyWidget::Init(IKMaterial* material)
{
	return true;
}

bool KEMaterialPropertyWidget::UnInit()
{
	return true;
}