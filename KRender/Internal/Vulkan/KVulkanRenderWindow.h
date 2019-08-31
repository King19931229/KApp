#pragma once
#include "Interface/IKRenderWindow.h"
#include "GLFW/glfw3.h"

class KVulkanRenderWindow : IKRenderWindow
{
	GLFWwindow* m_window;
public:
	KVulkanRenderWindow();
	virtual ~KVulkanRenderWindow();

	virtual bool Init(size_t top, size_t left, size_t width, size_t height);
	virtual bool UnInit();

	virtual bool Loop();

	virtual bool GetPosition(size_t &top, size_t &left);
	virtual bool SetPosition(size_t top, size_t left);

	virtual bool GetSize(size_t &width, size_t &height);
	virtual bool SetSize(size_t width, size_t height);
};