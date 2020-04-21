#include "KEEntityManipulator.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Publish/KMath.h"
#include <assert.h>

KEEntityManipulator::KEEntityManipulator()
	: m_Gizmo(nullptr),
	m_Window(nullptr),
	m_Camera(nullptr),
	m_Scene(nullptr),
	m_SelectType(SelectType::SELECT_TYPE_SINGLE),
	m_Enable(false),
	m_PreviousTransform(glm::mat4(1.0f))
{
	m_MouseDownPos[0] = 0.0f;
	m_MouseDownPos[1] = 0.0f;
}

KEEntityManipulator::~KEEntityManipulator()
{
	assert(!m_Gizmo);
}

bool KEEntityManipulator::Init(IKGizmoPtr gizmo, IKRenderWindow* window, const KCamera* camera, IKRenderScene* scene)
{
	if (gizmo && window && camera && scene)
	{
		m_Gizmo = gizmo;
		m_Window = window;
		m_Camera = camera;
		m_Scene = scene;

		m_KeyboardCallback = [this](InputKeyboard key, InputAction action)
		{
			OnKeyboardListen(key, action);
		};
		m_MouseCallback = [this](InputMouseButton key, InputAction action, float x, float y)
		{
			OnMouseListen(key, action, x, y);
		};
		m_TransformCallback = [this](const glm::mat4& transform)
		{
			OnGizmoTransformChange(transform);
		};

		m_Window->RegisterKeyboardCallback(&m_KeyboardCallback);
		m_Window->RegisterMouseCallback(&m_MouseCallback);
		m_Gizmo->RegisterTransformCallback(&m_TransformCallback);

		return true;
	}
	return false;
}

bool KEEntityManipulator::UnInit()
{
	if (m_Gizmo)
	{
		m_Gizmo->UnRegisterTransformCallback(&m_TransformCallback);
		m_Gizmo = nullptr;
	}
	if (m_Window)
	{
		m_Window->UnRegisterKeyboardCallback(&m_KeyboardCallback);
		m_Window->UnRegisterMouseCallback(&m_MouseCallback);
		m_Window = nullptr;
	}

	m_Camera = nullptr;
	m_Scene = nullptr;

	return true;
}

void KEEntityManipulator::OnKeyboardListen(InputKeyboard key, InputAction action)
{
	if (key == INPUT_KEY_CTRL)
	{
		if (action == INPUT_ACTION_PRESS)
		{
			m_SelectType = SelectType::SELECT_TYPE_MULTI;
		}
		else
		{
			m_SelectType = SelectType::SELECT_TYPE_SINGLE;
		}
	}
}

void KEEntityManipulator::OnMouseListen(InputMouseButton key, InputAction action, float x, float y)
{
	if (key == INPUT_MOUSE_BUTTON_LEFT)
	{
		if (action == INPUT_ACTION_PRESS)
		{
			m_MouseDownPos[0] = x;
			m_MouseDownPos[1] = y;
		}
		else if (action == INPUT_ACTION_RELEASE)
		{
			if (m_Gizmo->IsTriggered())
			{
				return;
			}
			// 点击操作
			if (x == m_MouseDownPos[0] && y == m_MouseDownPos[1])
			{
				IKEntityPtr entity = nullptr;

				size_t width = 0;
				size_t height = 0;
				m_Window->GetSize(width, height);

				if (m_SelectType == SelectType::SELECT_TYPE_SINGLE)
				{
					m_Entities.clear();
				}

				if (m_Scene->CloestPick(*m_Camera, (size_t)x, (size_t)y,
					width, height, entity))
				{
					if (m_SelectType == SelectType::SELECT_TYPE_SINGLE)
					{
						m_Entities.insert(entity);						
					}
					else if (m_SelectType == SelectType::SELECT_TYPE_MULTI)
					{
						auto it = m_Entities.find(entity);
						if (it == m_Entities.end())
						{
							m_Entities.insert(entity);
						}
						else
						{
							m_Entities.erase(it);
						}
					}
					UpdateGizmoTransform();
				}
			}
			// 拉框多选
			else
			{

			}
		}
	}

	if (m_Entities.empty())
	{
		m_Gizmo->Leave();
	}
	else
	{
		m_Gizmo->Enter();
	}
}

void KEEntityManipulator::OnGizmoTransformChange(const glm::mat4& transform)
{
	if (m_Entities.size() > 0)
	{
		glm::vec3 deltaTranslate = KMath::ExtractPosition(transform) - KMath::ExtractPosition(m_PreviousTransform);
		glm::mat3 deltaRotate = KMath::ExtractRotate(transform) * glm::inverse(KMath::ExtractRotate(m_PreviousTransform));
		glm::vec3 deltaScale = KMath::ExtractScale(transform) / KMath::ExtractScale(m_PreviousTransform);

		/*
		// 以下方法无效 由于整体变换 = T * R * S
		// 不能直接用逆矩阵的提取
		glm::mat4 deltaTransform = transform * glm::inverse(m_PreviousTransform);
		glm::vec3 deltaTranslate = KMath::ExtractPosition(deltaTransform);
		glm::mat3 deltaRotate = KMath::ExtractRotate(deltaTransform);
		glm::vec3 deltaScale = KMath::ExtractScale(deltaTransform);
		*/

		GizmoManipulateMode mode = GetManipulateMode();
		GizmoType type = GetGizmoType();

		for (IKEntityPtr entity : m_Entities)
		{
			IKTransformComponent* transformComponent = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transformComponent))
			{
				if (type == GizmoType::GIZMO_TYPE_MOVE)
				{
					glm::vec3 pos = deltaTranslate + transformComponent->GetPosition();
					transformComponent->SetPosition(pos);
				}
				else if (type == GizmoType::GIZMO_TYPE_SCALE)
				{
					glm::vec3 scale = deltaScale * transformComponent->GetScale();
					transformComponent->SetScale(scale);
				}
				else if (type == GizmoType::GIZMO_TYPE_ROTATE)
				{
					if (mode == GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL)
					{
						glm::mat3 rotate = deltaRotate * glm::mat3_cast(transformComponent->GetRotate());
						transformComponent->SetRotate(rotate);
					}
					else
					{
						
						glm::vec3 gizmoPos = KMath::ExtractPosition(transform);
						glm::vec3 releatedPos = transformComponent->GetPosition() - gizmoPos;
						releatedPos = glm::mat4(deltaRotate) * glm::vec4(releatedPos, 1.0f);
						transformComponent->SetPosition(glm::vec3(releatedPos) + gizmoPos);
						/*
						glm::mat4 mat = glm::translate(glm::mat4(1.0f), gizmoPos) * glm::mat4(deltaRotate) * glm::translate(glm::mat4(1.0f), -gizmoPos);
						glm::vec4 pos = mat * glm::vec4(transformComponent->GetPosition(), 1.0f);
						transformComponent->SetPosition(glm::vec3(pos));
						*/
					}
				}
				// 暂时先用这种方法更新场景图
				m_Scene->Move(entity);
			}
		}
	}

	m_PreviousTransform = transform;
}

void KEEntityManipulator::UpdateGizmoTransform()
{
	if (m_Entities.empty())
		return;

	if (m_Entities.size() > 1)
	{
		glm::vec3 pos = glm::vec3(0.0f);
		int num = 0;
		for (IKEntityPtr entity : m_Entities)
		{
			IKTransformComponent* transformComponent = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transformComponent))
			{
				pos += transformComponent->GetPosition();
				++num;
			}
		}
		if (num)
		{
			pos = pos / (float)num;
			m_Gizmo->SetMatrix(glm::translate(glm::mat4(1.0f), pos));
		}
	}
	else
	{
		IKEntityPtr entity = *m_Entities.begin();
		IKTransformComponent* transformComponent = nullptr;
		if (entity->GetComponent(CT_TRANSFORM, &transformComponent))
		{
			m_Gizmo->SetMatrix(transformComponent->GetFinal());
		}
	}

	m_PreviousTransform = m_Gizmo->GetMatrix();
}

SelectType KEEntityManipulator::GetSelectType() const
{
	return m_SelectType;
}

bool KEEntityManipulator::SetSelectType(SelectType type)
{
	m_SelectType = type;
	return true;
}

GizmoType KEEntityManipulator::GetGizmoType() const
{
	if (m_Gizmo)
	{
		GizmoType type = m_Gizmo->GetType();
		return type;
	}
	assert(false && "gizmo not set");
	return GizmoType::GIZMO_TYPE_MOVE;
}

bool KEEntityManipulator::SetGizmoType(GizmoType type)
{
	if (m_Gizmo)
	{
		m_Gizmo->SetType(type);
		return true;
	}
	return false;
}

GizmoManipulateMode KEEntityManipulator::GetManipulateMode() const
{
	if (m_Gizmo)
	{
		GizmoManipulateMode mode = m_Gizmo->GetManipulateMode();
		return mode;
	}
	assert(false && "gizmo not set");
	return GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL;
}

bool KEEntityManipulator::SetManipulateMode(GizmoManipulateMode mode)
{
	if (m_Gizmo)
	{
		m_Gizmo->SetManipulateMode(mode);
		return true;
	}
	return false;
}