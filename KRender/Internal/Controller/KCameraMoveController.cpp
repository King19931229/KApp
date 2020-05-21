#include "KCameraMoveController.h"

KCameraMoveController::KCameraMoveController()
	: m_Camera(nullptr),
	m_Window(nullptr),
	m_Enable(true),
	m_GizmoTriggered(false),
	m_Speed(1.0f)
{
	ZeroData();
}

KCameraMoveController::~KCameraMoveController()
{
	ASSERT_RESULT(!m_Camera);
	ASSERT_RESULT(!m_Window);
}

void KCameraMoveController::ZeroData()
{
	ZERO_ARRAY_MEMORY(m_Move);
	ZERO_ARRAY_MEMORY(m_MouseDown);

	for (int i = 0; i < ARRAY_SIZE(m_MousePos); ++i)
	{
		m_MousePos[0] = 0.0f;
		m_MousePos[1] = 0.0f;
	}

	m_TouchAction = 0;
	m_LastTouchCount = 0;
	m_LastTouchDistance = 0;

	ZERO_ARRAY_MEMORY(m_Touch);

	for (int i = 0; i < ARRAY_SIZE(m_TouchPos); ++i)
	{
		m_TouchPos[i][0] = 0.0f;
		m_TouchPos[1][1] = 0.0f;
	}
}

bool KCameraMoveController::Init(KCamera* camera, IKRenderWindow* window, IKGizmoPtr gizmo)
{
	UnInit();

	if (camera && window && gizmo)
	{
		m_Camera = camera;
		m_Window = window;
		m_Gizmo = gizmo;

		ZeroData();

		m_KeyCallback = [this](InputKeyboard key, InputAction action)
		{
			int sign = 0;
			if (action == INPUT_ACTION_PRESS)
			{
				sign = 1;
			}
			else if (action == INPUT_ACTION_RELEASE)
			{
				sign = -1;
			}

			// TODO 判断key up之前是否已经key down
			switch (key)
			{
			case INPUT_KEY_W:
				m_Move[2] += sign;
				break;
			case INPUT_KEY_S:
				m_Move[2] += -sign;
				break;
			case INPUT_KEY_A:
				m_Move[0] -= sign;
				break;
			case INPUT_KEY_D:
				m_Move[0] += sign;
				break;
			case INPUT_KEY_E:
				m_Move[1] += sign;
				break;
			case INPUT_KEY_Q:
				m_Move[1] -= sign;
				break;

			default:
				break;
			}
		};

		m_MouseCallback = [this](InputMouseButton mouse, InputAction action, float xPos, float yPos)
		{
			if (action == INPUT_ACTION_PRESS)
			{
				m_MousePos[0] = xPos;
				m_MousePos[1] = yPos;

				m_MouseDown[mouse] = true;
			}
			if (action == INPUT_ACTION_RELEASE)
			{
				m_MouseDown[mouse] = false;
			}
			if (action == INPUT_ACTION_REPEAT)
			{
				if (IsEnable())
				{
					float deltaX = xPos - m_MousePos[0];
					float deltaY = yPos - m_MousePos[1];

					size_t width = 0;
					size_t height = 0;

					m_Window->GetSize(width, height);

					if (m_MouseDown[INPUT_MOUSE_BUTTON_RIGHT])
					{
						if (abs(deltaX) > 0.0001f)
						{
							m_Camera->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -glm::quarter_pi<float>() * deltaX / width);
						}
						if (abs(deltaY) > 0.0001f)
						{
							m_Camera->RotateRight(-glm::quarter_pi<float>() * deltaY / height);
						}
					}
					if (m_MouseDown[INPUT_MOUSE_BUTTON_LEFT])
					{
						const float fSpeed = 500.f * m_Speed;

						glm::vec3 forward = m_Camera->GetForward(); forward.y = 0.0f;
						if (glm::length(forward) > 0.001f)
						{
							forward = glm::normalize(forward);
							m_Camera->Move(deltaY * fSpeed * forward / (float)width);
						}

						glm::vec3 right = m_Camera->GetRight();
						right.y = 0.0f;
						if (glm::length(right) > 0.001f)
						{
							right = glm::normalize(right);
							m_Camera->Move(-deltaX * fSpeed * right / (float)height);
						}
					}
				}

				m_MousePos[0] = xPos;
				m_MousePos[1] = yPos;
			}
		};

		m_ScrollCallback = [this](float xOffset, float yOffset)
		{
			const float fSpeed = 15.0f * m_Speed;
			m_Camera->MoveForward(fSpeed * yOffset);
		};

		m_TouchCallback = [this](const std::vector<std::tuple<float, float>>& touchPositions, InputAction action)
		{
			if (action == INPUT_ACTION_REPEAT)
			{
				if (touchPositions.size() >= (size_t)m_LastTouchCount && touchPositions.size() <= 2 && touchPositions.size() > 0)
				{
					m_TouchAction = (int)touchPositions.size();

					for (size_t i = 0; i < std::min((size_t)2, touchPositions.size()); ++i)
					{
						if (!m_Touch[i])
						{
							m_TouchPos[i][0] = std::get<0>(touchPositions[i]);
							m_TouchPos[i][1] = std::get<1>(touchPositions[i]);
						}
						m_Touch[i] = true;
					}
					m_LastTouchCount = (int)touchPositions.size();

					if (m_TouchAction != 2)
					{
						m_LastTouchDistance = 0;
					}
				}
				else
				{
					m_TouchAction = 0;
					m_LastTouchDistance = 0;
					for (int i = 0; i < ARRAY_SIZE(m_TouchPos); ++i)
					{
						m_TouchPos[i][0] = 0.0f;
						m_TouchPos[1][1] = 0.0f;
					}
					ZERO_ARRAY_MEMORY(m_Touch);
					m_LastTouchCount = 0;
				}
			}
			else
			{
				m_TouchAction = 0;
				m_LastTouchDistance = 0;
				for (int i = 0; i < ARRAY_SIZE(m_TouchPos); ++i)
				{
					m_TouchPos[i][0] = 0.0f;
					m_TouchPos[1][1] = 0.0f;
				}
				ZERO_ARRAY_MEMORY(m_Touch);
			}

			if (m_TouchAction == 1 && touchPositions.size() == 1)
			{
				if (IsEnable())
				{
					float dx = std::get<0>(touchPositions[0]) - m_TouchPos[0][0];
					float dy = std::get<1>(touchPositions[0]) - m_TouchPos[0][1];

					size_t width = 0;
					size_t height = 0;
					m_Window->GetSize(width, height);

					if (abs(dx) > 0.0001f)
					{
						m_Camera->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 3.0f * -glm::quarter_pi<float>() * dx / width);
					}
					if (abs(dy) > 0.0001f)
					{
						m_Camera->RotateRight(2.0f * -glm::quarter_pi<float>() * dy / height);
					}
				}
			}
			else if (m_TouchAction == 2 && touchPositions.size() == 2)
			{
				float dx = std::get<0>(touchPositions[0]) - std::get<0>(touchPositions[1]);
				float dy = std::get<1>(touchPositions[0]) - std::get<1>(touchPositions[1]);
				const float fSpeed = 0.3f * m_Speed;

				float distance = sqrtf(dx * dx + dy * dy);

				if (m_LastTouchDistance > 0.0f)
				{
					if (m_Enable)
					{
						m_Camera->MoveForward((distance - m_LastTouchDistance) * fSpeed);
					}
				}
				m_LastTouchDistance = distance;
			}

			if (m_TouchAction)
			{
				ZERO_ARRAY_MEMORY(m_Touch);
				for (size_t i = 0; i < std::min((size_t)2, touchPositions.size()); ++i)
				{
					m_TouchPos[i][0] = std::get<0>(touchPositions[i]);
					m_TouchPos[i][1] = std::get<1>(touchPositions[i]);
					m_Touch[i] = true;
				}
			}
		};

		m_FocusCallback = [this](bool gainFocus)
		{
			ZeroData();
		};

		m_GizmoTriggerCallback = [this](bool triggered)
		{
			m_GizmoTriggered = triggered;
		};

#if defined(_WIN32)
		m_Window->RegisterKeyboardCallback(&m_KeyCallback);
		m_Window->RegisterMouseCallback(&m_MouseCallback);
		m_Window->RegisterScrollCallback(&m_ScrollCallback);
		m_Window->RegisterFocusCallback(&m_FocusCallback);
#elif defined(__ANDROID__)
		m_Window->RegisterTouchCallback(&m_TouchCallback);
#endif
		m_Gizmo->RegisterTriggerCallback(&m_GizmoTriggerCallback);

		return true;
	}

	return false;
}

bool KCameraMoveController::UnInit()
{
	if (m_Window)
	{
#if defined(_WIN32)
		m_Window->UnRegisterKeyboardCallback(&m_KeyCallback);
		m_Window->UnRegisterMouseCallback(&m_MouseCallback);
		m_Window->UnRegisterScrollCallback(&m_ScrollCallback);
		m_Window->UnRegisterFocusCallback(&m_FocusCallback);
#elif defined(__ANDROID__)
		m_Window->UnRegisterTouchCallback(&m_TouchCallback);
#endif
		
	}

	if (m_Gizmo)
	{
		m_Gizmo->UnRegisterTriggerCallback(&m_GizmoTriggerCallback);
	}

	m_Camera = nullptr;
	m_Window = nullptr;
	m_Gizmo = nullptr;

	return true;
}

bool KCameraMoveController::Update(float dt)
{
	if (m_Camera && m_Window)
	{
		const float moveSpeed = 300.0f * m_Speed;

		if (m_Move[0])
			m_Camera->MoveRight(dt * moveSpeed * m_Move[0]);
		if (m_Move[1])
			m_Camera->Move(dt * moveSpeed * m_Move[1] * glm::vec3(0, 1, 0));
		if (m_Move[2])
			m_Camera->MoveForward(dt * moveSpeed * m_Move[2]);

		size_t width = 0;
		size_t height = 0;
		m_Window->GetSize(width, height);
		m_Camera->SetPerspective(glm::radians(45.0f), width / (float)height, m_Camera->GetNear(), m_Camera->GetFar());

		return true;
	}

	return false;
}