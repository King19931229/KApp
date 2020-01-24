#include "KEGraphWidget.h"
#include "Graph/KEGraphView.h"
#include "Graph/Node/KEGraphNodeView.h"
#include "KEditorConfig.h"
#include <QLayout>
#include <assert.h>

KEGraphWidget::KEGraphWidget()
	: QWidget(),
	m_View(nullptr)
{
	m_View = new KEGraphView(this);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_View);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	setWindowTitle("Graph");
	resize(800, 600);
}

KEGraphWidget::~KEGraphWidget()
{
	SAFE_DELETE(m_View);
}

void KEGraphWidget::Test()
{
}