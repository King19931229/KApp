#include "KUIOverlayController.h"
#include <assert.h>

KUIOverlayController::KUIOverlayController()
	: m_UIOverlay(nullptr),
	m_Window(nullptr)
{
}

KUIOverlayController::~KUIOverlayController()
{
	ASSERT_RESULT(m_UIOverlay == nullptr);
	ASSERT_RESULT(m_Window == nullptr);
}

bool KUIOverlayController::Init(IKUIOverlayPtr ui, IKRenderWindow* window)
{
	if (ui && window)
	{
		m_UIOverlay = ui;
		m_Window = window;

		m_MouseCallback = [this](InputMouseButton mouse, InputAction action, float xPos, float yPos)
		{
			if (m_Enable)
			{
				m_UIOverlay->SetMousePosition((unsigned int)xPos, (unsigned int)yPos);

				if (action == INPUT_ACTION_PRESS)
				{
					m_UIOverlay->SetMouseDown(mouse, true);
				}
				if (action == INPUT_ACTION_RELEASE)
				{
					m_UIOverlay->SetMouseDown(mouse, false);
				}
			}
		};

		m_TouchCallback = [this](const std::vector<std::tuple<float, float>>& touchPositions, InputAction action)
		{
			if (touchPositions.size() == 1)
			{
				if (m_Enable)
				{
					float xPos = std::get<0>(touchPositions[0]);
					float yPos = std::get<1>(touchPositions[0]);

					m_UIOverlay->SetMousePosition((unsigned int)xPos, (unsigned int)yPos);

					if (action == INPUT_ACTION_PRESS)
					{
						m_UIOverlay->SetMouseDown(INPUT_MOUSE_BUTTON_LEFT, true);
					}
					if (action == INPUT_ACTION_RELEASE)
					{
						m_UIOverlay->SetMouseDown(INPUT_MOUSE_BUTTON_LEFT, false);
					}
				}
			}
		};

#if defined(_WIN32)
		m_Window->RegisterMouseCallback(&m_MouseCallback);
#elif defined(__ANDROID__)
		m_Window->RegisterTouchCallback(&m_TouchCallback);
#endif

		return true;
	}
	return false;
}

bool KUIOverlayController::UnInit()
{
	if (m_UIOverlay && m_Window)
	{
#if defined(_WIN32)
		m_Window->UnRegisterMouseCallback(&m_MouseCallback);
#elif defined(__ANDROID__)
		m_Window->UnRegisterTouchCallback(&m_TouchCallback);
#endif
	}

	m_UIOverlay = nullptr;
	m_Window = nullptr;

	return true;
}

bool KUIOverlayController::Update(float dt)
{
	return true;
}