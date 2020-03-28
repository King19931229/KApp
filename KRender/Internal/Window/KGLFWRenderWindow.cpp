#include "KGLFWRenderWindow.h"
#include "KBase/Interface/IKLog.h"
#include "Interface/IKRenderDevice.h"
#include <assert.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#endif

KGLFWRenderWindow::KGLFWRenderWindow()
	: m_device(nullptr)
{
#ifndef __ANDROID__
	m_LastMovePos[0] = -1.0f;
	m_LastMovePos[1] = -1.0f;
	m_window = nullptr;
#if defined(_WIN32)
	m_HWND = NULL;
#endif
#endif
}

KGLFWRenderWindow::~KGLFWRenderWindow()
{

}

RenderWindowType KGLFWRenderWindow::GetType()
{
	return RENDER_WINDOW_GLFW;
}

#ifndef __ANDROID__
void KGLFWRenderWindow::FramebufferResizeCallback(GLFWwindow* handle, int width, int height)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (window && window->m_device)
	{
		window->m_device->RecreateSwapChain();
	}
}

bool KGLFWRenderWindow::GLFWKeyToInputKeyboard(int key, InputKeyboard& board)
{
	switch (key)
	{
	case GLFW_KEY_W:
		board = INPUT_KEY_W;
		return true;

	case GLFW_KEY_S:
		board = INPUT_KEY_S;
		return true;

	case GLFW_KEY_A:
		board = INPUT_KEY_A;
		return true;

	case GLFW_KEY_D:
		board = INPUT_KEY_D;
		return true;

	case GLFW_KEY_Q:
		board = INPUT_KEY_Q;
		return true;

	case GLFW_KEY_E:
		board = INPUT_KEY_E;
		return true;

	case GLFW_KEY_R:
		board = INPUT_KEY_R;
		return true;

	case GLFW_KEY_1:
		board = INPUT_KEY_1;
		return true;
	case GLFW_KEY_2:
		board = INPUT_KEY_2;
		return true;
	case GLFW_KEY_3:
		board = INPUT_KEY_3;
		return true;
	case GLFW_KEY_4:
		board = INPUT_KEY_4;
		return true;

	case GLFW_KEY_ENTER:
		board = INPUT_KEY_ENTER;
		return true;

	default:
		return false;
	}
}

bool KGLFWRenderWindow::GLFWMouseButtonToInputMouseButton(int mouse, InputMouseButton& mouseButton)
{
	switch (mouse)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		mouseButton = INPUT_MOUSE_BUTTON_LEFT;
		return true;

	case GLFW_MOUSE_BUTTON_MIDDLE:
		mouseButton = INPUT_MOUSE_BUTTON_MIDDLE;
		return true;

	case GLFW_MOUSE_BUTTON_RIGHT:
		mouseButton = INPUT_MOUSE_BUTTON_RIGHT;
		return true;

	default:
		return false;
	}
}

bool KGLFWRenderWindow::GLFWActionToInputAction(int action, InputAction& inputAction)
{
	switch (action)
	{
	case GLFW_RELEASE:
		inputAction = INPUT_ACTION_RELEASE;
		return true;

	case GLFW_PRESS:
		inputAction = INPUT_ACTION_PRESS;
		return true;

	case GLFW_REPEAT:
		inputAction = INPUT_ACTION_REPEAT;
		return true;

	default:
		return false;
	}
}

void KGLFWRenderWindow::KeyboardCallback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (window && window->m_device && !window->m_KeyboardCallbacks.empty())
	{
		InputKeyboard keyboard;
		InputAction inputAction;

		if (GLFWKeyToInputKeyboard(key, keyboard) && GLFWActionToInputAction(action, inputAction))
		{
			for (auto it = window->m_KeyboardCallbacks.begin(),
				itEnd = window->m_KeyboardCallbacks.end();
				it != itEnd; ++it)
			{
				KKeyboardCallbackType& callback = (*(*it));
				callback(keyboard, inputAction);
			}
		}
	}
}

void KGLFWRenderWindow::MouseCallback(GLFWwindow* handle, int mouse, int action, int mods)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (window && window->m_device && !window->m_MouseCallbacks.empty())
	{
		InputMouseButton mouseButton;
		InputAction inputAction;

		double xpos = 0, ypos = 0;
		glfwGetCursorPos(window->m_window, &xpos, &ypos);

		if (GLFWMouseButtonToInputMouseButton(mouse, mouseButton) && GLFWActionToInputAction(action, inputAction))
		{
			for (auto it = window->m_MouseCallbacks.begin(),
				itEnd = window->m_MouseCallbacks.end();
				it != itEnd; ++it)
			{
				KMouseCallbackType& callback = (*(*it));
				callback(mouseButton, inputAction, (float)xpos, (float)ypos);
			}
		}
	}
}

void KGLFWRenderWindow::ScrollCallback(GLFWwindow* handle, double xoffset, double yoffset)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (window && window->m_device && !window->m_ScrollCallbacks.empty())
	{
		for (auto it = window->m_ScrollCallbacks.begin(),
			itEnd = window->m_ScrollCallbacks.end();
			it != itEnd; ++it)
		{
			KScrollCallbackType& callback = (*(*it));
			callback((float)xoffset, (float)yoffset);
		}
	}
}

void KGLFWRenderWindow::OnMouseMove()
{
	if (!m_MouseCallbacks.empty())
	{
		double xpos = 0, ypos = 0;
		glfwGetCursorPos(m_window, &xpos, &ypos);

		constexpr float exp = 0.001f;
		if (fabs(xpos - m_LastMovePos[0]) > exp || fabs(ypos - m_LastMovePos[1]) > exp)
		{
			for (auto it = m_MouseCallbacks.begin(), itEnd = m_MouseCallbacks.end();
				it != itEnd; ++it)
			{
				KMouseCallbackType& callback = (*(*it));
				callback(INPUT_MOUSE_BUTTON_NONE, INPUT_ACTION_REPEAT, (float)xpos, (float)ypos);
			}

			m_LastMovePos[0] = xpos;
			m_LastMovePos[1] = ypos;
		}
	}
}
#endif

bool KGLFWRenderWindow::Init(size_t top, size_t left, size_t width, size_t height, bool resizable)
{
#ifndef __ANDROID__
	if (glfwInit() == GLFW_TRUE)
	{
		glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow((int)width, (int)height, "Vulkan window", nullptr, nullptr);
		if (m_window)
		{
			glfwSetWindowUserPointer(m_window, this);
			if (resizable)
			{
				glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
				glfwSetKeyCallback(m_window, KeyboardCallback);
				glfwSetMouseButtonCallback(m_window, MouseCallback);
				glfwSetScrollCallback(m_window, ScrollCallback);
			}
			glfwSetWindowPos(m_window, (int)top, (int)left);
#ifdef	_WIN32
			m_HWND = glfwGetWin32Window(m_window);
#endif
			m_LastMovePos[0] = -1.0f;
			m_LastMovePos[1] = -1.0f;

			return true;
		}
	}
	else
	{
		m_window = nullptr;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::Init(android_app* app)
{
	assert(false && "GLFW window can not inited by android_app");
	return false;
}

bool KGLFWRenderWindow::Init(void* hwnd)
{
	// https://github.com/glfw/glfw/issues/25
	assert(false && "GLFW window can not inited by hwnd");
	return false;
}

bool KGLFWRenderWindow::UnInit()
{
	m_device = nullptr;
#ifndef __ANDROID__
	if (m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}

	m_KeyboardCallbacks.clear();
	m_MouseCallbacks.clear();
	m_ScrollCallbacks.clear();
#endif
	return true;
}

android_app* KGLFWRenderWindow::GetAndroidApp()
{
	assert(false && "GLFW window can not get android handle");
	return nullptr;
}

void* KGLFWRenderWindow::GetHWND()
{
#ifndef __ANDROID__
#	ifdef _WIN32
	return m_HWND;
#	endif
#endif
	assert(false && "none win32 platform can not get hwnd handle");
	return nullptr;
}

bool KGLFWRenderWindow::IdleUntilForeground()
{
#ifndef __ANDROID__
	if (m_window)
	{
		int width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_window, &width, &height);
			glfwWaitEvents();
		}
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::Loop()
{
#ifndef __ANDROID__
	if (m_window)
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
			if (m_device)
			{
				OnMouseMove();
				m_device->Present();
			}
		}

		if (m_device)
		{
			m_device->Wait();
		}
		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

bool KGLFWRenderWindow::GetPosition(size_t &top, size_t &left)
{
#ifndef __ANDROID__
	if (m_window)
	{
		int xPos = -1, yPos = -1;
		glfwGetWindowPos(m_window, &xPos, &yPos);
		top = (size_t)xPos, left = (size_t)yPos;
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetPosition(size_t top, size_t left)
{
#ifndef __ANDROID__
	if (m_window)
	{
		glfwSetWindowPos(m_window, (int)top, (int)left);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::GetSize(size_t &width, size_t &height)
{
#ifndef __ANDROID__
	if (m_window)
	{
		int nWidth = -1, nHeight = -1;
		glfwGetWindowSize(m_window, &nWidth, &nHeight);
		width = (size_t) nWidth, height = (size_t) nHeight;
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetSize(size_t width, size_t height)
{
#ifndef __ANDROID__
	if (m_window)
	{
		glfwSetWindowSize(m_window, (int)width, (int)height);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetResizable(bool resizable)
{
	return false;
}

bool KGLFWRenderWindow::IsResizable()
{
#ifndef __ANDROID__
	if (m_window)
	{
		int hint = glfwGetWindowAttrib(m_window, GLFW_RESIZABLE);
		return hint == GLFW_TRUE;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetWindowTitle(const char* pName)
{
#ifndef __ANDROID__
	if (m_window)
	{
		glfwSetWindowTitle(m_window, pName);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef __ANDROID__
	if (callback && std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback) == m_KeyboardCallbacks.end())
	{
		m_KeyboardCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef __ANDROID__
	if (callback && std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback) == m_MouseCallbacks.end())
	{
		m_MouseCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef __ANDROID__
	if (callback && std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback) == m_ScrollCallbacks.end())
	{
		m_ScrollCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterTouchCallback(KTouchCallbackType* callback)
{
	assert(false && "GLFW window can not registered touch callback");
	return false;
}

bool KGLFWRenderWindow::UnRegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef __ANDROID__
	if (callback)
	{
		auto it = std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback);
		if (it != m_KeyboardCallbacks.end())
		{
			m_KeyboardCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KGLFWRenderWindow::UnRegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef __ANDROID__
	if (callback)
	{
		auto it = std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback);
		if (it != m_MouseCallbacks.end())
		{
			m_MouseCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KGLFWRenderWindow::UnRegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef __ANDROID__
	if (callback)
	{
		auto it = std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback);
		if (it != m_ScrollCallbacks.end())
		{
			m_ScrollCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KGLFWRenderWindow::UnRegisterTouchCallback(KTouchCallbackType *callback)
{
	assert(false && "GLFW window can not unregistered touch callback");
	return false;
}

bool KGLFWRenderWindow::SetRenderDevice(IKRenderDevice* device)
{
	m_device = device;
	return true;
}