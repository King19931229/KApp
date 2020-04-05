#include "KEQtRenderWindow.h"

#ifdef _WIN32
#include <Windows.h>
#endif

KEQtRenderWindow::KEQtRenderWindow()
	: m_Device(nullptr),
	m_HWND(nullptr)
{
}

KEQtRenderWindow::~KEQtRenderWindow()
{
	assert(m_HWND == nullptr);
}

RenderWindowType KEQtRenderWindow::GetType()
{
	return RENDER_WINDOW_EXTERNAL;
}

bool KEQtRenderWindow::Init(size_t top, size_t left, size_t width, size_t height, bool resizable)
{
	return false;
}

bool KEQtRenderWindow::Init(android_app* app)
{
	return false;
}

bool KEQtRenderWindow::Init(void* hwnd)
{
	ASSERT_RESULT(m_Device);
	m_HWND = hwnd;
	m_Device->Init(this);
	return true;
}

bool KEQtRenderWindow::UnInit()
{
	m_HWND = nullptr;
	if (m_Device)
	{
		m_Device->UnInit();
		m_Device = nullptr;
	}
	return true;
}

android_app* KEQtRenderWindow::GetAndroidApp()
{
	return nullptr;
}

void* KEQtRenderWindow::GetHWND()
{
	return m_HWND;
}

bool KEQtRenderWindow::Loop()
{
	return false;
}

bool KEQtRenderWindow::IdleUntilForeground()
{
	return true;
}

bool KEQtRenderWindow::GetPosition(size_t &top, size_t &left)
{
	return false;
}

bool KEQtRenderWindow::SetPosition(size_t top, size_t left)
{
	return false;
}

bool KEQtRenderWindow::GetSize(size_t &width, size_t &height)
{
#ifdef _WIN32
	HWND hwnd = (HWND)m_HWND;
	RECT area;
	if (GetClientRect(hwnd, &area))
	{
		width = (size_t)area.right;
		height = (size_t)area.bottom;
		return true;
	}
#endif
	return false;
}

bool KEQtRenderWindow::SetSize(size_t width, size_t height)
{
	return false;
}

bool KEQtRenderWindow::SetResizable(bool resizable)
{
	return false;
}

bool KEQtRenderWindow::IsResizable()
{
	return false;
}

bool KEQtRenderWindow::SetWindowTitle(const char* pName)
{
	return false;
}

bool KEQtRenderWindow::RegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
	if (callback && std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback) == m_KeyboardCallbacks.end())
	{
		m_KeyboardCallbacks.push_back(callback);
		return true;
	}
	return false;
}

bool KEQtRenderWindow::RegisterMouseCallback(KMouseCallbackType* callback)
{
	if (callback && std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback) == m_MouseCallbacks.end())
	{
		m_MouseCallbacks.push_back(callback);
		return true;
	}
	return false;
}

bool KEQtRenderWindow::RegisterScrollCallback(KScrollCallbackType* callback)
{
	if (callback && std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback) == m_ScrollCallbacks.end())
	{
		m_ScrollCallbacks.push_back(callback);
		return true;
	}
	return false;
}

bool KEQtRenderWindow::RegisterTouchCallback(KTouchCallbackType* callback)
{
	return false;
}

bool KEQtRenderWindow::UnRegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
	if (callback)
	{
		auto it = std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback);
		if (it != m_KeyboardCallbacks.end())
		{
			m_KeyboardCallbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool KEQtRenderWindow::UnRegisterMouseCallback(KMouseCallbackType* callback)
{
	if (callback)
	{
		auto it = std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback);
		if (it != m_MouseCallbacks.end())
		{
			m_MouseCallbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool KEQtRenderWindow::UnRegisterScrollCallback(KScrollCallbackType* callback)
{
	if (callback)
	{
		auto it = std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback);
		if (it != m_ScrollCallbacks.end())
		{
			m_ScrollCallbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool KEQtRenderWindow::UnRegisterTouchCallback(KTouchCallbackType* callback)
{
	return false;
}

bool KEQtRenderWindow::SetRenderDevice(IKRenderDevice* device)
{
	m_Device = device;
	return true;
}