#pragma once
#include "Interface/IKRenderConfig.h"
#include "KBase/Publish/KInput.h"

#include <functional>
#include <tuple>

typedef std::function<void(InputKeyboard key, InputAction action)> KKeyboardCallbackType;
typedef std::function<void(InputMouseButton key, InputAction action, float x, float y)> KMouseCallbackType;
typedef std::function<void(float x, float y)> KScrollCallbackType;
typedef std::function<void(const std::vector<std::tuple<float, float>>& touchPositions, InputAction action)> KTouchCallbackType;

// 安卓专用
struct ANativeWindow;
struct android_app;

enum RenderWindowType
{
	RENDER_WINDOW_GLFW,
	RENDER_WINDOW_ANDROID_NATIVE,
	RENDER_WINDOW_EXTERNAL
};

struct IKRenderWindow
{
	virtual ~IKRenderWindow() {}

	virtual RenderWindowType GetType() = 0;

	virtual bool Init(size_t top, size_t left, size_t width, size_t height, bool resizable) = 0;
	virtual bool Init(android_app* app) = 0;
	virtual bool Init(void* HWND) = 0;
	virtual bool UnInit() = 0;

	virtual android_app* GetAndroidApp() = 0;
	virtual void* GetHWND() = 0;

	virtual bool Loop() = 0;

	virtual bool IdleUntilForeground() = 0;

	virtual bool GetPosition(size_t &top, size_t &left) = 0;
	virtual bool SetPosition(size_t top, size_t left) = 0;

	virtual bool GetSize(size_t &width, size_t &height) = 0;
	virtual bool SetSize(size_t width, size_t height) = 0;

	virtual bool SetResizable(bool resizable) = 0;
	virtual bool IsResizable() = 0;

	virtual bool SetWindowTitle(const char* pName) = 0;
	virtual bool SetRenderDevice(IKRenderDevice* device) = 0;

	virtual bool RegisterKeyboardCallback(KKeyboardCallbackType* callback) = 0;
	virtual bool RegisterMouseCallback(KMouseCallbackType* callback) = 0;
	virtual bool RegisterScrollCallback(KScrollCallbackType* callback) = 0;
	virtual bool RegisterTouchCallback(KTouchCallbackType* callback) = 0;

	virtual bool UnRegisterKeyboardCallback(KKeyboardCallbackType* callback) = 0;
	virtual bool UnRegisterMouseCallback(KMouseCallbackType* callback) = 0;
	virtual bool UnRegisterScrollCallback(KScrollCallbackType* callback) = 0;
	virtual bool UnRegisterTouchCallback(KTouchCallbackType* callback) = 0;
};

EXPORT_DLL IKRenderWindowPtr CreateRenderWindow(RenderWindowType windowType);