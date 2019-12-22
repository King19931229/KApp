#include "KVulkanRenderWindow.h"
#include "KVulkanRenderDevice.h"

KVulkanRenderWindow::KVulkanRenderWindow()
	: m_device(nullptr)
{
#ifndef __ANDROID__
	m_window = nullptr;
	ZERO_ARRAY_MEMORY(m_MouseDown);
#else

#endif
}

KVulkanRenderWindow::~KVulkanRenderWindow()
{

}

#ifndef __ANDROID__

void KVulkanRenderWindow::FramebufferResizeCallback(GLFWwindow* handle, int width, int height)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device)
	{
		window->m_device->RecreateSwapChain();
	}
}

bool KVulkanRenderWindow::GLFWKeyToInputKeyboard(int key, InputKeyboard& board)
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

	case GLFW_KEY_ENTER:
		board = INPUT_KEY_ENTER;
		return true;

	default:
		return false;
	}
}

bool KVulkanRenderWindow::GLFWMouseButtonToInputMouseButton(int mouse, InputMouseButton& mouseButton)
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

bool KVulkanRenderWindow::GLFWActionToInputAction(int action, InputAction& inputAction)
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

void KVulkanRenderWindow::KeyboardCallback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device && !window->m_KeyboardCallbacks.empty())
	{
		InputKeyboard keyboard;
		InputAction inputAction;

		if(GLFWKeyToInputKeyboard(key, keyboard) && GLFWActionToInputAction(action, inputAction))
		{
			for(auto it = window->m_KeyboardCallbacks.begin(),
				itEnd = window->m_KeyboardCallbacks.end();
				it != itEnd; ++it)
			{
				KKeyboardCallbackType& callback = (*(*it));
				callback(keyboard, inputAction);
			}
		}
	}
}

void KVulkanRenderWindow::MouseCallback(GLFWwindow* handle, int mouse, int action, int mods)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device && !window->m_MouseCallbacks.empty())
	{
		InputMouseButton mouseButton;
		InputAction inputAction;

		double xpos = 0, ypos = 0;
		glfwGetCursorPos(window->m_window, &xpos, &ypos);

		if(GLFWMouseButtonToInputMouseButton(mouse, mouseButton) && GLFWActionToInputAction(action, inputAction))
		{
			window->m_MouseDown[mouseButton] = inputAction == INPUT_ACTION_PRESS ? true : false;

			for(auto it = window->m_MouseCallbacks.begin(),
				itEnd = window->m_MouseCallbacks.end();
				it != itEnd; ++it)
			{
				KMouseCallbackType& callback = (*(*it));
				callback(mouseButton, inputAction, (float)xpos, (float)ypos);
			}
		}
	}
}

void KVulkanRenderWindow::ScrollCallback(GLFWwindow* handle, double xoffset, double yoffset)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device && !window->m_ScrollCallbacks.empty())
	{
		for(auto it = window->m_ScrollCallbacks.begin(),
			itEnd = window->m_ScrollCallbacks.end();
			it != itEnd; ++it)
		{
			KScrollCallbackType& callback = (*(*it));
			callback((float)xoffset, (float)yoffset);
		}
	}
}

void KVulkanRenderWindow::OnMouseMove()
{
	if(!m_MouseCallbacks.empty())
	{
		double xpos = 0, ypos = 0;
		glfwGetCursorPos(m_window, &xpos, &ypos);
		for(auto it = m_MouseCallbacks.begin(), itEnd = m_MouseCallbacks.end();
			it != itEnd; ++it)
		{
			KMouseCallbackType& callback = (*(*it));
			callback(INPUT_MOUSE_BUTTON_NONE, INPUT_ACTION_REPEAT, (float)xpos, (float)ypos);
		}
	}
}

#endif

bool KVulkanRenderWindow::Init(size_t top, size_t left, size_t width, size_t height, bool resizable)
{
#ifndef	__ANDROID__
	if(glfwInit() == GLFW_TRUE)
	{
		glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow((int)width, (int)height, "Vulkan window", nullptr, nullptr);
		if(m_window)
		{
			glfwSetWindowUserPointer(m_window, this);
			if(resizable)
			{
				glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
				glfwSetKeyCallback(m_window, KeyboardCallback);
				glfwSetMouseButtonCallback(m_window, MouseCallback);
				glfwSetScrollCallback(m_window, ScrollCallback);
			}
			glfwSetWindowPos(m_window, (int)top, (int)left);
			ZERO_ARRAY_MEMORY(m_MouseDown);
			return true;
		}
	}
	else
	{
		m_window = nullptr;
	}
#else

#endif
	return false;
}

bool KVulkanRenderWindow::UnInit()
{
	if(m_device)
	{
		m_device = nullptr;
	}
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}

	m_KeyboardCallbacks.clear();
	m_MouseCallbacks.clear();
#else

#endif

	return true;
}

bool KVulkanRenderWindow::IdleUntilForeground()
{
#ifndef	__ANDROID__
	int width = 0, height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}
#else

#endif
	return true;
}

bool KVulkanRenderWindow::Loop()
{
#ifndef	__ANDROID__
	if(m_window)
	{
		while(!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
			OnMouseMove();
			if(m_device)
			{
				m_device->Present();
			}
		}
		// �������߳�ֱ��device���ж����������?
		if(m_device)
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

bool KVulkanRenderWindow::GetPosition(size_t &top, size_t &left)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		int xPos = -1, yPos = -1;
		glfwGetWindowPos(m_window, &xPos, &yPos);
		top = (size_t)xPos, left = (size_t)yPos;
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::SetPosition(size_t top, size_t left)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwSetWindowPos(m_window, (int)top, (int)left);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::GetSize(size_t &width, size_t &height)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		int nWidth = -1, nHeight = -1;
		glfwGetWindowSize(m_window, &nWidth, &nHeight);
		width = (size_t)nWidth, height = (size_t)nHeight;
		return true;
	}
#else

#endif
	return false;
}

bool KVulkanRenderWindow::SetSize(size_t width, size_t height)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwSetWindowSize(m_window, (int)width, (int)height);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::SetResizable(bool resizable)
{
	return false;
}

bool KVulkanRenderWindow::IsResizable()
{
#ifndef	__ANDROID__
	if(m_window)
	{
		int hint = glfwGetWindowAttrib(m_window, GLFW_RESIZABLE);
		return hint == GLFW_TRUE;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::SetWindowTitle(const char* pName)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwSetWindowTitle(m_window, pName);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::RegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback && std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback) == m_KeyboardCallbacks.end())
	{
		m_KeyboardCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::RegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback && std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback) == m_MouseCallbacks.end())
	{
		m_MouseCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::RegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback && std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback) == m_ScrollCallbacks.end())
	{
		m_ScrollCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::UnRegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback)
	{
		auto it = std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback);
		if(it != m_KeyboardCallbacks.end())
		{
			m_KeyboardCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KVulkanRenderWindow::UnRegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback)
	{
		auto it = std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback);
		if(it != m_MouseCallbacks.end())
		{
			m_MouseCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KVulkanRenderWindow::UnRegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback)
	{
		auto it = std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback);
		if(it != m_ScrollCallbacks.end())
		{
			m_ScrollCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}