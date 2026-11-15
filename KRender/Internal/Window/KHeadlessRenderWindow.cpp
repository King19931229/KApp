#include "KHeadlessRenderWindow.h"
#include "KHeadlessSwapChain.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKUIOverlay.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KRenderThread.h"
#include "imgui.h"

namespace
{
	bool KeyToInputKeyboard(int key, InputKeyboard& keyboard)
	{
		switch (key)
		{
		case ImGuiKey_W: keyboard = INPUT_KEY_W; return true;
		case ImGuiKey_S: keyboard = INPUT_KEY_S; return true;
		case ImGuiKey_A: keyboard = INPUT_KEY_A; return true;
		case ImGuiKey_D: keyboard = INPUT_KEY_D; return true;
		case ImGuiKey_Q: keyboard = INPUT_KEY_Q; return true;
		case ImGuiKey_E: keyboard = INPUT_KEY_E; return true;
		case ImGuiKey_R: keyboard = INPUT_KEY_R; return true;
		case ImGuiKey_Enter: keyboard = INPUT_KEY_ENTER; return true;
		case ImGuiKey_LeftCtrl:
		case ImGuiKey_RightCtrl: keyboard = INPUT_KEY_CTRL; return true;
		case ImGuiKey_Delete: keyboard = INPUT_KEY_DELETE; return true;
		case ImGuiKey_Space: keyboard = INPUT_KEY_SPACE; return true;
		case ImGuiKey_1: keyboard = INPUT_KEY_1; return true;
		case ImGuiKey_2: keyboard = INPUT_KEY_2; return true;
		case ImGuiKey_3: keyboard = INPUT_KEY_3; return true;
		case ImGuiKey_4: keyboard = INPUT_KEY_4; return true;
		default: return false;
		}
	}
}

KHeadlessRenderWindow::KHeadlessRenderWindow()
	: m_KeyDown(ImGuiKey_COUNT, false), m_KeyCapture(ImGuiKey_COUNT, false)
{
}

KHeadlessRenderWindow::~KHeadlessRenderWindow()
{
	ASSERT_RESULT(!m_Initialized);
}

bool KHeadlessRenderWindow::Init(int32_t top, int32_t left, uint32_t width, uint32_t height, bool resizable, bool primary)
{
	ASSERT_RESULT(IsInGameThread());
	if (!m_Device || m_Initialized || !width || !height) return false;
	m_Top = top;
	m_Left = left;
	m_Width = width;
	m_Height = height;
	m_Resizable = resizable;
	m_Primary = primary;
	m_Focused = true;
	{
		std::lock_guard<std::mutex> lock(m_InputMutex);
		m_Initialized = true;
	}
	bool result = true;
	if (primary)
	{
		ENQUEUE_RENDER_COMMAND(HeadlessInitWindow)([this, &result]() { result = m_Device->Init(this); });
		FLUSH_RENDER_COMMAND();
	}
	if (!result)
	{
		std::lock_guard<std::mutex> lock(m_InputMutex);
		m_Initialized = false;
	}
	return result;
}

bool KHeadlessRenderWindow::UnInit()
{
	{
		std::lock_guard<std::mutex> lock(m_InputMutex);
		m_Initialized = false;
		m_InputEvents.clear();
	}
	if (m_Device)
	{
		ENQUEUE_RENDER_COMMAND(HeadlessUnInitWindow)([this]()
		{
			if (m_Primary) m_Device->UnInit();
			else
			{
				m_Device->Wait();
				DestroyUISwapChain();
			}
		});
		FLUSH_RENDER_COMMAND();
		m_Device = nullptr;
	}
	m_KeyboardCallbacks.clear();
	m_MouseCallbacks.clear();
	m_ScrollCallbacks.clear();
	m_FocusCallbacks.clear();
	std::fill(m_KeyDown.begin(), m_KeyDown.end(), false);
	std::fill(m_KeyCapture.begin(), m_KeyCapture.end(), false);
	std::fill(std::begin(m_MouseDown), std::end(m_MouseDown), false);
	std::fill(std::begin(m_MouseCapture), std::end(m_MouseCapture), false);
	m_MouseX = m_MouseY = 0;
	m_EditorViewport = {};
	return true;
}

bool KHeadlessRenderWindow::CreateUISwapChain()
{
	DestroyUISwapChain();
	m_SwapChain = IKSwapChainPtr(KNEW KHeadlessSwapChain());
	if (!m_SwapChain->Init(this))
	{
		DestroyUISwapChain();
		return false;
	}
	m_Device->CreateUIOverlay(m_UIOverlay);
	if (!m_UIOverlay->Init() || !m_UIOverlay->Resize(m_SwapChain->GetWidth(), m_SwapChain->GetHeight()))
	{
		DestroyUISwapChain();
		return false;
	}
	return true;
}

bool KHeadlessRenderWindow::DestroyUISwapChain()
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

bool KHeadlessRenderWindow::SetSize(uint32_t width, uint32_t height)
{
	ASSERT_RESULT(IsInGameThread());
	if (!m_Initialized) return false;
	ENQUEUE_RENDER_COMMAND(HeadlessResizeWindow)([this, width, height]() { m_Width = width; m_Height = height; });
	FLUSH_RENDER_COMMAND();
	return true;
}

bool KHeadlessRenderWindow::GetInputSize(uint32_t& width, uint32_t& height)
{
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
	return GetSize(width, height);
}

void KHeadlessRenderWindow::SetEditorViewport(const KEditorViewport& viewport)
{
	std::lock_guard<std::mutex> lock(m_ViewportMutex);
	m_EditorViewport = viewport;
}

bool KHeadlessRenderWindow::EditorKeyboardFocused()
{
	std::lock_guard<std::mutex> lock(m_ViewportMutex);
	return m_EditorViewport.focused;
}

void KHeadlessRenderWindow::WindowToInput(float& x, float& y, bool& hovered)
{
	x = m_MouseX;
	y = m_MouseY;
	hovered = true;
	if (KRenderGlobal::InEditor)
	{
		std::lock_guard<std::mutex> lock(m_ViewportMutex);
		x -= m_EditorViewport.x;
		y -= m_EditorViewport.y;
		hovered = m_EditorViewport.hovered && x >= 0 && y >= 0 && x < m_EditorViewport.width && y < m_EditorViewport.height;
		x = std::max(0.0f, std::min(x, std::max(0.0f, m_EditorViewport.width - 1)));
		y = std::max(0.0f, std::min(y, std::max(0.0f, m_EditorViewport.height - 1)));
	}
}

void KHeadlessRenderWindow::OnKey(int key, InputAction action)
{
	if (!m_Focused && action != INPUT_ACTION_RELEASE) return;
	const bool wasCtrl = m_KeyDown[ImGuiKey_LeftCtrl] || m_KeyDown[ImGuiKey_RightCtrl];
	if (action != INPUT_ACTION_REPEAT) m_KeyDown[key] = action == INPUT_ACTION_PRESS;
	const bool ctrl = m_KeyDown[ImGuiKey_LeftCtrl] || m_KeyDown[ImGuiKey_RightCtrl];
	if (m_UIOverlay && action != INPUT_ACTION_REPEAT)
	{
		m_UIOverlay->SetKey(key, action == INPUT_ACTION_PRESS);
		m_UIOverlay->SetKey(ImGuiMod_Ctrl, ctrl);
		m_UIOverlay->SetKey(ImGuiMod_Shift, m_KeyDown[ImGuiKey_LeftShift] || m_KeyDown[ImGuiKey_RightShift]);
		m_UIOverlay->SetKey(ImGuiMod_Alt, m_KeyDown[ImGuiKey_LeftAlt] || m_KeyDown[ImGuiKey_RightAlt]);
		m_UIOverlay->SetKey(ImGuiMod_Super, m_KeyDown[ImGuiKey_LeftSuper] || m_KeyDown[ImGuiKey_RightSuper]);
	}
	const bool controlKey = key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl;
	if (action == INPUT_ACTION_PRESS) m_KeyCapture[key] = !KRenderGlobal::InEditor || controlKey || EditorKeyboardFocused();
	bool sendToScene = m_KeyCapture[key];
	if (action == INPUT_ACTION_RELEASE) m_KeyCapture[key] = false;
	if (controlKey && wasCtrl == ctrl) sendToScene = false;
	InputKeyboard keyboard;
	if (sendToScene && KeyToInputKeyboard(key, keyboard))
	{
		for (auto callback : m_KeyboardCallbacks) (*callback)(keyboard, action);
	}
}

void KHeadlessRenderWindow::OnMouseMove(float x, float y)
{
	m_MouseX = x;
	m_MouseY = y;
	if (m_UIOverlay) m_UIOverlay->SetMousePosition((unsigned int)std::max(0.0f, x), (unsigned int)std::max(0.0f, y));
	bool hovered = false;
	WindowToInput(x, y, hovered);
	const bool captured = std::any_of(std::begin(m_MouseCapture), std::end(m_MouseCapture), [](bool value) { return value; });
	if (m_Focused && (!KRenderGlobal::InEditor || hovered || captured))
	{
		for (auto callback : m_MouseCallbacks) (*callback)(INPUT_MOUSE_BUTTON_NONE, INPUT_ACTION_REPEAT, x, y);
	}
}

void KHeadlessRenderWindow::OnMouseButton(InputMouseButton button, InputAction action)
{
	if (!m_Focused && action != INPUT_ACTION_RELEASE) return;
	m_MouseDown[button] = action == INPUT_ACTION_PRESS;
	if (m_UIOverlay) m_UIOverlay->SetMouseDown(button, m_MouseDown[button]);
	float x = 0, y = 0;
	bool hovered = false;
	WindowToInput(x, y, hovered);
	if (action == INPUT_ACTION_PRESS) m_MouseCapture[button] = !KRenderGlobal::InEditor || hovered;
	const bool sendToScene = m_MouseCapture[button];
	if (action == INPUT_ACTION_RELEASE) m_MouseCapture[button] = false;
	if (sendToScene)
	{
		for (auto callback : m_MouseCallbacks) (*callback)(button, action, x, y);
	}
}

void KHeadlessRenderWindow::OnFocus(bool focused)
{
	if (!focused)
	{
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_COUNT; ++key)
		{
			if (m_KeyDown[key]) OnKey(key, INPUT_ACTION_RELEASE);
		}
		for (int button = 0; button < INPUT_MOUSE_BUTTON_COUNT; ++button)
		{
			if (m_MouseDown[button]) OnMouseButton((InputMouseButton)button, INPUT_ACTION_RELEASE);
		}
	}
	m_Focused = focused;
	if (m_UIOverlay) m_UIOverlay->SetFocus(focused);
	for (auto callback : m_FocusCallbacks) (*callback)(focused);
}

bool KHeadlessRenderWindow::Tick()
{
	ASSERT_RESULT(IsInGameThread());
	std::vector<InputEvent> events;
	{
		std::lock_guard<std::mutex> lock(m_InputMutex);
		if (!m_Initialized) return false;
		events.swap(m_InputEvents);
	}
	if (KRenderGlobal::InEditor && !EditorKeyboardFocused())
	{
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_COUNT; ++key)
		{
			if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl) continue;
			InputKeyboard keyboard;
			if (m_KeyCapture[key] && KeyToInputKeyboard(key, keyboard))
			{
				for (auto callback : m_KeyboardCallbacks) (*callback)(keyboard, INPUT_ACTION_RELEASE);
			}
			m_KeyCapture[key] = false;
		}
	}
	for (const InputEvent& event : events)
	{
		switch (event.type)
		{
		case KEY: OnKey(event.key, event.action); break;
		case CHARACTER: if (m_Focused && m_UIOverlay) m_UIOverlay->SetCharacter((uint32_t)event.key); break;
		case MOUSE_MOVE: OnMouseMove(event.x, event.y); break;
		case MOUSE_BUTTON: OnMouseButton((InputMouseButton)event.key, event.action); break;
		case FOCUS: OnFocus(event.key != 0); break;
		case SCROLL:
			if (m_Focused)
			{
				if (m_UIOverlay) m_UIOverlay->SetMouseScroll(event.x, event.y);
				float x = 0, y = 0;
				bool hovered = false;
				WindowToInput(x, y, hovered);
				if (!KRenderGlobal::InEditor || hovered)
				{
					for (auto callback : m_ScrollCallbacks) (*callback)(event.x, event.y);
				}
			}
			break;
		}
	}
	return true;
}

bool KHeadlessRenderWindow::QueueInput(InputEventType type, int key, InputAction action, float x, float y)
{
	std::lock_guard<std::mutex> lock(m_InputMutex);
	if (!m_Initialized) return false;
	m_InputEvents.push_back({ type, key, action, x, y });
	return true;
}

bool KHeadlessRenderWindow::InjectKey(int key, InputAction action)
{
	if (key < ImGuiKey_NamedKey_BEGIN || key >= ImGuiKey_MouseLeft || action < INPUT_ACTION_RELEASE || action >= INPUT_ACTION_UNKNOWN) return false;
	return QueueInput(KEY, key, action);
}

bool KHeadlessRenderWindow::InjectCharacter(uint32_t character)
{
	if (character > 0x10FFFF || (character >= 0xD800 && character <= 0xDFFF)) return false;
	return QueueInput(CHARACTER, (int)character);
}

bool KHeadlessRenderWindow::InjectMouseMove(float x, float y)
{
	return QueueInput(MOUSE_MOVE, 0, INPUT_ACTION_UNKNOWN, x, y);
}

bool KHeadlessRenderWindow::InjectMouseButton(InputMouseButton button, InputAction action)
{
	if (button < INPUT_MOUSE_BUTTON_LEFT || button >= INPUT_MOUSE_BUTTON_COUNT || (action != INPUT_ACTION_PRESS && action != INPUT_ACTION_RELEASE)) return false;
	return QueueInput(MOUSE_BUTTON, button, action);
}

bool KHeadlessRenderWindow::InjectScroll(float x, float y)
{
	return QueueInput(SCROLL, 0, INPUT_ACTION_UNKNOWN, x, y);
}

bool KHeadlessRenderWindow::InjectFocus(bool focused)
{
	return QueueInput(FOCUS, focused ? 1 : 0);
}
