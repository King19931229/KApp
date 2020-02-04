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
	m_View(nullptr),
	m_bInit(false)
{
	
}

KEGraphWidget::~KEGraphWidget()
{
	assert(!m_bInit);
	assert(!m_MenuBar);
	assert(!m_View);
}

bool KEGraphWidget::Init()
{
	if (!m_bInit)
	{
		m_MenuBar = new QMenuBar(this);
		m_View = CreateViewImpl();

		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->addWidget(m_MenuBar);
		layout->addWidget(m_View);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		setLayout(layout);

		setWindowTitle("Graph");
		resize(800, 600);

		m_bInit = true;
		return true;
	}
	return false;
}

bool KEGraphWidget::UnInit()
{
	if (m_bInit)
	{
		SAFE_DELETE(m_View);
		SAFE_DELETE(m_MenuBar);
		m_bInit = false;
	}
	return true;
}