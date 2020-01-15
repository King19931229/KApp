#include "KERenderWidget.h"
#include <assert.h>

KERenderWidget::KERenderWidget(QWidget* pParent)
	: m_RenderCore(nullptr)
{
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
}

KERenderWidget::~KERenderWidget()
{
	ASSERT_RESULT(m_RenderCore == nullptr);
}

bool KERenderWidget::Init(IKRenderCorePtr& core)
{
	m_RenderCore = core.get();
	return true;
}

bool KERenderWidget::UnInit()
{
	m_RenderCore = nullptr;
	return true;
}

void KERenderWidget::resizeEvent(QResizeEvent *event)
{
	return;
}

void KERenderWidget::paintEvent(QPaintEvent *event)
{
	m_RenderCore->Tick();
	// ��֤�˺�����ÿһ֡������
	update();
}