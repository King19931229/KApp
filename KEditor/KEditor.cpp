#include "KEditor.h"
#include "KERenderWidget.h"
#include "KEQtRenderWindow.h"

#include <assert.h>

KEditor::KEditor(QWidget *parent)
	: QMainWindow(parent),
	m_RenderWidget(nullptr),
	m_bInit(false)
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
	if (!m_bInit)
	{
		m_RenderWidget = new KERenderWidget();

		m_RenderCore = CreateRenderCore();

		m_RenderWindow = IKRenderWindowPtr((IKRenderWindow*)new KEQtRenderWindow());
		m_RenderWindow->Init((void*)m_RenderWidget->winId());

		m_RenderDevice = CreateRenderDevice(RENDER_DEVICE_VULKAN);

		m_RenderDevice->Init(m_RenderWindow.get());
		m_RenderCore->Init(m_RenderDevice, m_RenderWindow);

		m_RenderWidget->Init(m_RenderCore);
		setCentralWidget(m_RenderWidget);

		m_bInit = true;
		return true;
	}

	return false;
}

bool KEditor::UnInit()
{
	if (m_bInit)
	{
		m_RenderWindow->UnInit();
		m_RenderWindow = nullptr;

		m_RenderDevice->UnInit();
		m_RenderDevice = nullptr;

		m_RenderCore->UnInit();
		m_RenderCore = nullptr;

		if (m_RenderWidget)
		{
			m_RenderWidget->UnInit();
			SAFE_DELETE(m_RenderWidget);
		}

		m_bInit = false;
	}

	return true;
}