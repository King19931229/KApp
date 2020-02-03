#include "KEGraphWidget.h"
#include "Graph/KEGraphView.h"
#include "Graph/Node/KEGraphNodeView.h"

#include "Graph/Test/KEGraphNodeTestModel.h"
#include "KEditorConfig.h"
#include <QLayout>
#include <assert.h>

KEGraphWidget::KEGraphWidget()
	: QWidget(),
	m_MenuBar(nullptr),
	m_View(nullptr)
{
	m_MenuBar = new QMenuBar(this);
	m_View = new KEGraphView(this);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_MenuBar);
	layout->addWidget(m_View);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setLayout(layout);

	setWindowTitle("Graph");
	resize(800, 600);
}

KEGraphWidget::~KEGraphWidget()
{
	SAFE_DELETE(m_MenuBar);
	SAFE_DELETE(m_View);
}

void KEGraphWidget::Test()
{
	m_View->RegisterModel("Test", []()->KEGraphNodeModelPtr
	{
		return KEGraphNodeModelPtr(new KEGraphNodeTestModel());
	});
}