#pragma once
#include "Interface/IKRenderWindow.h"
#include <algorithm>
#include "KBase/Publish/KTemplate.h"
#include <mutex>

class KHeadlessRenderWindow : public IKRenderWindow
{
protected:
	enum InputEventType { KEY, CHARACTER, MOUSE_MOVE, MOUSE_BUTTON, SCROLL, FOCUS };
	struct InputEvent
	{
		InputEventType type;
		int key;
		InputAction action;
		float x, y;
	};
	IKRenderDevice* m_Device = nullptr;
	IKUIOverlayPtr m_UIOverlay;
	IKSwapChainPtr m_SwapChain;
	size_t m_Top = 0, m_Left = 0, m_Width = 0, m_Height = 0;
	bool m_Initialized = false, m_Primary = false, m_Resizable = true, m_Focused = true;
	float m_MouseX = 0, m_MouseY = 0;
	std::mutex m_InputMutex, m_ViewportMutex;
	std::vector<InputEvent> m_InputEvents;
	KEditorViewport m_EditorViewport;
	std::vector<bool> m_KeyDown, m_KeyCapture;
	bool m_MouseDown[INPUT_MOUSE_BUTTON_COUNT] = {};
	bool m_MouseCapture[INPUT_MOUSE_BUTTON_COUNT] = {};
	std::vector<KKeyboardCallbackType*> m_KeyboardCallbacks;
	std::vector<KMouseCallbackType*> m_MouseCallbacks;
	std::vector<KScrollCallbackType*> m_ScrollCallbacks;
	std::vector<KFocusCallbackType*> m_FocusCallbacks;

	bool QueueInput(InputEventType type, int key = 0, InputAction action = INPUT_ACTION_UNKNOWN, float x = 0, float y = 0);
	void WindowToInput(float& x, float& y, bool& hovered);
	bool EditorKeyboardFocused();
	void OnKey(int key, InputAction action);
	void OnMouseMove(float x, float y);
	void OnMouseButton(InputMouseButton button, InputAction action);
	void OnFocus(bool focused);

public:
	KHeadlessRenderWindow();
	~KHeadlessRenderWindow() override;
	RenderWindowType GetType() override { return RENDER_WINDOW_HEADLESS; }
	bool Init(size_t top, size_t left, size_t width, size_t height, bool resizable, bool primary) override;
	bool Init(android_app* app) override { return false; }
	bool Init(void* hwnd, bool primary) override { return false; }
	bool UnInit() override;
	bool CreateUISwapChain() override;
	bool DestroyUISwapChain() override;
	android_app* GetAndroidApp() override { return nullptr; }
	void* GetHWND() override { return nullptr; }
	IKUIOverlay* GetUIOverlay() override { return m_UIOverlay.get(); }
	IKSwapChain* GetSwapChain() override { return m_SwapChain.get(); }
	bool Tick() override;
	bool Loop() override { return false; }
	bool IsMinimized() override { return !m_Width || !m_Height; }
	bool GetPosition(size_t& top, size_t& left) override { top = m_Top; left = m_Left; return true; }
	bool SetPosition(size_t top, size_t left) override { m_Top = top; m_Left = left; return true; }
	bool GetSize(size_t& width, size_t& height) override { width = m_Width; height = m_Height; return true; }
	bool GetInputSize(size_t& width, size_t& height) override;
	void SetEditorViewport(const KEditorViewport& viewport) override;
	bool SetSize(size_t width, size_t height) override;
	bool SetResizable(bool resizable) override { m_Resizable = resizable; return true; }
	bool IsResizable() override { return m_Resizable; }
	bool SetWindowTitle(const char* name) override { return true; }
	bool SetRenderDevice(IKRenderDevice* device) override { m_Device = device; return true; }

	bool RegisterKeyboardCallback(KKeyboardCallbackType* callback) override { return KTemplate::RegisterCallback(m_KeyboardCallbacks, callback); }
	bool RegisterMouseCallback(KMouseCallbackType* callback) override { return KTemplate::RegisterCallback(m_MouseCallbacks, callback); }
	bool RegisterScrollCallback(KScrollCallbackType* callback) override { return KTemplate::RegisterCallback(m_ScrollCallbacks, callback); }
	bool RegisterTouchCallback(KTouchCallbackType* callback) override { return false; }
	bool UnRegisterKeyboardCallback(KKeyboardCallbackType* callback) override { return KTemplate::UnRegisterCallback(m_KeyboardCallbacks, callback); }
	bool UnRegisterMouseCallback(KMouseCallbackType* callback) override { return KTemplate::UnRegisterCallback(m_MouseCallbacks, callback); }
	bool UnRegisterScrollCallback(KScrollCallbackType* callback) override { return KTemplate::UnRegisterCallback(m_ScrollCallbacks, callback); }
	bool UnRegisterTouchCallback(KTouchCallbackType* callback) override { return false; }
	bool RegisterFocusCallback(KFocusCallbackType* callback) override { return KTemplate::RegisterCallback(m_FocusCallbacks, callback); }
	bool UnRegisterFocusCallback(KFocusCallbackType* callback) override { return KTemplate::UnRegisterCallback(m_FocusCallbacks, callback); }

	bool InjectKey(int key, InputAction action) override;
	bool InjectCharacter(uint32_t character) override;
	bool InjectMouseMove(float x, float y) override;
	bool InjectMouseButton(InputMouseButton button, InputAction action) override;
	bool InjectScroll(float x, float y) override;
	bool InjectFocus(bool focused) override;
};
