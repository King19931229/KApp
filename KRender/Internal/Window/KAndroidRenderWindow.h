#pragma once
#include "Interface/IKRenderWindow.h"
#include <chrono>


#if defined(__ANDROID__)
#include "android_native_app_glue.h"
#endif

class KAndroidRenderWindow : public IKRenderWindow
{
protected:
	IKRenderDevice* m_Device;
#if defined(__ANDROID__)
	android_app* m_app;
	bool m_bFocus;
#endif
	IKSwapChain* m_SwapChain;
	std::vector<KTouchCallbackType*> m_TouchCallbacks;
	std::vector<KResizeCallbackType*> m_ResizeCallbacks;
public:
	KAndroidRenderWindow();
	virtual ~KAndroidRenderWindow();

	virtual RenderWindowType GetType();

	virtual bool Init(size_t top, size_t left, size_t width, size_t height, bool resizable, bool primary);
	virtual bool Init(android_app* app);
	virtual bool Init(void* hwnd);
	virtual bool UnInit();

	virtual android_app* GetAndroidApp();
	virtual void* GetHWND();

	virtual bool SetSwapChain(IKSwapChain* swapChain);
	virtual IKSwapChain* GetSwapChain();

	virtual bool Loop();

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
#ifdef __ANDROID__
	void ShowAlert(const char* message);
	static int32_t HandleAppInput(struct android_app* app, AInputEvent* event);
	static void HandleAppCommand(android_app* app, int32_t cmd);
#endif
};