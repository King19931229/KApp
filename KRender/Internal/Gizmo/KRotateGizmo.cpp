#include "KRotateGizmo.h"
#include "Internal/KRenderGlobal.h"
#include "Publish/KPlane.h"
#include "KBase/Interface/IKLog.h"

KRotateGizmo::KRotateGizmo()
	: KGizmoBase(),
	m_OperatorType(RotateOperator::ROTATE_NONE)
{
}

KRotateGizmo::~KRotateGizmo()
{
}

#define ALL_SCENE_ENTITY { m_XPlaneEntity, m_YPlaneEntity, m_ZPlaneEntity, m_RotateEntity }
#define NO_ENTITY {}

KRotateGizmo::RotateOperator KRotateGizmo::GetOperatorType(unsigned int x, unsigned int y, KPlane& plane, glm::vec3& intersectPos)
{
	glm::vec3 origin, dir;
	if (CalcPickRay(x, y, origin, dir))
	{
		glm::vec3 transformPos = glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);

		glm::vec3 xAxis = GetAxis(GizmoAxis::AXIS_X);
		glm::vec3 yAxis = GetAxis(GizmoAxis::AXIS_Y);
		glm::vec3 zAxis = GetAxis(GizmoAxis::AXIS_Z);

		plane.Init(transformPos, xAxis);
		if (plane.Intersect(origin, dir, intersectPos))
		{
			glm::vec3 normalizePos = (intersectPos - transformPos) / m_ScreenScaleFactor;
			float length = glm::length(normalizePos);
			if (length > 0.9f && length < 1.1f)
			{
				return RotateOperator::ROTATE_X;
			}
		}

		plane.Init(transformPos, yAxis);
		if (plane.Intersect(origin, dir, intersectPos))
		{
			glm::vec3 normalizePos = (intersectPos - transformPos) / m_ScreenScaleFactor;
			float length = glm::length(normalizePos);
			if (length > 0.9f && length < 1.1f)
			{
				return RotateOperator::ROTATE_Y;
			}
		}

		plane.Init(transformPos, zAxis);
		if (plane.Intersect(origin, dir, intersectPos))
		{
			glm::vec3 normalizePos = (intersectPos - transformPos) / m_ScreenScaleFactor;
			float length = glm::length(normalizePos);
			if (length > 0.9f && length < 1.1f)
			{
				return RotateOperator::ROTATE_Z;
			}
		}

	}
	return RotateOperator::ROTATE_NONE;
}

glm::mat3 KRotateGizmo::GetRotate(KEntityPtr entity, const glm::mat3& gizmoRotate)
{
	if (entity == m_RotateEntity)
	{
		return glm::mat3(1.0f);
	}
	else
	{
		return gizmoRotate;
	}
}

constexpr float RADIUS = 1.0f;

static const glm::vec4 X_PLANE_COLOR = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
static const glm::vec4 Y_PLANE_COLOR = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
static const glm::vec4 Z_PLANE_COLOR = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

static const glm::vec4 SELECT_PLANE_COLOR = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

static const glm::vec4 ROTATE_COLOR = glm::vec4(1.0f, 1.0f, 0.0f, 0.5f);
static const glm::vec4 EMTPY_COLOR = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

bool KRotateGizmo::Init(const KCamera* camera)
{
	UnInit();
	KGizmoBase::Init(camera);

	m_XPlaneEntity = KECSGlobal::EntityManager.CreateEntity();
	m_YPlaneEntity = KECSGlobal::EntityManager.CreateEntity();
	m_ZPlaneEntity = KECSGlobal::EntityManager.CreateEntity();

	m_RotateEntity = KECSGlobal::EntityManager.CreateEntity();

	KRenderComponent* renderComponent = nullptr;
	KDebugComponent* debugComponent = nullptr;

	// XRotate
	m_XPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCircle(glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)), RADIUS);

	m_XPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(X_PLANE_COLOR);

	m_XPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YRotate
	m_YPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCircle(glm::mat4(1.0f), RADIUS);

	m_YPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(Y_PLANE_COLOR);

	m_YPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// ZRotate
	m_ZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCircle(glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), RADIUS);

	m_ZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(Z_PLANE_COLOR);

	m_ZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	m_RotateEntity->RegisterComponent(CT_RENDER);
	m_RotateEntity->RegisterComponent(CT_TRANSFORM);
	m_RotateEntity->RegisterComponent(CT_DEBUG);

	m_AllEntity = ALL_SCENE_ENTITY;

	return true;
}

bool KRotateGizmo::UnInit()
{
	KGizmoBase::UnInit();
	m_AllEntity = NO_ENTITY;
	return true;
}

void KRotateGizmo::OnMouseDown(unsigned int x, unsigned int y)
{
	m_OperatorType = GetOperatorType(x, y, m_PickPlane, m_IntersectPos);
	if (m_OperatorType != RotateOperator::ROTATE_NONE)
	{
		m_PickTransform = m_Transform;
	}
}

void KRotateGizmo::OnMouseMove(unsigned int x, unsigned int y)
{
	if (m_OperatorType == RotateOperator::ROTATE_NONE)
	{
		KPlane plane;
		glm::vec3 intersectPos;
		RotateOperator predictType = GetOperatorType(x, y, plane, intersectPos);

		SetEntityColor(m_XPlaneEntity, X_PLANE_COLOR);
		SetEntityColor(m_YPlaneEntity, Y_PLANE_COLOR);
		SetEntityColor(m_ZPlaneEntity, Z_PLANE_COLOR);

		switch (predictType)
		{
		case KRotateGizmo::RotateOperator::ROTATE_X:
			SetEntityColor(m_XPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KRotateGizmo::RotateOperator::ROTATE_Y:
			SetEntityColor(m_YPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KRotateGizmo::RotateOperator::ROTATE_Z:
			SetEntityColor(m_ZPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KRotateGizmo::RotateOperator::ROTATE_SCREEN:
			break;
		case KRotateGizmo::RotateOperator::ROTATE_TWIN:
			break;
		case KRotateGizmo::RotateOperator::ROTATE_NONE:
			break;
		default:
			break;
		}
	}
	else
	{
		glm::vec3 origin, dir;
		if (CalcPickRay(x, y, origin, dir))
		{
			glm::vec3 transformPos = glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);
			glm::vec3 intersectPos;
			if (m_PickPlane.Intersect(origin, dir, intersectPos))
			{
				glm::vec3 v1 = m_IntersectPos - transformPos;
				glm::vec3 v2 = intersectPos - transformPos;
				glm::vec3 v3 = glm::cross(v1, v2);

				float cosTheta = glm::dot(v1, v2) / (glm::length(v1) * glm::length(v2));
				cosTheta = glm::clamp(cosTheta, -1.0f, 1.0f);

				float theta = acosf(cosTheta);

				if (glm::dot(v3, m_PickPlane.GetNormal()) < 0.0f)
				{
					theta = -theta;
				}

				SetEntityColor(m_RotateEntity, ROTATE_COLOR);

				KRenderComponent* renderComponent = nullptr;
				if (m_RotateEntity && m_RotateEntity->GetComponent(CT_RENDER, &renderComponent))
				{
					renderComponent->UnInit();
					renderComponent->InitAsArc(v1, m_PickPlane.GetNormal(), RADIUS, theta);
				}

				//KLog::Logger->Log(LL_DEBUG, "%f", theta);

				m_Transform = glm::rotate(glm::mat4(1.0f), theta, m_PickPlane.GetNormal()) * m_PickTransform;
			}

		}
	}
}

void KRotateGizmo::OnMouseUp(unsigned int x, unsigned int y)
{
	m_OperatorType = RotateOperator::ROTATE_NONE;
	SetEntityColor(m_RotateEntity, EMTPY_COLOR);
}