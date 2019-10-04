#include "KVulkanRenderWindow.h"
#include "KVulkanRenderDevice.h"

KVulkanRenderWindow::KVulkanRenderWindow()
	: m_window(nullptr),
	m_device(nullptr)
{

}

KVulkanRenderWindow::~KVulkanRenderWindow()
{

}

void KVulkanRenderWindow::FramebufferResizeCallback(GLFWwindow* handle, int width, int height)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device)
	{
		window->m_device->RecreateSwapChain();
	}
}

bool KVulkanRenderWindow::Init(size_t top, size_t left, size_t width, size_t height, bool resizable)
{
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
			}
			glfwSetWindowPos(m_window, (int)top, (int)left);
			return true;
		}
	}
	else
	{
		m_window = nullptr;
	}
	return false;
}

bool KVulkanRenderWindow::UnInit()
{
	if(m_device)
	{
		m_device = nullptr;
	}
	if(m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}
	return true;
}

bool KVulkanRenderWindow::IdleUntilForeground()
{
	int width = 0, height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}
	return true;
}

bool KVulkanRenderWindow::Loop()
{
	if(m_window)
	{
		while(!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
			if(m_device)
			{
				m_device->Present();
			}
		}
		// 挂起主线程直到device持有对象被销毁完毕
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
}

bool KVulkanRenderWindow::GetPosition(size_t &top, size_t &left)
{
	if(m_window)
	{
		int xPos = -1, yPos = -1;
		glfwGetWindowPos(m_window, &xPos, &yPos);
		top = (size_t)xPos, left = (size_t)yPos;
		return true;
	}
	return false;
}

bool KVulkanRenderWindow::SetPosition(size_t top, size_t left)
{
	if(m_window)
	{
		glfwSetWindowPos(m_window, (int)top, (int)left);
		return true;
	}
	return false;
}

bool KVulkanRenderWindow::GetSize(size_t &width, size_t &height)
{
	if(m_window)
	{
		int nWidth = -1, nHeight = -1;
		glfwGetWindowSize(m_window, &nWidth, &nHeight);
		width = (size_t)nWidth, height = (size_t)nHeight;
		return true;
	}
	return false;
}

bool KVulkanRenderWindow::SetSize(size_t width, size_t height)
{
	if(m_window)
	{
		glfwSetWindowSize(m_window, (int)width, (int)height);
		return true;
	}
	return false;
}

bool KVulkanRenderWindow::SetResizable(bool resizable)
{
	return false;
}

bool KVulkanRenderWindow::IsResizable()
{
	if(m_window)
	{
		int hint = glfwGetWindowAttrib(m_window, GLFW_RESIZABLE);
		return hint == GLFW_TRUE;
	}
	return false;
}

bool KVulkanRenderWindow::SetWindowTitle(const char* pName)
{
	if(m_window)
	{
		glfwSetWindowTitle(m_window, pName);
		return true;
	}
	return false;
}