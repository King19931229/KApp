#include "KGizmoBase.h"
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

void KGizmoBase::SetEntityColor(KEntityPtr entity, const glm::vec4& color)
{
	KDebugComponent* debugComponent = nullptr;
	if (entity && entity->GetComponent(CT_DEBUG, &debugComponent))
	{
		debugComponent->SetColor(color);
	}
};

bool KGizmoBase::CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir)
{
	if (m_Camera && m_ScreenWidth > 0 && m_ScreenHeight > 0)
	{
		glm::vec4 near = glm::vec4(
			2.0f * ((float)x / (float)m_ScreenWidth) - 1.0f,
			2.0f * ((float)y / (float)m_ScreenHeight) - 1.0f,
#ifdef GLM_FORCE_DEPTH_ZERO_TO_ONE
			0.0f,
#else
			- 1.0f,
#endif
			1.0f
		);
		glm::vec4 far = glm::vec4(near.x, near.y, 1.0f, 1.0f);

		glm::mat4 vp = m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix();
		glm::mat4 inv_vp = glm::inverse(vp);

		near = inv_vp * near;
		far = inv_vp * far;

		glm::vec3 nearPos = glm::vec3(near.x, near.y, near.z) / near.w;
		glm::vec3 farPos = glm::vec3(far.x, far.y, far.z) / far.w;

		origin = nearPos;
		dir = glm::normalize(farPos - nearPos);
		return true;
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
	for (KEntityPtr entity : m_AllEntity)
	{
		if (entity)
		{
			KRenderGlobal::Scene.Remove(entity);
			entity->UnRegisterAllComponent();
			KECSGlobal::EntityManager.ReleaseEntity(entity);
		}
	}
	return true;
}

void KGizmoBase::Enter()
{
	Leave();
	for (KEntityPtr entity : m_AllEntity)
	{
		assert(entity);
		KRenderGlobal::Scene.Add(entity);
	}
}

void KGizmoBase::Leave()
{
	for (KEntityPtr entity : m_AllEntity)
	{
		assert(entity);
		KRenderGlobal::Scene.Remove(entity);
	}
}

glm::mat3 KGizmoBase::GetRotate(KEntityPtr entity, const glm::mat3& gizmoRotate)
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
	return glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);
}

glm::vec3 KGizmoBase::TransformScale() const
{
	glm::mat4 rotate = TransformRotate();
	glm::mat4 translate = glm::translate(glm::mat4(1.0f), TransformPos());
	glm::mat4 scale = glm::inverse(translate * rotate) * m_Transform;
	return glm::vec3(scale[0][0], scale[1][1], scale[2][2]);
}

glm::mat3 KGizmoBase::TransformRotate() const
{
	glm::mat3 rotate;

	glm::vec3 xAxis = m_Transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 yAxis = m_Transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	glm::vec3 zAxis = m_Transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

	xAxis = glm::normalize(xAxis);
	yAxis = glm::normalize(yAxis);
	zAxis = glm::normalize(zAxis);

	rotate = glm::mat3(xAxis, yAxis, zAxis);

	// z轴不是x.cross(y) 是负缩放导致的
	if (glm::dot(glm::cross(xAxis, yAxis), zAxis) < 0.0f)
	{
		rotate *= glm::mat3(-1.0f);
	}

	return rotate;
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

	for (KEntityPtr entity : m_AllEntity)
	{
		if (entity)
		{
			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transform))
			{
				transform->SetPosition(transformPos);
				transform->SetRotate(GetRotate(entity, rotate));
				transform->SetScale(scale);
				KRenderGlobal::Scene.Move(entity);
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