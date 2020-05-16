#include "KEReflectPropertyWidget.h"
#include "Reflection/KEReflectionManager.h"
#include "Reflection/KEReflectObjectTreeModel.h"
#include "Reflection/KEReflectObjectTreeView.h"

#include <assert.h>

KEReflectPropertyWidget::KEReflectPropertyWidget(QWidget *parent)
	: QMainWindow(parent),
	m_MainWindow(parent),
	m_Current(nullptr)
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

void KEReflectPropertyWidget::SetObject(KReflectionObjectBase* reflection)
{
	if (centralWidget())
	{
		centralWidget()->setParent(nullptr);
	}

	m_TreeView->setModel(nullptr);
	m_TreeModel->SetReflection(reflection);
	m_TreeView->setModel(m_TreeModel);

	setCentralWidget(m_TreeView);
	m_TreeView->expandAll();

	m_Current = reflection;
}

void KEReflectPropertyWidget::RefreshObject(KReflectionObjectBase* reflection)
{
	// TODO 先这样解决
	if (m_Current && m_Current == reflection)
	{
		SetObject(reflection);
	}
}

void KEReflectPropertyWidget::ClearObject(KReflectionObjectBase* reflection)
{
	// TODO 先这样解决
	SetObject(nullptr);
}

bool KEReflectPropertyWidget::Init()
{
	return true;
}

bool KEReflectPropertyWidget::UnInit()
{
	m_Current = nullptr;
	return true;
}