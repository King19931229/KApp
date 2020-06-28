#include "KEMaterialRenderWidget.h"
#include "Other/KEQtRenderWindow.h"

KEMaterialRenderWidget::KEMaterialRenderWidget(QWidget* pParent)
	: KERenderWidget(pParent)
{
}

KEMaterialRenderWidget::~KEMaterialRenderWidget()
{
}

bool KEMaterialRenderWidget::Init(IKEnginePtr& engine)
{
	UnInit();

	if (engine)
	{
		m_Engine = engine.get();
		IKRenderCore* render = m_Engine->GetRenderCore();

		if (render)
		{
			m_SecordaryWindow = IKRenderWindowPtr(KNEW KEQtRenderWindow());
			m_SecordaryWindow->SetRenderDevice(engine->GetRenderCore()->GetRenderDevice());
			m_SecordaryWindow->Init((void*)winId(), false);

			m_RenderWindow = (KEQtRenderWindow*)m_SecordaryWindow.get();
			m_RenderDevice = render->GetRenderDevice();

			m_Engine->GetRenderCore()->RegisterSecordaryWindow(m_SecordaryWindow);

			return true;
		}
	}
	return false;
}

bool KEMaterialRenderWidget::UnInit()
{
	if (m_Engine)
	{
		if (m_SecordaryWindow)
		{
			m_Engine->GetRenderCore()->UnRegisterSecordaryWindow(m_SecordaryWindow);
			m_SecordaryWindow = nullptr;
		}
		m_Engine = nullptr;
	}
	m_RenderDevice = nullptr;
	m_RenderWindow = nullptr;
	return true;
}

void KEMaterialRenderWidget::paintEvent(QPaintEvent *event)
{
	KERenderWidget::paintEvent(event);
}