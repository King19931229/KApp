#include "KGLFWRenderWindow.h"
#include <limits>
#include "Interface/IKRenderDevice.h"
#include "Interface/IKUIOverlay.h"
#include "Interface/IKSwapChain.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Publish/KTemplate.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KRenderThread.h"
#include "imgui.h"

#ifdef _WIN32
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#endif

KGLFWRenderWindow::KGLFWRenderWindow()
	: m_Device(nullptr),
	m_SwapChain(nullptr),
	m_bPrimary(true)
{
#ifndef __ANDROID__
	m_LastMovePos[0] = -1.0f;
	m_LastMovePos[1] = -1.0f;
	m_Window = nullptr;
#if defined(_WIN32)
	m_HWND = NULL;
#endif
#endif
}

KGLFWRenderWindow::~KGLFWRenderWindow()
{
	ASSERT_RESULT(!m_UIOverlay);
	ASSERT_RESULT(!m_SwapChain);
}

RenderWindowType KGLFWRenderWindow::GetType()
{
	return RENDER_WINDOW_GLFW;
}

#ifndef __ANDROID__
static ImGuiKey GLFWKeyToImGuiKey(int key)
{
	if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) return (ImGuiKey)(ImGuiKey_A + key - GLFW_KEY_A);
	if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) return (ImGuiKey)(ImGuiKey_0 + key - GLFW_KEY_0);
	if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F24) return (ImGuiKey)(ImGuiKey_F1 + key - GLFW_KEY_F1);
	if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) return (ImGuiKey)(ImGuiKey_Keypad0 + key - GLFW_KEY_KP_0);
	switch (key)
	{
	case GLFW_KEY_TAB: return ImGuiKey_Tab;
	case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
	case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
	case GLFW_KEY_UP: return ImGuiKey_UpArrow;
	case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
	case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
	case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
	case GLFW_KEY_HOME: return ImGuiKey_Home;
	case GLFW_KEY_END: return ImGuiKey_End;
	case GLFW_KEY_INSERT: return ImGuiKey_Insert;
	case GLFW_KEY_DELETE: return ImGuiKey_Delete;
	case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
	case GLFW_KEY_SPACE: return ImGuiKey_Space;
	case GLFW_KEY_ENTER: return ImGuiKey_Enter;
	case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
	case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
	case GLFW_KEY_COMMA: return ImGuiKey_Comma;
	case GLFW_KEY_MINUS: return ImGuiKey_Minus;
	case GLFW_KEY_PERIOD: return ImGuiKey_Period;
	case GLFW_KEY_SLASH: return ImGuiKey_Slash;
	case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
	case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
	case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
	case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
	case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
	case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
	case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
	case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
	case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
	case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
	case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
	case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
	case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
	case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
	case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
	case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
	case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
	case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
	case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
	case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
	case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
	case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
	case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
	case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
	case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
	case GLFW_KEY_MENU: return ImGuiKey_Menu;
	default: return ImGuiKey_None;
	}
}

void KGLFWRenderWindow::FramebufferResizeCallback(GLFWwindow* handle, int width, int height)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (window)
	{
		for (KResizeCallbackType* resizeCB : window->m_ResizeCallbacks)
		{
			if (resizeCB)
			{
				(*resizeCB)(width, height);
			}
		}
	}
}

bool KGLFWRenderWindow::GLFWKeyToInputKeyboard(int key, InputKeyboard& board)
{
	switch (key)
	{
	case GLFW_KEY_W:
		board = INPUT_KEY_W;
		return true;

	case GLFW_KEY_S:
		board = INPUT_KEY_S;
		return true;

	case GLFW_KEY_A:
		board = INPUT_KEY_A;
		return true;

	case GLFW_KEY_D:
		board = INPUT_KEY_D;
		return true;

	case GLFW_KEY_Q:
		board = INPUT_KEY_Q;
		return true;

	case GLFW_KEY_E:
		board = INPUT_KEY_E;
		return true;

	case GLFW_KEY_R:
		board = INPUT_KEY_R;
		return true;

	case GLFW_KEY_1:
		board = INPUT_KEY_1;
		return true;
	case GLFW_KEY_2:
		board = INPUT_KEY_2;
		return true;
	case GLFW_KEY_3:
		board = INPUT_KEY_3;
		return true;
	case GLFW_KEY_4:
		board = INPUT_KEY_4;
		return true;

	case GLFW_KEY_ENTER:
		board = INPUT_KEY_ENTER;
		return true;

	case GLFW_KEY_LEFT_CONTROL:
	case GLFW_KEY_RIGHT_CONTROL:
		board = INPUT_KEY_CTRL;
		return true;

	case GLFW_KEY_DELETE:
		board = INPUT_KEY_DELETE;
		return true;

	case GLFW_KEY_SPACE:
		board = INPUT_KEY_SPACE;
		return true;

	default:
		return false;
	}
}

bool KGLFWRenderWindow::GLFWMouseButtonToInputMouseButton(int mouse, InputMouseButton& mouseButton)
{
	switch (mouse)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		mouseButton = INPUT_MOUSE_BUTTON_LEFT;
		return true;

	case GLFW_MOUSE_BUTTON_MIDDLE:
		mouseButton = INPUT_MOUSE_BUTTON_MIDDLE;
		return true;

	case GLFW_MOUSE_BUTTON_RIGHT:
		mouseButton = INPUT_MOUSE_BUTTON_RIGHT;
		return true;

	default:
		return false;
	}
}

bool KGLFWRenderWindow::GLFWActionToInputAction(int action, InputAction& inputAction)
{
	switch (action)
	{
	case GLFW_RELEASE:
		inputAction = INPUT_ACTION_RELEASE;
		return true;

	case GLFW_PRESS:
		inputAction = INPUT_ACTION_PRESS;
		return true;

	case GLFW_REPEAT:
		inputAction = INPUT_ACTION_REPEAT;
		return true;

	default:
		return false;
	}
}

void KGLFWRenderWindow::KeyboardCallback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (window && window->m_UIOverlay && action != GLFW_REPEAT)
	{
		IKUIOverlay* ui = window->m_UIOverlay.get();
		ui->SetKey(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
		ui->SetKey(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
		ui->SetKey(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
		ui->SetKey(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
		ImGuiKey imguiKey = GLFWKeyToImGuiKey(key);
		if (imguiKey != ImGuiKey_None) ui->SetKey(imguiKey, action == GLFW_PRESS);
	}
	bool sendToScene = true;
	if (window && KRenderGlobal::InEditor && key >= 0 && key <= GLFW_KEY_LAST)
	{
		if (action == GLFW_PRESS) window->m_EditorKeyCapture[key] = key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL || window->EditorKeyboardFocused();
		sendToScene = window->m_EditorKeyCapture[key];
		if (action == GLFW_RELEASE) window->m_EditorKeyCapture[key] = false;
	}
	if (window && sendToScene && window->m_Device && !window->m_KeyboardCallbacks.empty())
	{
		InputKeyboard keyboard;
		InputAction inputAction;

		if (GLFWKeyToInputKeyboard(key, keyboard) && GLFWActionToInputAction(action, inputAction))
		{
			for (auto it = window->m_KeyboardCallbacks.begin(),
				itEnd = window->m_KeyboardCallbacks.end();
				it != itEnd; ++it)
			{
				KKeyboardCallbackType& callback = (*(*it));
				callback(keyboard, inputAction);
			}
		}
	}
}

void KGLFWRenderWindow::CharacterCallback(GLFWwindow* handle, unsigned int character)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (window && window->m_UIOverlay) window->m_UIOverlay->SetCharacter(character);
}

void KGLFWRenderWindow::MouseCallback(GLFWwindow* handle, int mouse, int action, int mods)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (!window) return;
	if (action == GLFW_PRESS || action == GLFW_RELEASE) window->OnMouseMove();
	double xpos = 0, ypos = 0;
	glfwGetCursorPos(window->m_Window, &xpos, &ypos);
	float inputX = 0, inputY = 0;
	bool hovered = false;
	window->WindowToInput(xpos, ypos, inputX, inputY, hovered);
	InputMouseButton mouseButton;
	if (window->m_UIOverlay && GLFWMouseButtonToInputMouseButton(mouse, mouseButton))
	{
		int width = 0, height = 0, framebufferWidth = 0, framebufferHeight = 0;
		glfwGetWindowSize(handle, &width, &height);
		glfwGetFramebufferSize(handle, &framebufferWidth, &framebufferHeight);
		window->m_UIOverlay->SetMousePosition((unsigned int)std::max(0.0, xpos * framebufferWidth / std::max(width, 1)), (unsigned int)std::max(0.0, ypos * framebufferHeight / std::max(height, 1)));
		window->m_UIOverlay->SetMouseDown(mouseButton, action == GLFW_PRESS);
	}
	bool sendToScene = true;
	if (KRenderGlobal::InEditor && mouse >= 0 && mouse <= GLFW_MOUSE_BUTTON_LAST)
	{
		if (action == GLFW_PRESS) window->m_EditorMouseCapture[mouse] = hovered;
		sendToScene = window->m_EditorMouseCapture[mouse];
		if (action == GLFW_RELEASE) window->m_EditorMouseCapture[mouse] = false;
	}
	if (sendToScene && window->m_Device && !window->m_MouseCallbacks.empty())
	{
		InputAction inputAction;

		if (GLFWMouseButtonToInputMouseButton(mouse, mouseButton) && GLFWActionToInputAction(action, inputAction))
		{
			for (auto it = window->m_MouseCallbacks.begin(),
				itEnd = window->m_MouseCallbacks.end();
				it != itEnd; ++it)
			{
				KMouseCallbackType& callback = (*(*it));
				callback(mouseButton, inputAction, inputX, inputY);
			}
		}
	}
}

void KGLFWRenderWindow::ScrollCallback(GLFWwindow* handle, double xoffset, double yoffset)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	if (!window) return;
	if (window->m_UIOverlay) window->m_UIOverlay->SetMouseScroll((float)xoffset, (float)yoffset);
	double x = 0, y = 0;
	glfwGetCursorPos(handle, &x, &y);
	float localX, localY;
	bool hovered;
	window->WindowToInput(x, y, localX, localY, hovered);
	if ((!KRenderGlobal::InEditor || hovered) && window->m_Device && !window->m_ScrollCallbacks.empty())
	{
		for (auto it = window->m_ScrollCallbacks.begin(),
			itEnd = window->m_ScrollCallbacks.end();
			it != itEnd; ++it)
		{
			KScrollCallbackType& callback = (*(*it));
			callback((float)xoffset, (float)yoffset);
		}
	}
}

void KGLFWRenderWindow::FocusCallback(GLFWwindow* handle, int focus)
{
	KGLFWRenderWindow* window = (KGLFWRenderWindow*)glfwGetWindowUserPointer(handle);
	bool gainFocus = focus == GLFW_TRUE;
	if (window && window->m_UIOverlay) window->m_UIOverlay->SetFocus(gainFocus);
	if (window && KRenderGlobal::InEditor && !gainFocus)
	{
		for (int key = 0; key <= GLFW_KEY_LAST; ++key)
		{
			if (window->m_EditorKeyCapture[key]) KeyboardCallback(handle, key, 0, GLFW_RELEASE, 0);
		}
		for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
		{
			if (window->m_EditorMouseCapture[button]) MouseCallback(handle, button, GLFW_RELEASE, 0);
		}
	}
	if (window && window->m_Device && !window->m_FocusCallbacks.empty())
	{
		for (auto it = window->m_FocusCallbacks.begin(),
			itEnd = window->m_FocusCallbacks.end();
			it != itEnd; ++it)
		{
			KFocusCallbackType& callback = (*(*it));
			callback(gainFocus);
		}
	}
}

void KGLFWRenderWindow::OnMouseMove()
{
	if (m_UIOverlay || !m_MouseCallbacks.empty())
	{
		double xpos = 0, ypos = 0;
		glfwGetCursorPos(m_Window, &xpos, &ypos);

		constexpr float exp = 0.001f;
		if (fabs(xpos - m_LastMovePos[0]) > exp || fabs(ypos - m_LastMovePos[1]) > exp)
		{
			int width = 0, height = 0, framebufferWidth = 0, framebufferHeight = 0;
			glfwGetWindowSize(m_Window, &width, &height);
			glfwGetFramebufferSize(m_Window, &framebufferWidth, &framebufferHeight);
			if (m_UIOverlay) m_UIOverlay->SetMousePosition((unsigned int)std::max(0.0, xpos * framebufferWidth / std::max(width, 1)), (unsigned int)std::max(0.0, ypos * framebufferHeight / std::max(height, 1)));
			float inputX, inputY;
			bool hovered;
			WindowToInput(xpos, ypos, inputX, inputY, hovered);
			bool captured = std::any_of(std::begin(m_EditorMouseCapture), std::end(m_EditorMouseCapture), [](bool value) { return value; });
			if (!KRenderGlobal::InEditor || hovered || captured)
			for (auto it = m_MouseCallbacks.begin(), itEnd = m_MouseCallbacks.end();
				it != itEnd; ++it)
			{
				KMouseCallbackType& callback = (*(*it));
				callback(INPUT_MOUSE_BUTTON_NONE, INPUT_ACTION_REPEAT, inputX, inputY);
			}

			m_LastMovePos[0] = xpos;
			m_LastMovePos[1] = ypos;
		}
	}
}

void KGLFWRenderWindow::WindowToInput(double x, double y, float& localX, float& localY, bool& hovered)
{
	int width = 0, height = 0, framebufferWidth = 0, framebufferHeight = 0;
	glfwGetWindowSize(m_Window, &width, &height);
	glfwGetFramebufferSize(m_Window, &framebufferWidth, &framebufferHeight);
	localX = (float)(x * framebufferWidth / std::max(width, 1));
	localY = (float)(y * framebufferHeight / std::max(height, 1));
	hovered = true;
	if (KRenderGlobal::InEditor)
	{
		std::lock_guard<std::mutex> lock(m_ViewportMutex);
		localX -= m_EditorViewport.x;
		localY -= m_EditorViewport.y;
		hovered = m_EditorViewport.hovered && localX >= 0 && localY >= 0 && localX < m_EditorViewport.width && localY < m_EditorViewport.height;
		localX = std::max(0.0f, std::min(localX, std::max(0.0f, m_EditorViewport.width - 1)));
		localY = std::max(0.0f, std::min(localY, std::max(0.0f, m_EditorViewport.height - 1)));
	}
}

bool KGLFWRenderWindow::EditorKeyboardFocused()
{
	std::lock_guard<std::mutex> lock(m_ViewportMutex);
	return m_EditorViewport.focused;
}
#endif

bool KGLFWRenderWindow::Init(int32_t top, int32_t left, uint32_t width, uint32_t height, bool resizable, bool primary)
{
	if (width > uint32_t((std::numeric_limits<int32_t>::max)()) || height > uint32_t((std::numeric_limits<int32_t>::max)())) return false;
#ifndef __ANDROID__
	ASSERT_RESULT(m_Device);
	ASSERT_RESULT(IsInGameThread());

	m_bPrimary = primary;
	m_Window = nullptr;

	if (m_bPrimary)
	{
		if (glfwInit() != GLFW_TRUE) return false;
	}

	{
		glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow((int)width, (int)height, "Vulkan window", nullptr, nullptr);
		if (m_Window)
		{
			glfwSetWindowUserPointer(m_Window, this);
			{
				glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
				glfwSetKeyCallback(m_Window, KeyboardCallback);
				glfwSetCharCallback(m_Window, CharacterCallback);
				glfwSetMouseButtonCallback(m_Window, MouseCallback);
				glfwSetScrollCallback(m_Window, ScrollCallback);
				glfwSetWindowFocusCallback(m_Window, FocusCallback);
			}
			glfwSetWindowPos(m_Window, (int)top, (int)left);
#ifdef	_WIN32
			m_HWND = glfwGetWin32Window(m_Window);
#endif
			m_LastMovePos[0] = -1.0f;
			m_LastMovePos[1] = -1.0f;

			if (m_bPrimary)
			{
				bool result = false;
				ENQUEUE_RENDER_COMMAND(GLFW_InitWindow)([this, &result]()
				{
					result = m_Device->Init(this);
				});
				FLUSH_RENDER_COMMAND();
				return result;
			}

			return true;
		}
	}
#endif
	return false;
}

bool KGLFWRenderWindow::Init(android_app* app)
{
	assert(false && "GLFW window can not inited by android_app");
	return false;
}

bool KGLFWRenderWindow::Init(void* hwnd, bool primary)
{
	// https://github.com/glfw/glfw/issues/25
	assert(false && "GLFW window can not inited by hwnd");
	return false;
}

bool KGLFWRenderWindow::UnInit()
{
#ifndef __ANDROID__
	if (m_Device)
	{
		if (m_bPrimary)
		{
			ENQUEUE_RENDER_COMMAND(GLFW_UnInitWindow)([this]()
			{
				return m_Device->UnInit();
			});
			FLUSH_RENDER_COMMAND();
		}
		m_Device = nullptr;
	}

	if (m_Window)
	{
		glfwDestroyWindow(m_Window);
		if (m_bPrimary)
		{
			glfwTerminate();
		}
		m_Window = nullptr;
	}

	m_KeyboardCallbacks.clear();
	m_MouseCallbacks.clear();
	m_ScrollCallbacks.clear();
	m_FocusCallbacks.clear();
	m_ResizeCallbacks.clear();
#endif
	return true;
}

bool KGLFWRenderWindow::CreateUISwapChain()
{
	DestroyUISwapChain();

	m_Device->CreateSwapChain(m_SwapChain);
	ASSERT_RESULT(m_SwapChain->Init(this));

	m_Device->CreateUIOverlay(m_UIOverlay);
	m_UIOverlay->Init();
	m_UIOverlay->Resize(m_SwapChain->GetWidth(), m_SwapChain->GetHeight());

	return true;
}

bool KGLFWRenderWindow::DestroyUISwapChain()
{
	if (m_UIOverlay)
	{
		m_UIOverlay->UnInit();
		m_UIOverlay = nullptr;
	}

	if (m_SwapChain)
	{
		m_SwapChain->UnInit();
		m_SwapChain = nullptr;
	}

	return true;
}

IKUIOverlay* KGLFWRenderWindow::GetUIOverlay()
{
	return m_UIOverlay ? m_UIOverlay.get() : nullptr;
}

IKSwapChain* KGLFWRenderWindow::GetSwapChain()
{
	return m_SwapChain ? m_SwapChain.get() : nullptr;
}

android_app* KGLFWRenderWindow::GetAndroidApp()
{
	assert(false && "GLFW window can not get android handle");
	return nullptr;
}

void* KGLFWRenderWindow::GetHWND()
{
#ifndef __ANDROID__
#	ifdef _WIN32
	return m_HWND;
#	endif
#endif
	assert(false && "none win32 platform can not get hwnd handle");
	return nullptr;
}

bool KGLFWRenderWindow::IsMinimized()
{
#ifndef __ANDROID__
	if (m_Window)
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		if (width == 0 || height == 0)
		{
			return true;
		}
	}
#endif
	return false;
}

bool KGLFWRenderWindow::IdleUntilForeground()
{
	/*
#ifndef __ANDROID__
	if (m_Window)
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_Window, &width, &height);
			glfwWaitEvents();
		}
		return true;
	}
#endif
	return false;
	*/
	return false;
}

bool KGLFWRenderWindow::Tick()
{
#ifndef __ANDROID__
	if (m_Window)
	{
		if (glfwWindowShouldClose(m_Window))
		{
			return false;
		}
		glfwPollEvents();
		if (KRenderGlobal::InEditor && !EditorKeyboardFocused())
		{
			for (int key = 0; key <= GLFW_KEY_LAST; ++key)
			{
				if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) continue;
				InputKeyboard keyboard;
				if (m_EditorKeyCapture[key] && GLFWKeyToInputKeyboard(key, keyboard))
				{
					for (auto callback : m_KeyboardCallbacks) (*callback)(keyboard, INPUT_ACTION_RELEASE);
				}
				m_EditorKeyCapture[key] = false;
			}
		}
		OnMouseMove();
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::Loop()
{
	return false;
}

bool KGLFWRenderWindow::GetPosition(int32_t &top, int32_t &left)
{
#ifndef __ANDROID__
	if (m_Window)
	{
		int xPos = -1, yPos = -1;
		glfwGetWindowPos(m_Window, &xPos, &yPos);
		top = (int32_t)xPos, left = (int32_t)yPos;
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetPosition(int32_t top, int32_t left)
{
#ifndef __ANDROID__
	if (m_Window)
	{
		glfwSetWindowPos(m_Window, (int)top, (int)left);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::GetSize(uint32_t &width, uint32_t &height)
{
#ifndef __ANDROID__
	if (m_Window)
	{
		int nWidth = -1, nHeight = -1;
		glfwGetFramebufferSize(m_Window, &nWidth, &nHeight);
		if (nWidth < 0 || nHeight < 0) return false;
		width = (uint32_t)nWidth;
		height = (uint32_t)nHeight;
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::GetInputSize(uint32_t& width, uint32_t& height)
{
#ifndef __ANDROID__
	if (KRenderGlobal::InEditor)
	{
		std::lock_guard<std::mutex> lock(m_ViewportMutex);
		if (m_EditorViewport.width > 0 && m_EditorViewport.height > 0)
		{
			width = (uint32_t)m_EditorViewport.width;
			height = (uint32_t)m_EditorViewport.height;
			return true;
		}
	}
#endif
	return GetSize(width, height);
}

void KGLFWRenderWindow::SetEditorViewport(const KEditorViewport& viewport)
{
#ifndef __ANDROID__
	std::lock_guard<std::mutex> lock(m_ViewportMutex);
	m_EditorViewport = viewport;
#endif
}

bool KGLFWRenderWindow::SetSize(uint32_t width, uint32_t height)
{
	if (width > uint32_t((std::numeric_limits<int32_t>::max)()) || height > uint32_t((std::numeric_limits<int32_t>::max)())) return false;
#ifndef __ANDROID__
	if (m_Window)
	{
		glfwSetWindowSize(m_Window, (int)width, (int)height);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetResizable(bool resizable)
{
	return false;
}

bool KGLFWRenderWindow::IsResizable()
{
#ifndef __ANDROID__
	if (m_Window)
	{
		int hint = glfwGetWindowAttrib(m_Window, GLFW_RESIZABLE);
		return hint == GLFW_TRUE;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetWindowTitle(const char* pName)
{
#ifndef __ANDROID__
	if (m_Window)
	{
		glfwSetWindowTitle(m_Window, pName);
		return true;
	}
#endif
	return false;
}

bool KGLFWRenderWindow::SetRenderDevice(IKRenderDevice* device)
{
	m_Device = device;
	return true;
}

bool KGLFWRenderWindow::RegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::RegisterCallback(m_KeyboardCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::RegisterCallback(m_MouseCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::RegisterCallback(m_ScrollCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterTouchCallback(KTouchCallbackType* callback)
{
	assert(false && "GLFW window can not registered touch callback");
	return false;
}

bool KGLFWRenderWindow::UnRegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::UnRegisterCallback(m_KeyboardCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::UnRegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::UnRegisterCallback(m_MouseCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::UnRegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::UnRegisterCallback(m_ScrollCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::UnRegisterTouchCallback(KTouchCallbackType *callback)
{
	assert(false && "GLFW window can not unregistered touch callback");
	return false;
}

bool KGLFWRenderWindow::RegisterFocusCallback(KFocusCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::RegisterCallback(m_FocusCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::UnRegisterFocusCallback(KFocusCallbackType* callback)
{
#ifndef __ANDROID__
	return KTemplate::UnRegisterCallback(m_FocusCallbacks, callback);
#endif
	return false;
}

bool KGLFWRenderWindow::RegisterResizeCallback(KResizeCallbackType* callback)
{
	return KTemplate::RegisterCallback(m_ResizeCallbacks, callback);
}

bool KGLFWRenderWindow::UnRegisterResizeCallback(KResizeCallbackType* callback)
{
	return KTemplate::UnRegisterCallback(m_ResizeCallbacks, callback);
}
