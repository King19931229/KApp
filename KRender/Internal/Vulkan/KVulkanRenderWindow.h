#pragma once
#include "Interface/IKRenderWindow.h"
#include <chrono>

#ifndef __ANDROID__
#include "GLFW/glfw3.h"
#else
#include "android_native_app_glue.h"
#endif

class KVulkanRenderDevice;

class KVulkanRenderWindow : IKRenderWindow
{	
	KVulkanRenderDevice* m_device;
#ifndef __ANDROID__
	GLFWwindow* m_window;
	std::vector<KKeyboardCallbackType*> m_KeyboardCallbacks;
	std::vector<KMouseCallbackType*> m_MouseCallbacks;
	std::vector<KScrollCallbackType*> m_ScrollCallbacks;
	bool m_MouseDown[INPUT_MOUSE_BUTTON_COUNT];

	static bool GLFWKeyToInputKeyboard(int key, InputKeyboard& keyboard);
	static bool GLFWMouseButtonToInputMouseButton(int mouse, InputMouseButton& mouseButton);
	static bool GLFWActionToInputAction(int action, InputAction& inputAction);

	static void FramebufferResizeCallback(GLFWwindow* handle, int width, int height);
	static void KeyboardCallback(GLFWwindow* handle, int key, int scancode, int action, int mods);
	static void MouseCallback(GLFWwindow* handle, int mouse, int action, int mods);
	static void ScrollCallback(GLFWwindow* handle, double xoffset, double yoffset);

	void OnMouseMove();
#else
    ANativeWindow* m_window;
	android_app* m_app;
	bool m_bFocus;
#endif

public:
	KVulkanRenderWindow();
	virtual ~KVulkanRenderWindow();

	virtual bool Init(size_t top, size_t left, size_t width, size_t height, bool resizable);
	virtual bool Init(android_app* app);
	virtual bool UnInit();

	virtual bool Loop();

	virtual bool GetPosition(size_t &top, size_t &left);
	virtual bool SetPosition(size_t top, size_t left);

	virtual bool GetSize(size_t &width, size_t &height);
	virtual bool SetSize(size_t width, size_t height);

	virtual bool SetResizable(bool resizable);
	virtual bool IsResizable();

	virtual bool SetWindowTitle(const char* pName);

	virtual bool RegisterKeyboardCallback(KKeyboardCallbackType* callback);
	virtual bool RegisterMouseCallback(KMouseCallbackType* callback);
	virtual bool RegisterScrollCallback(KScrollCallbackType* callback);

	virtual bool UnRegisterKeyboardCallback(KKeyboardCallbackType* callback);
	virtual bool UnRegisterMouseCallback(KMouseCallbackType* callback);
	virtual bool UnRegisterScrollCallback(KScrollCallbackType* callback);

	inline void SetVulkanDevice(KVulkanRenderDevice* device) { m_device = device; }
#if defined(_WIN32)
	inline GLFWwindow* GetGLFWwindow() { return m_window; }
#else
    inline android_app* GetAndroidApp() { return m_app; }
    void ShowAlert(const char* message);
	static int32_t HandleAppInput(struct android_app* app, AInputEvent* event);
	static void HandleAppCommand(android_app* app, int32_t cmd);
#endif
	bool IdleUntilForeground();
};