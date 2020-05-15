#include "KEReflectPropertyWidget.h"
#include "Reflection/KEReflectionManager.h"
#include "Reflection/KEReflectObjectTreeModel.h"
#include "Reflection/KEReflectObjectTreeView.h"

#include <assert.h>

KEReflectPropertyWidget::KEReflectPropertyWidget(QWidget *parent)
	: QMainWindow(parent),
	m_MainWindow(parent)
{
	m_TreeView = new KEReflectObjectTreeView();
	m_TreeModel = new KEReflectObjectTreeModel();
	m_TreeView->setModel(m_TreeModel);
}

KEReflectPropertyWidget::~KEReflectPropertyWidget()
{
	if (centralWidget())
	{
		centralWidget()->setParent(nullptr);
	}

	m_TreeView->setModel(nullptr);
	SAFE_DELETE(m_TreeView);
	SAFE_DELETE(m_TreeModel);
}

QSize KEReflectPropertyWidget::sizeHint() const
{
	assert(m_MainWindow);
	int width = m_MainWindow->width() / 4;
	int height = m_MainWindow->height();
	return QSize(width, height);
}

void KEReflectPropertyWidget::SetWidget(KEReflectObjectWidget* widget)
{
	if (centralWidget())
	{
		centralWidget()->setParent(nullptr);
	}
	setCentralWidget(widget);
}

void KEReflectPropertyWidget::SetObject(KReflectionObjectBase* reflection)
{
	if (centralWidget())
	{
		centralWidget()->setParent(nullptr);
	}

	m_TreeView->setModel(nullptr);
	m_TreeModel->SetReflection(reflection);
	m_TreeView->setModel(m_TreeModel);
}

bool KEReflectPropertyWidget::Init()
{
	return true;
}

bool KEReflectPropertyWidget::UnInit()
{
	return true;
}