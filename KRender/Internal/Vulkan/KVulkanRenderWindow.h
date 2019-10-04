#pragma once
#include "Interface/IKRenderWindow.h"
#include "GLFW/glfw3.h"

class KVulkanRenderDevice;

class KVulkanRenderWindow : IKRenderWindow
{
	GLFWwindow* m_window;
	KVulkanRenderDevice* m_device;
	static void FramebufferResizeCallback(GLFWwindow* handle, int width, int height);
public:
	KVulkanRenderWindow();
	virtual ~KVulkanRenderWindow();

	virtual bool Init(size_t top, size_t left, size_t width, size_t height, bool resizable);
	virtual bool UnInit();

	virtual bool Loop();

	virtual bool GetPosition(size_t &top, size_t &left);
	virtual bool SetPosition(size_t top, size_t left);

	virtual bool GetSize(size_t &width, size_t &height);
	virtual bool SetSize(size_t width, size_t height);

	virtual bool SetResizable(bool resizable);
	virtual bool IsResizable();

	virtual bool SetWindowTitle(const char* pName);

	inline GLFWwindow* GetGLFWwindow() { return m_window; }
	inline void SetVulkanDevice(KVulkanRenderDevice* device) { m_device = device; }

	bool IdleUntilForeground();
};