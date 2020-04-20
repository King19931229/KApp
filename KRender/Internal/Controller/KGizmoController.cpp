#include "KGizmoController.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

#include <assert.h>

KGizmoController::KGizmoController()
	: m_Gizmo(nullptr),
	m_Camera(nullptr),
	m_Window(nullptr),
	m_Enable(true)
{
}

KGizmoController::~KGizmoController()
{
	ASSERT_RESULT(m_Gizmo == nullptr);
	ASSERT_RESULT(m_Camera == nullptr);
	ASSERT_RESULT(m_Window == nullptr);
}

bool KGizmoController::Init(IKGizmoPtr gizmo, KCamera* camera, IKRenderWindow* window)
{
	if (gizmo && camera && window)
	{
		m_Gizmo = gizmo;
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
				size_t width = 0;
				size_t height = 0;
				m_Window->GetSize(width, height);
				m_Gizmo->SetScreenSize((unsigned int) width, (unsigned int) height);

				if (action == INPUT_ACTION_PRESS)
				{
#if 0
					IKEntityPtr entity = nullptr;
					if (mouse == INPUT_MOUSE_BUTTON_LEFT)
					{
						if (KRenderGlobal::Scene.CloestPick(*m_Camera, (size_t) xPos, (size_t) yPos,
															width, height, entity))
						{
							IKTransformComponent* compoent = nullptr;
							if (entity->GetComponent(CT_TRANSFORM, &compoent))
							{
								const glm::mat4& transform = compoent->GetFinal();
								m_Gizmo->SetMatrix(transform);
							}
						}
					}
#endif
					m_Gizmo->OnMouseDown((unsigned int) xPos, (unsigned int) yPos);
				}

				if (action == INPUT_ACTION_RELEASE)
				{
					m_Gizmo->OnMouseUp((unsigned int) xPos, (unsigned int) yPos);
				}
				if (action == INPUT_ACTION_REPEAT)
				{
					m_Gizmo->OnMouseMove((unsigned int) xPos, (unsigned int) yPos);
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
	m_Camera = nullptr;
	m_Window = nullptr;

	return true;
}

bool KGizmoController::Update(float dt)
{
	return true;
}