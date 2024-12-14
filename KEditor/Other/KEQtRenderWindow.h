#pragma once
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderDevice.h"

class KEQtRenderWindow : public IKRenderWindow
{
	friend class KERenderWidget;
protected:
	IKRenderDevice* m_Device;
	void* m_HWND;
	IKUIOverlayPtr m_UIOverlay;
	IKSwapChainPtr m_SwapChain;
	bool m_bPrimary;
	std::vector<KKeyboardCallbackType*> m_KeyboardCallbacks;
	std::vector<KMouseCallbackType*> m_MouseCallbacks;
	std::vector<KScrollCallbackType*> m_ScrollCallbacks;
	std::vector<KFocusCallbackType*> m_FocusCallbacks;
	std::vector<KResizeCallbackType*> m_ResizeCallbacks;
public:
	KEQtRenderWindow();
	virtual ~KEQtRenderWindow();

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