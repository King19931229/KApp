#pragma once
#include "Interface/IKRenderWindow.h"
#include <chrono>

#ifndef __ANDROID__
#include "GLFW/glfw3.h"
#endif

class KGLFWRenderWindow : public IKRenderWindow
{
protected:
	IKRenderDevice* m_Device;
#ifndef __ANDROID__
#if defined(_WIN32)
	void* m_HWND;
#endif
	GLFWwindow* m_Window;
	IKUIOverlayPtr m_UIOverlay;
	IKSwapChainPtr m_SwapChain;
	double m_LastMovePos[2];
	bool m_bPrimary;
	std::vector<KKeyboardCallbackType*> m_KeyboardCallbacks;
	std::vector<KMouseCallbackType*> m_MouseCallbacks;
	std::vector<KScrollCallbackType*> m_ScrollCallbacks;
	std::vector<KFocusCallbackType*> m_FocusCallbacks;
	std::vector<KResizeCallbackType*> m_ResizeCallbacks;

	static bool GLFWKeyToInputKeyboard(int key, InputKeyboard& keyboard);
	static bool GLFWMouseButtonToInputMouseButton(int mouse, InputMouseButton& mouseButton);
	static bool GLFWActionToInputAction(int action, InputAction& inputAction);

	static void FramebufferResizeCallback(GLFWwindow* handle, int width, int height);
	static void KeyboardCallback(GLFWwindow* handle, int key, int scancode, int action, int mods);
	static void MouseCallback(GLFWwindow* handle, int mouse, int action, int mods);
	static void ScrollCallback(GLFWwindow* handle, double xoffset, double yoffset);
	static void FocusCallback(GLFWwindow* handle, int focus);

	void OnMouseMove();
#endif
public:
	KGLFWRenderWindow();
	virtual ~KGLFWRenderWindow();

	virtual RenderWindowType GetType();

	virtual bool Init(size_t top, size_t left, size_t width, size_t height, bool resizable, bool primary);
	virtual bool Init(android_app* app);
	virtual bool Init(void* hwnd, bool primary);
	virtual bool UnInit();

	virtual bool CreateUISwapChain();
	virtual bool DestroyUISwapChain();

	virtual android_app* GetAndroidApp();
	virtual void* GetHWND();

	virtual IKUIOverlay* GetUIOverlay();
	virtual IKSwapChain* GetSwapChain();

	virtual bool Tick();
	virtual bool Loop();

	virtual bool IsMinimized();
	virtual bool IdleUntilForeground();

	virtual bool GetPosition(size_t &top, size_t &left);
	virtual bool SetPosition(size_t top, size_t left);

	virtual bool GetSize(size_t &width, size_t &height);
	virtual bool SetSize(size_t width, size_t height);

	virtual bool SetResizable(bool resizable);
	virtual bool IsResizable();

	virtual bool SetWindowTitle(const char* pName);
	virtual bool SetRenderDevice(IKRenderDevice* device);

	virtual bool RegisterKeyboardCallback(KKeyboardCallbackType* callback);
	virtual bool RegisterMouseCallback(KMouseCallbackType* callback);
	virtual bool RegisterScrollCallback(KScrollCallbackType* callback);
	virtual bool RegisterTouchCallback(KTouchCallbackType* callback);

	virtual bool UnRegisterKeyboardCallback(KKeyboardCallbackType* callback);
	virtual bool UnRegisterMouseCallback(KMouseCallbackType* callback);
	virtual bool UnRegisterScrollCallback(KScrollCallbackType* callback);
	virtual bool UnRegisterTouchCallback(KTouchCallbackType* callback);

	virtual bool RegisterFocusCallback(KFocusCallbackType* callback);
	virtual bool UnRegisterFocusCallback(KFocusCallbackType* callback);

	virtual bool RegisterResizeCallback(KResizeCallbackType* callback);
	virtual bool UnRegisterResizeCallback(KResizeCallbackType* callback);
};