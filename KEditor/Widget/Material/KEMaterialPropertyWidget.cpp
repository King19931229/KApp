#include "KEMaterialPropertyWidget.h"
#include "Widget/Material/ModelView/KEMaterialPropertyTreeView.h"
#include "Widget/Material/ModelView/KEMaterialPropertyTreeModel.h"
#include <assert.h>

KEMaterialPropertyWidget::KEMaterialPropertyWidget(QWidget *parent)
	: QMainWindow(parent),
	m_MaterialWindow(parent)
{
	m_TreeView = KNEW KEMaterialPropertyTreeView();
	m_TreeModel = KNEW KEMaterialPropertyTreeModel();
	m_TreeView->setModel(m_TreeModel);
	setCentralWidget(m_TreeView);
}

KEMaterialPropertyWidget::~KEMaterialPropertyWidget()
{
	m_TreeView->setModel(nullptr);
	SAFE_DELETE(m_TreeView);
	SAFE_DELETE(m_TreeModel);
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
	m_TreeView->setModel(nullptr);
	m_TreeModel->SetMaterial(material);
	m_TreeView->setModel(m_TreeModel);

	m_TreeView->expandAll();
	return true;
}

bool KEMaterialPropertyWidget::UnInit()
{
	m_TreeView->setModel(nullptr);
	m_TreeModel->SetMaterial(nullptr);
	m_TreeView->setModel(m_TreeModel);

	m_TreeView->expandAll();
	return true;
}