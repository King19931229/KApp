#include "KEResourceBrowser.h"
#include "KEditorConfig.h"
#include <assert.h>

KEResourceBrowser::KEResourceBrowser(QWidget *parent)
	: QWidget(parent)
{
	m_MainWindow = parent;

	ui.setupUi(this);

	m_Layout = new QHBoxLayout();

	m_Layout->addWidget(ui.treeWidget);
	m_Layout->addWidget(ui.itemWidget);
	m_Layout->setStretch(0, 30);
	m_Layout->setStretch(1, 70);

	this->setLayout(m_Layout);	
}

KEResourceBrowser::~KEResourceBrowser()
{
	SAFE_DELETE(m_Layout);
}

QSize KEResourceBrowser::sizeHint() const
{
	assert(m_MainWindow);
	int width = m_MainWindow->width();
	int height = m_MainWindow->height() * 2 / 5;
	return QSize(width, height);
}

bool KEResourceBrowser::Init()
{
	return false;
}

bool KEResourceBrowser::UnInit()
{
	return false;
}