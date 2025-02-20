#include "KERenderWidget.h"
#include "Other/KEQtRenderWindow.h"
#include "KEngine/Interface/IKEngine.h"

#include <QWheelEvent>
#include <assert.h>

KERenderWidget::KERenderWidget(QWidget* pParent)
	: m_Engine(nullptr),
	m_RenderDevice(nullptr),
	m_RenderWindow(nullptr)
{
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	setAcceptDrops(true);
	ZERO_ARRAY_MEMORY(m_KeyDown);
}

KERenderWidget::~KERenderWidget()
{
	ASSERT_RESULT(m_Engine == nullptr);
	ASSERT_RESULT(m_RenderDevice == nullptr);
	ASSERT_RESULT(m_RenderWindow == nullptr);
}

IKRenderWindow* KERenderWidget::GetRenderWindow()
{
	return m_RenderWindow;
}

void KERenderWidget::resizeEvent(QResizeEvent *event)
{
}

void KERenderWidget::paintEvent(QPaintEvent *event)
{
	// 保证此函数体每一帧都调用
	update();
}

void KERenderWidget::closeEvent(QCloseEvent* event)
{
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

	case Qt::Key_R:
		keyboard = INPUT_KEY_R;
		return true;

	case Qt::Key_1:
		keyboard = INPUT_KEY_1;
		return true;
	case Qt::Key_2:
		keyboard = INPUT_KEY_2;
		return true;
	case Qt::Key_3:
		keyboard = INPUT_KEY_3;
		return true;
	case Qt::Key_4:
		keyboard = INPUT_KEY_4;
		return true;

	case Qt::Key_Enter:
		keyboard = INPUT_KEY_ENTER;
		return true;
	case Qt::Key_Control:
		keyboard = INPUT_KEY_CTRL;
		return true;
	case Qt::Key_Delete:
		keyboard = INPUT_KEY_DELETE;
	case Qt::Key_Space:
		keyboard = INPUT_KEY_SPACE;
		return true;

	default:
		return false;
	}
}

void KERenderWidget::HandleKeyEvent(QKeyEvent *event, InputAction action)
{
	Qt::Key key = (Qt::Key)event->key();
	InputKeyboard keyboard;

	if (QtKeyToInputKeyboard(key, keyboard))
	{
		if (action == INPUT_ACTION_RELEASE)
		{
			// 键盘按下消息在其他窗口促发
			// 但是释放消息在这个窗口发生
			// 该消息直接忽略
			if (!m_KeyDown[keyboard])
			{
				return;
			}
			m_KeyDown[keyboard] = false;
		}
		else if (INPUT_ACTION_PRESS)
		{
			m_KeyDown[keyboard] = true;
		}

		if (!m_RenderWindow->m_KeyboardCallbacks.empty())
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

void KERenderWidget::HandleFocusEvent(QFocusEvent *event)
{
	bool gainFocus = event->gotFocus();
	if (!m_RenderWindow->m_FocusCallbacks.empty())
	{
		for (auto it = m_RenderWindow->m_FocusCallbacks.begin(),
			itEnd = m_RenderWindow->m_FocusCallbacks.end();
			it != itEnd; ++it)
		{
			KFocusCallbackType& callback = (*(*it));
			callback(gainFocus);
		}
	}
}

void KERenderWidget::focusInEvent(QFocusEvent *event)
{
	HandleFocusEvent(event);
}

void KERenderWidget::focusOutEvent(QFocusEvent *event)
{
	HandleFocusEvent(event);
}

void KERenderWidget::dragEnterEvent(QDragEnterEvent *event)
{
	event->setDropAction(Qt::MoveAction);
	event->accept();
}

void KERenderWidget::dragMoveEvent(QDragMoveEvent *event)
{
	event->setDropAction(Qt::MoveAction);
	event->accept();
}

void KERenderWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
}