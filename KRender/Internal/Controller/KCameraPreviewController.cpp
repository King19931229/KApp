#include "KCameraPreviewController.h"

EXPORT_DLL IKCameraPreviewControllerPtr CreateCameraPreviewController()
{
	return IKCameraPreviewControllerPtr(KNEW KCameraPreviewController());
}

KCameraPreviewController::KCameraPreviewController()
	: m_Camera(nullptr),
	m_Window(nullptr),
	m_PreviewCenter(glm::vec3(0.0f)),
	m_PreviewDistance(100.0f),
	m_Enable(true)
{
	ZeroData();
}

KCameraPreviewController::~KCameraPreviewController()
{
	ASSERT_RESULT(!m_Camera);
	ASSERT_RESULT(!m_Window);
}

void KCameraPreviewController::ZeroData()
{
	for (int i = 0; i < ARRAY_SIZE(m_MousePos); ++i)
	{
		m_MousePos[0] = 0.0f;
		m_MousePos[1] = 0.0f;
	}
	ZERO_ARRAY_MEMORY(m_MouseDown);
}

bool KCameraPreviewController::Init(KCamera* camera, IKRenderWindow* window)
{
	UnInit();

	if (camera && window)
	{
		m_Camera = camera;
		m_Window = window;

		ZeroData();

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
				if (m_Enable)
				{
					float deltaX = xPos - m_MousePos[0];
					float deltaY = yPos - m_MousePos[1];

					size_t width = 0;
					size_t height = 0;

					m_Window->GetSize(width, height);

					const float fSpeed = 1.0f;

					if (m_MouseDown[INPUT_MOUSE_BUTTON_LEFT])
					{
						if (abs(deltaX) > 0.0001f)
						{
							m_Camera->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -glm::quarter_pi<float>() * deltaX * fSpeed / width);
						}
						if (abs(deltaY) > 0.0001f)
						{
							m_Camera->RotateRight(-glm::quarter_pi<float>() * deltaY * fSpeed / height);
						}
					}
				}

				m_MousePos[0] = xPos;
				m_MousePos[1] = yPos;
			}
		};

		m_ScrollCallback = [this](float xOffset, float yOffset)
		{
			if (m_Enable)
			{
				const float fSpeed = 3.0f;
				m_PreviewDistance = std::min(std::max(0.01f, m_PreviewDistance - yOffset * fSpeed), 1000.0f);
			}
		};

		m_Window->RegisterMouseCallback(&m_MouseCallback);
		m_Window->RegisterScrollCallback(&m_ScrollCallback);
	}

	return true;
}

bool KCameraPreviewController::UnInit()
{
	if (m_Window)
	{
		m_Window->UnRegisterMouseCallback(&m_MouseCallback);
		m_Window->UnRegisterScrollCallback(&m_ScrollCallback);
	}
	return true;
}

void KCameraPreviewController::SetEnable(bool enable)
{
	m_Enable = enable;
}

void KCameraPreviewController::SetPreviewCenter(const glm::vec3& center)
{
	m_PreviewCenter = center;
}

void KCameraPreviewController::Update()
{
	if (m_Camera)
	{
		m_Camera->SetPosition(m_PreviewCenter - m_Camera->GetForward() * m_PreviewDistance);
		size_t width = 0;
		size_t height = 0;
		m_Window->GetSize(width, height);
		m_Camera->SetPerspective(glm::radians(45.0f), width / (float)height, m_Camera->GetNear(), m_Camera->GetFar());
	}
}