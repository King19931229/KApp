#include "KEQtRenderWindow.h"
#include "KRender/Interface/IKUIOverlay.h"
#include "KRender/Interface/IKSwapChain.h"
#include "KBase/Publish/KTemplate.h"

#ifdef _WIN32
#include <Windows.h>
#endif

KEQtRenderWindow::KEQtRenderWindow()
	: m_Device(nullptr),
	m_HWND(nullptr),
	m_SwapChain(nullptr),
	m_bPrimary(true)
{
}

KEQtRenderWindow::~KEQtRenderWindow()
{
	ASSERT_RESULT(m_HWND == nullptr);
	ASSERT_RESULT(!m_UIOverlay);
	ASSERT_RESULT(!m_SwapChain);
}

RenderWindowType KEQtRenderWindow::GetType()
{
	return RENDER_WINDOW_EXTERNAL;
}

bool KEQtRenderWindow::Init(size_t top, size_t left, size_t width, size_t height, bool resizable, bool primary)
{
	return false;
}

bool KEQtRenderWindow::Init(android_app* app)
{
	return false;
}

bool KEQtRenderWindow::Init(void* hwnd, bool primary)
{
	ASSERT_RESULT(m_Device);
	m_HWND = hwnd;
	m_bPrimary = primary;
	if (m_Device && m_bPrimary)
	{
		return m_Device->Init(this);
	}
	return m_Device != nullptr;
}

bool KEQtRenderWindow::UnInit()
{
	m_HWND = nullptr;

	if (m_Device)
	{
		if (m_bPrimary)
		{
			m_Device->UnInit();
		}
		m_Device = nullptr;
	}

	m_KeyboardCallbacks.clear();
	m_MouseCallbacks.clear();
	m_ScrollCallbacks.clear();
	m_FocusCallbacks.clear();
	m_ResizeCallbacks.clear();

	return true;
}

bool KEQtRenderWindow::CreateUISwapChain()
{
	DestroyUISwapChain();

	m_Device->CreateSwapChain(m_SwapChain);
	ASSERT_RESULT(m_SwapChain->Init(this));

	m_Device->CreateUIOverlay(m_UIOverlay);
	m_UIOverlay->Init();
	m_UIOverlay->Resize(m_SwapChain->GetWidth(), m_SwapChain->GetHeight());

	return true;
}

bool KEQtRenderWindow::DestroyUISwapChain()
{
	if (m_UIOverlay)
	{
		m_UIOverlay->UnInit();
		m_UIOverlay = nullptr;
	}

	if (m_SwapChain)
	{
		m_SwapChain->UnInit();
		m_SwapChain = nullptr;
	}

	return true;
}

android_app* KEQtRenderWindow::GetAndroidApp()
{
	return nullptr;
}

IKUIOverlay* KEQtRenderWindow::GetUIOverlay()
{
	return m_UIOverlay ? m_UIOverlay.get() : nullptr;
}

IKSwapChain* KEQtRenderWindow::GetSwapChain()
{
	return m_SwapChain ? m_SwapChain.get() : nullptr;
}

void* KEQtRenderWindow::GetHWND()
{
	return m_HWND;
}

bool KEQtRenderWindow::Tick()
{
	return true;
}

bool KEQtRenderWindow::Loop()
{
	return false;
}

bool KEQtRenderWindow::IsMinimized()
{
	return false;
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

bool KEQtRenderWindow::SetRenderDevice(IKRenderDevice* device)
{
	m_Device = device;
	return true;
}

bool KEQtRenderWindow::RegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
	return KTemplate::RegisterCallback(m_KeyboardCallbacks, callback);
}

bool KEQtRenderWindow::RegisterMouseCallback(KMouseCallbackType* callback)
{
	return KTemplate::RegisterCallback(m_MouseCallbacks, callback);
}

bool KEQtRenderWindow::RegisterScrollCallback(KScrollCallbackType* callback)
{
	return KTemplate::RegisterCallback(m_ScrollCallbacks, callback);
}

bool KEQtRenderWindow::RegisterTouchCallback(KTouchCallbackType* callback)
{
	return false;
}

bool KEQtRenderWindow::UnRegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
	return KTemplate::UnRegisterCallback(m_KeyboardCallbacks, callback);
}

bool KEQtRenderWindow::UnRegisterMouseCallback(KMouseCallbackType* callback)
{
	return KTemplate::UnRegisterCallback(m_MouseCallbacks, callback);
}

bool KEQtRenderWindow::UnRegisterScrollCallback(KScrollCallbackType* callback)
{
	return KTemplate::UnRegisterCallback(m_ScrollCallbacks, callback);
}

bool KEQtRenderWindow::UnRegisterTouchCallback(KTouchCallbackType* callback)
{
	return false;
}

bool KEQtRenderWindow::RegisterFocusCallback(KFocusCallbackType* callback)
{
	return KTemplate::RegisterCallback(m_FocusCallbacks, callback);
}

bool KEQtRenderWindow::UnRegisterFocusCallback(KFocusCallbackType* callback)
{
	return KTemplate::UnRegisterCallback(m_FocusCallbacks, callback);
}

bool KEQtRenderWindow::RegisterResizeCallback(KResizeCallbackType* callback)
{
	return KTemplate::RegisterCallback(m_ResizeCallbacks, callback);
}

bool KEQtRenderWindow::UnRegisterResizeCallback(KResizeCallbackType* callback)
{
	return KTemplate::UnRegisterCallback(m_ResizeCallbacks, callback);
}