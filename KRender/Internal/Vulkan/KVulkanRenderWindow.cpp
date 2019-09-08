#include "KVulkanRenderWindow.h"

KVulkanRenderWindow::KVulkanRenderWindow()
	: m_window(nullptr)
{

}

KVulkanRenderWindow::~KVulkanRenderWindow()
{

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
	if(m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		return true;
	}
	return false;
}

bool KVulkanRenderWindow::Loop()
{
	if(m_window)
	{
		while(!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
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