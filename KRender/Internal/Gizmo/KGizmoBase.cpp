#include "KGizmoBase.h"
#include "KBase/Interface/Component/IKDebugComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Publish/KMath.h"
#include "KBase/Publish/KTemplate.h"
#include "Internal/KRenderGlobal.h"

KGizmoBase::KGizmoBase()
	: m_Transform(glm::mat4(1.0f)),
	m_Mode(GizmoManipulateMode::GIZMO_MANIPULATE_WORLD),
	m_ScreenWidth(0),
	m_ScreenHeight(0),
	m_DisplayScale(1.0f),
	m_ScreenScaleFactor(1.0f),
	m_Camera(nullptr)
{
}

KGizmoBase::~KGizmoBase()
{
	assert(!m_Camera);
}

void KGizmoBase::SetEntityColor(IKEntityPtr entity, const glm::vec4& color)
{
	IKDebugComponent* debugComponent = nullptr;
	if (entity && entity->GetComponent(CT_DEBUG, &debugComponent))
	{
		debugComponent->SetColor(color);
	}
};

bool KGizmoBase::CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir)
{
	if (m_Camera && m_ScreenWidth > 0 && m_ScreenHeight > 0)
	{
		if (m_Camera->CalcPickRay(x, y, m_ScreenWidth, m_ScreenHeight, origin, dir))
		{
			return true;
		}
	}
	return false;
}

glm::vec3 KGizmoBase::GetAxis(GizmoAxis axis)
{
	glm::vec3 axisVec = glm::vec3(0.0f);
	switch (axis)
	{
	case KGizmoBase::GizmoAxis::AXIS_X:
		axisVec = glm::vec3(1.0f, 0.0f, 0.0f);
		break;
	case KGizmoBase::GizmoAxis::AXIS_Y:
		axisVec = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case KGizmoBase::GizmoAxis::AXIS_Z:
		axisVec = glm::vec3(0.0f, 0.0f, 1.0f);
		break;
	default:
		break;
	}

	if (m_Mode == GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL)
	{
		axisVec = m_Transform * glm::vec4(axisVec, 0.0f);
		axisVec = glm::normalize(axisVec);
	}

	return axisVec;
}

bool KGizmoBase::Init(const KCamera* camera)
{
	m_Camera = camera;
	return true;
}

bool KGizmoBase::UnInit()
{
	m_Camera = nullptr;
	for (IKEntityPtr entity : m_AllEntity)
	{
		if (entity)
		{
			KRenderGlobal::Scene.Remove(entity.get());
			entity->UnRegisterAllComponent();
			if (KECS::EntityManager)
			{
				KECS::EntityManager->ReleaseEntity(entity);
			}
		}
	}
	m_TransformCallback.clear();
	m_TriggerCallback.clear();
	return true;
}

void KGizmoBase::OnTriggerCallback(bool trigger)
{
	for (KGizmoTriggerCallback* callback : m_TriggerCallback)
	{
		(*callback)(trigger);
	}
}

void KGizmoBase::OnTransformCallback(const glm::mat4& transform)
{
	for (KGizmoTransformCallback* callback : m_TransformCallback)
	{
		(*callback)(m_Transform);
	}
}

bool KGizmoBase::RegisterTransformCallback(KGizmoTransformCallback* callback)
{
	return KTemplate::RegisterCallback(m_TransformCallback, callback);
}

bool KGizmoBase::UnRegisterTransformCallback(KGizmoTransformCallback* callback)
{
	return KTemplate::UnRegisterCallback(m_TransformCallback, callback);
}

bool KGizmoBase::RegisterTriggerCallback(KGizmoTriggerCallback* callback)
{
	return KTemplate::RegisterCallback(m_TriggerCallback, callback);
}

bool KGizmoBase::UnRegisterTriggerCallback(KGizmoTriggerCallback* callback)
{
	return KTemplate::UnRegisterCallback(m_TriggerCallback, callback);
}

void KGizmoBase::Enter()
{
	Leave();
	for (IKEntityPtr entity : m_AllEntity)
	{
		assert(entity);
		KRenderGlobal::Scene.Add(entity.get());
	}
}

void KGizmoBase::Leave()
{
	for (IKEntityPtr entity : m_AllEntity)
	{
		assert(entity);
		KRenderGlobal::Scene.Remove(entity.get());
	}
}

glm::mat3 KGizmoBase::GetRotate(IKEntityPtr entity, const glm::mat3& gizmoRotate)
{
	return gizmoRotate;
}

glm::vec3 KGizmoBase::GetScale()
{
	glm::vec3 scale = TransformScale();

	scale.x /= fabs(scale.x);
	scale.y /= fabs(scale.y);
	scale.z /= fabs(scale.z);

	return scale * glm::vec3(m_ScreenScaleFactor);
}

glm::vec3 KGizmoBase::TransformPos() const
{
	return KMath::ExtractPosition(m_Transform);
}

glm::vec3 KGizmoBase::TransformScale() const
{
	return KMath::ExtractScale(m_Transform);
}

glm::mat3 KGizmoBase::TransformRotate() const
{
	return KMath::ExtractRotate(m_Transform);
}

void KGizmoBase::Update()
{
	glm::vec3 transformPos = TransformPos();
	glm::mat3 rotate;

	if (m_Mode == GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL)
	{
		rotate = TransformRotate();
	}
	else
	{
		rotate = glm::mat3(1.0f);
	}

	if (m_Camera)
	{
		glm::vec4 pos = glm::vec4(transformPos, 1.0f);
		glm::vec4 tpos = m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix() * pos;
		m_ScreenScaleFactor = m_DisplayScale * 0.15f * tpos.w;
	}

	glm::vec3 scale = GetScale();

	for (IKEntityPtr entity : m_AllEntity)
	{
		if (entity)
		{
			IKTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transform))
			{
				transform->SetPosition(transformPos);
				transform->SetRotateMatrix(GetRotate(entity, rotate));
				transform->SetScale(scale);
			}
		}
	}
}

const glm::mat4& KGizmoBase::GetMatrix() const
{
	return m_Transform;
}

void KGizmoBase::SetMatrix(const glm::mat4& matrix)
{
	m_Transform = matrix;
}

GizmoManipulateMode KGizmoBase::GetManipulateMode() const
{
	return m_Mode;
}

void KGizmoBase::SetManipulateMode(GizmoManipulateMode mode)
{
	m_Mode = mode;
}

float KGizmoBase::GetDisplayScale() const
{
	return m_DisplayScale;
}

void KGizmoBase::SetDisplayScale(float scale)
{
	m_DisplayScale = scale;
}

void KGizmoBase::SetScreenSize(unsigned int width, unsigned int height)
{
	m_ScreenWidth = width;
	m_ScreenHeight = height;
}