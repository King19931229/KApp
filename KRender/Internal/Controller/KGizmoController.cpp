#include "KGizmoController.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

#include <assert.h>

KGizmoController::KGizmoController()
	: m_Gizmo(nullptr),
	m_CameraCube(nullptr),
	m_Camera(nullptr),
	m_Window(nullptr),
	m_Enable(true)
{
}

KGizmoController::~KGizmoController()
{
	ASSERT_RESULT(m_Gizmo == nullptr);
	ASSERT_RESULT(m_CameraCube == nullptr);
	ASSERT_RESULT(m_Camera == nullptr);
	ASSERT_RESULT(m_Window == nullptr);
}

bool KGizmoController::Init(IKGizmoPtr gizmo, IKCameraCubePtr cameraCube, KCamera* camera, IKRenderWindow* window)
{
	if (gizmo && camera && window)
	{
		m_Gizmo = gizmo;
		m_CameraCube = cameraCube;
		m_Camera = camera;
		m_Window = window;

		m_KeyCallback = [this](InputKeyboard key, InputAction action)
		{
			if(m_Enable)
			{
				switch (key)
				{
					case INPUT_KEY_1:
						m_Gizmo->SetType(GizmoType::GIZMO_TYPE_MOVE);
						break;

					case INPUT_KEY_2:
						m_Gizmo->SetType(GizmoType::GIZMO_TYPE_ROTATE);
						break;

					case INPUT_KEY_3:
						m_Gizmo->SetType(GizmoType::GIZMO_TYPE_SCALE);
						break;

					case INPUT_KEY_R:
					{
						if (action == INPUT_ACTION_PRESS)
						{
							GizmoManipulateMode mode = m_Gizmo->GetManipulateMode() ==
													   GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL ?
													   GizmoManipulateMode::GIZMO_MANIPULATE_WORLD :
													   GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL;
							m_Gizmo->SetManipulateMode(mode);
						}
						break;
					}

					default:
						break;
				}
			}
		};

		m_MouseCallback = [this](InputMouseButton mouse, InputAction action, float xPos, float yPos)
		{
			if(m_Enable)
			{
				if (action == INPUT_ACTION_PRESS)
				{
					m_Gizmo->OnMouseDown((unsigned int) xPos, (unsigned int) yPos);
					m_CameraCube->OnMouseDown((unsigned int)xPos, (unsigned int)yPos);
				}
				if (action == INPUT_ACTION_RELEASE)
				{
					m_Gizmo->OnMouseUp((unsigned int) xPos, (unsigned int) yPos);
					m_CameraCube->OnMouseUp((unsigned int)xPos, (unsigned int)yPos);
				}
				if (action == INPUT_ACTION_REPEAT)
				{
					m_Gizmo->OnMouseMove((unsigned int) xPos, (unsigned int) yPos);
					m_CameraCube->OnMouseMove((unsigned int)xPos, (unsigned int)yPos);
				}
			}
		};
#if defined(_WIN32)
		m_Window->RegisterKeyboardCallback(&m_KeyCallback);
		m_Window->RegisterMouseCallback(&m_MouseCallback);
#endif
		return true;
	}

	return false;
}

bool KGizmoController::UnInit()
{
	if (m_Window)
	{
#if defined(_WIN32)
		m_Window->UnRegisterKeyboardCallback(&m_KeyCallback);
		m_Window->UnRegisterMouseCallback(&m_MouseCallback);
#endif
	}

	m_Gizmo = nullptr;
	m_CameraCube = nullptr;
	m_Camera = nullptr;
	m_Window = nullptr;

	return true;
}

bool KGizmoController::Update(float dt)
{
	size_t width = 0;
	size_t height = 0;
	m_Window->GetSize(width, height);

	m_Gizmo->SetScreenSize((unsigned int)width, (unsigned int)height);
	m_CameraCube->SetScreenSize((unsigned int)width, (unsigned int)height);
	m_CameraCube->Update(dt);

	return true;
}