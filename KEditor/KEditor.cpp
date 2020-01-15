#include "KEditor.h"
#include "KERenderWidget.h"

#include <assert.h>

KEditor::KEditor(QWidget *parent)
	: QMainWindow(parent),
	m_RenderWidget(nullptr)
{
	ui.setupUi(this);
}

KEditor::~KEditor()
{
	assert(m_RenderWidget == nullptr);
	assert(m_RenderCore == nullptr);
}

bool KEditor::Init()
{
	m_RenderWidget = new KERenderWidget();
	m_RenderCore = CreateRenderCore();

	m_RenderCore->Init(RD_VULKAN, (void*)m_RenderWidget->winId());
	m_RenderWidget->Init(m_RenderCore);

	setCentralWidget(m_RenderWidget);

	return true;
}

bool KEditor::UnInit()
{
	if (m_RenderWidget)
	{
		m_RenderWidget->UnInit();
		SAFE_DELETE(m_RenderWidget);
	}
	m_RenderCore->UnInit();
	m_RenderCore = nullptr;

	return true;
}