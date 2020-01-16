#include "KERenderWidget.h"
#include "KEQtRenderWindow.h"
#include <qevent.h>

#include <assert.h>

KERenderWidget::KERenderWidget(QWidget* pParent)
	: m_RenderCore(nullptr),
	m_RenderWindow(nullptr)
{
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
}

KERenderWidget::~KERenderWidget()
{
	ASSERT_RESULT(m_RenderCore == nullptr);
	ASSERT_RESULT(m_RenderWindow == nullptr);
}

bool KERenderWidget::Init(IKRenderCorePtr& core)
{
	m_RenderCore = core.get();
	m_RenderWindow = (KEQtRenderWindow*)(core->GetRenderWindow());
	return true;
}

bool KERenderWidget::UnInit()
{
	m_RenderCore = nullptr;
	m_RenderWindow = nullptr;
	return true;
}

void KERenderWidget::resizeEvent(QResizeEvent *event)
{
	if (m_RenderCore)
	{
		IKRenderDevice* device = m_RenderCore->GetRenderDevice();
		device->RecreateSwapChain();
	}
}

void KERenderWidget::paintEvent(QPaintEvent *event)
{
	m_RenderCore->Tick();
	// 保证此函数体每一帧都调用
	update();
}

bool KERenderWidget::QtKeyToInputKeyboard(Qt::Key button, InputKeyboard& keyboard)
{
	switch (button)
	{
	case Qt::Key_W:
		keyboard = INPUT_KEY_W;
		return true;
	case Qt::Key_S:
		keyboard = INPUT_KEY_S;
		return true;
	case Qt::Key_A:
		keyboard = INPUT_KEY_A;
		return true;
	case Qt::Key_D:
		keyboard = INPUT_KEY_D;
		return true;

	case Qt::Key_Q:
		keyboard = INPUT_KEY_Q;
		return true;
	case Qt::Key_E:
		keyboard = INPUT_KEY_E;
		return true;

	case Qt::Key_Enter:
		keyboard = INPUT_KEY_ENTER;
		return true;

	default:
		return false;
	}
}

void KERenderWidget::HandleKeyEvent(QKeyEvent *event, InputAction action)
{
	if (!m_RenderWindow->m_KeyboardCallbacks.empty())
	{
		Qt::Key key = (Qt::Key)event->key();
		InputKeyboard keyboard;

		if (QtKeyToInputKeyboard(key, keyboard))
		{
			for (auto it = m_RenderWindow->m_KeyboardCallbacks.begin(),
				itEnd = m_RenderWindow->m_KeyboardCallbacks.end();
				it != itEnd; ++it)
			{
				KKeyboardCallbackType& callback = (*(*it));
				callback(keyboard, action);
			}
		}
	}
}

void KERenderWidget::keyPressEvent(QKeyEvent *event)
{
	HandleKeyEvent(event, INPUT_ACTION_PRESS);
}

void KERenderWidget::keyReleaseEvent(QKeyEvent *event)
{
	HandleKeyEvent(event, INPUT_ACTION_RELEASE);
}

bool KERenderWidget::QtButtonToMouseButton(Qt::MouseButton button, InputMouseButton& mouseButton)
{
	if (button == Qt::LeftButton)
	{
		mouseButton = INPUT_MOUSE_BUTTON_LEFT;
		return true;
	}
	else if (button == Qt::RightButton)
	{
		mouseButton = INPUT_MOUSE_BUTTON_RIGHT;
		return true;
	}
	else if (button == Qt::MidButton)
	{
		mouseButton = INPUT_MOUSE_BUTTON_MIDDLE;
		return true;
	}
	else if (button == Qt::NoButton)
	{
		mouseButton = INPUT_MOUSE_BUTTON_NONE;
		return true;
	}
	return false;
}

void KERenderWidget::HandleMouseEvent(QMouseEvent *event, InputAction action)
{
	if (!m_RenderWindow->m_MouseCallbacks.empty())
	{
		Qt::MouseButton button = event->button();
		QPoint mousePos = event->pos();
		InputMouseButton mouseButton;
		if (QtButtonToMouseButton(button, mouseButton))
		{
			for (auto it = m_RenderWindow->m_MouseCallbacks.begin(),
				itEnd = m_RenderWindow->m_MouseCallbacks.end();
				it != itEnd; ++it)
			{
				KMouseCallbackType& callback = (*(*it));
				callback(mouseButton, action, (float)mousePos.x(), (float)mousePos.y());
			}
		}
	}
}

void KERenderWidget::mousePressEvent(QMouseEvent *event)
{
	HandleMouseEvent(event, INPUT_ACTION_PRESS);
}

void KERenderWidget::mouseReleaseEvent(QMouseEvent *event)
{
	HandleMouseEvent(event, INPUT_ACTION_RELEASE);
}

void KERenderWidget::mouseMoveEvent(QMouseEvent *event)
{
	HandleMouseEvent(event, INPUT_ACTION_REPEAT);
}

void KERenderWidget::wheelEvent(QWheelEvent *event)
{
	int delta = event->delta() / QWheelEvent::DefaultDeltasPerStep;
	if (!m_RenderWindow->m_ScrollCallbacks.empty())
	{
		for (auto it = m_RenderWindow->m_ScrollCallbacks.begin(),
			itEnd = m_RenderWindow->m_ScrollCallbacks.end();
			it != itEnd; ++it)
		{
			KScrollCallbackType& callback = (*(*it));
			callback((float)0, (float)delta);
		}
	}
}