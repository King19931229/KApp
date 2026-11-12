#include "KScaleGizmo.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KPlane.h"
#include "KBase/Publish/KMath.h"
#include "KBase/Interface/IKLog.h"

KScaleGizmo::KScaleGizmo()
	: KGizmoBase(),
	m_PickTransform(1.0f),
	m_PickScaleVec(0.0f),
	m_PickDirection(1.0f, 0.0f),
	m_OperatorType(ScaleOperator::SCALE_NONE)
{
}

KScaleGizmo::~KScaleGizmo()
{
	assert(!m_Camera);
}

#define ALL_SCENE_ENTITY { m_OriginEntity, m_XAxisEntity, m_YAxisEntity, m_ZAxisEntity, m_XCubeEntity, m_YCubeEntity, m_ZCubeEntity, m_XZPlaneEntity, m_YZPlaneEntity, m_XYPlaneEntity, m_XYZPlaneEntity }
#define NO_ENTITY {}

constexpr float ORIGIN_RADIUS = 0.08f;

constexpr float AXIS_LENGTH = 1.0f;
constexpr float AXIS_RADIUS = 0.025f;

constexpr float CUBE_HALF_EXTEND = 0.045f;

constexpr float PLANE_SIZE = 0.5f;

static bool IsAxisHit(float along, float across)
{
	return along >= 0.0f &&
		((along < AXIS_LENGTH && fabs(across) < AXIS_RADIUS) ||
		(along >= AXIS_LENGTH - CUBE_HALF_EXTEND && along <= AXIS_LENGTH + CUBE_HALF_EXTEND && fabs(across) <= CUBE_HALF_EXTEND));
}
constexpr float INNER_PLANE_SIZE = PLANE_SIZE * 0.4f;

static const glm::vec4 X_AXIS_COLOR = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
static const glm::vec4 Y_AXIS_COLOR = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
static const glm::vec4 Z_AXIS_COLOR = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

static const glm::vec4 SELECT_AXIS_COLOR = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

static const glm::vec4 PLANE_COLOR = glm::vec4(1.0f, 1.0f, 0.0f, 0.5f);
static const glm::vec4 PLANE_EMPYE_COLOR = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
static const glm::vec4 SELECT_PLANE_COLOR = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

bool KScaleGizmo::Init(const KCamera* camera)
{
	UnInit();

	KGizmoBase::Init(camera);

	m_OriginEntity = KECS::EntityManager->CreateEntity();

	m_XAxisEntity = KECS::EntityManager->CreateEntity();
	m_YAxisEntity = KECS::EntityManager->CreateEntity();
	m_ZAxisEntity = KECS::EntityManager->CreateEntity();

	m_XCubeEntity = KECS::EntityManager->CreateEntity();
	m_YCubeEntity = KECS::EntityManager->CreateEntity();
	m_ZCubeEntity = KECS::EntityManager->CreateEntity();

	m_XZPlaneEntity = KECS::EntityManager->CreateEntity();
	m_YZPlaneEntity = KECS::EntityManager->CreateEntity();
	m_XYPlaneEntity = KECS::EntityManager->CreateEntity();

	m_XYZPlaneEntity = KECS::EntityManager->CreateEntity();

	KRenderComponent* renderComponent = nullptr;

	// Origin
	m_OriginEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(KDebugUtility::CreateSphere({ glm::mat4(1.0f), ORIGIN_RADIUS }));
	renderComponent->SetUtilityColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_OriginEntity->RegisterComponent(CT_TRANSFORM);

	// XAxis
	m_XAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(KDebugUtility::CreateCylinder({
		glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)), AXIS_LENGTH, AXIS_RADIUS }));
	renderComponent->SetUtilityColor(X_AXIS_COLOR);

	m_XAxisEntity->RegisterComponent(CT_TRANSFORM);

	// YAxis
	m_YAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(KDebugUtility::CreateCylinder({ glm::mat4(1.0f), AXIS_LENGTH, AXIS_RADIUS }));
	renderComponent->SetUtilityColor(Y_AXIS_COLOR);

	m_YAxisEntity->RegisterComponent(CT_TRANSFORM);

	// ZAxis
	m_ZAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(KDebugUtility::CreateCylinder({
		glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), AXIS_LENGTH, AXIS_RADIUS }));
	renderComponent->SetUtilityColor(Z_AXIS_COLOR);

	m_ZAxisEntity->RegisterComponent(CT_TRANSFORM);

	// XCube
	m_XCubeEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(
		KDebugUtility::CreateCube({ glm::translate(glm::mat4(1.0f), glm::vec3(AXIS_LENGTH, 0.0f, 0.0f)),
			glm::vec3(CUBE_HALF_EXTEND) }));
	renderComponent->SetUtilityColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_XCubeEntity->RegisterComponent(CT_TRANSFORM);

	// YCube
	m_YCubeEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(
		KDebugUtility::CreateCube({ glm::translate(glm::mat4(1.0f), glm::vec3(0, AXIS_LENGTH, 0.0f)),
			glm::vec3(CUBE_HALF_EXTEND) }));
	renderComponent->SetUtilityColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_YCubeEntity->RegisterComponent(CT_TRANSFORM);

	// ZArrow
	m_ZCubeEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(
		KDebugUtility::CreateCube({ glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, AXIS_LENGTH)),
			glm::vec3(CUBE_HALF_EXTEND) }));
	renderComponent->SetUtilityColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_ZCubeEntity->RegisterComponent(CT_TRANSFORM);

	// XZPlane
	m_XZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(
		KDebugUtility::CreateTriangle({
		glm::vec3(0.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));
	renderComponent->SetUtilityColor(PLANE_COLOR);

	m_XZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_YZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(
		KDebugUtility::CreateTriangle({ glm::vec3(0.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));
	renderComponent->SetUtilityColor(PLANE_COLOR);

	m_YZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_XYPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(
		KDebugUtility::CreateTriangle({ glm::vec3(0.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }));
	renderComponent->SetUtilityColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

	m_XYPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// XYZPlane
	m_XYZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsDebugUtility(
		KDebugUtility::CreateTriangle({ glm::vec3(PLANE_SIZE, 0, 0), 1.414f * PLANE_SIZE, 1.414f * PLANE_SIZE, glm::vec3(-1, 1, 0), glm::vec3(-1, 0, 1) }));
	renderComponent->SetUtilityColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

	m_XYZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	m_AllEntity = ALL_SCENE_ENTITY;

	return true;
}

bool KScaleGizmo::UnInit()
{
	KGizmoBase::UnInit();
	m_AllEntity = NO_ENTITY;
	return true;
}

KScaleGizmo::ScaleOperator KScaleGizmo::GetOperatorType(unsigned int x, unsigned int y, KPlane& plane, glm::vec3& intersectPos)
{
	glm::vec3 origin, dir;
	if (CalcPickRay(x, y, origin, dir))
	{
		glm::vec3 transformPos = glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);

		glm::vec3 xAxis = GetAxis(GizmoAxis::AXIS_X);
		glm::vec3 yAxis = GetAxis(GizmoAxis::AXIS_Y);
		glm::vec3 zAxis = GetAxis(GizmoAxis::AXIS_Z);

		// XY plane
		plane.Init(transformPos, zAxis);
		if (plane.Intersect(origin, dir, intersectPos))
		{
			glm::vec3 normalizePos = (intersectPos - transformPos) / m_ScreenScaleFactor;
			normalizePos = glm::vec3(glm::dot(normalizePos, xAxis), glm::dot(normalizePos, yAxis), glm::dot(normalizePos, zAxis));
			{
				if (normalizePos.x < INNER_PLANE_SIZE && normalizePos.x > 0.0f && normalizePos.y < INNER_PLANE_SIZE && normalizePos.y > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick xyz plane");
					return ScaleOperator::SCALE_XYZ;
				}
				if (IsAxisHit(normalizePos.y, normalizePos.x))
				{
					//KLog::Logger->Log(LL_DEBUG, "pick y axis");
					return ScaleOperator::SCALE_Y;
				}
				else if (IsAxisHit(normalizePos.x, normalizePos.y))
				{
					//KLog::Logger->Log(LL_DEBUG, "pick x axis");
					return ScaleOperator::SCALE_X;
				}
				else if (normalizePos.x < PLANE_SIZE && normalizePos.x > 0.0f && normalizePos.y < PLANE_SIZE && normalizePos.y > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick xy plane");
					return ScaleOperator::SCALE_XY;
				}
			}
		}

		// YZ plane
		plane.Init(transformPos, xAxis);
		if (plane.Intersect(origin, dir, intersectPos))
		{
			glm::vec3 normalizePos = (intersectPos - transformPos) / m_ScreenScaleFactor;
			normalizePos = glm::vec3(glm::dot(normalizePos, xAxis), glm::dot(normalizePos, yAxis), glm::dot(normalizePos, zAxis));
			{
				if (normalizePos.y < INNER_PLANE_SIZE && normalizePos.y > 0.0f && normalizePos.z < INNER_PLANE_SIZE && normalizePos.z > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick xyz plane");
					return ScaleOperator::SCALE_XYZ;
				}
				else if (IsAxisHit(normalizePos.z, normalizePos.y))
				{
					//KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return ScaleOperator::SCALE_Z;
				}
				else if (IsAxisHit(normalizePos.y, normalizePos.z))
				{
					//KLog::Logger->Log(LL_DEBUG, "pick y axis");
					return ScaleOperator::SCALE_Y;
				}
				else if (normalizePos.y < PLANE_SIZE && normalizePos.y > 0.0f && normalizePos.z < PLANE_SIZE && normalizePos.z > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick yz plane");
					return ScaleOperator::SCALE_YZ;
				}
			}
		}

		// XZ plane
		plane.Init(transformPos, yAxis);
		if (plane.Intersect(origin, dir, intersectPos))
		{
			glm::vec3 normalizePos = (intersectPos - transformPos) / m_ScreenScaleFactor;
			normalizePos = glm::vec3(glm::dot(normalizePos, xAxis), glm::dot(normalizePos, yAxis), glm::dot(normalizePos, zAxis));
			{
				if (normalizePos.x < INNER_PLANE_SIZE && normalizePos.x > 0.0f && normalizePos.z < INNER_PLANE_SIZE && normalizePos.z > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick xyz plane");
					return ScaleOperator::SCALE_XYZ;
				}
				else if (IsAxisHit(normalizePos.z, normalizePos.x))
				{
					//KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return ScaleOperator::SCALE_Z;
				}
				else if (IsAxisHit(normalizePos.x, normalizePos.z))
				{
					//KLog::Logger->Log(LL_DEBUG, "pick x axis");
					return ScaleOperator::SCALE_X;
				}
				else if (normalizePos.x < PLANE_SIZE && normalizePos.x > 0.0f && normalizePos.z < PLANE_SIZE && normalizePos.z > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick xz plane");
					return ScaleOperator::SCALE_XZ;
				}
			}
		}
	}

	return ScaleOperator::SCALE_NONE;
}

//#define DEBUG_GIZMO

glm::vec3 KScaleGizmo::GetScale()
{
#ifdef DEBUG_GIZMO
	glm::vec3 scale = TransformScale();
	return scale * glm::vec3(m_ScreenScaleFactor);
#else
	return KGizmoBase::GetScale();
#endif
}

void KScaleGizmo::OnMouseDown(unsigned int x, unsigned int y)
{
	KPlane plane;
	glm::vec3 intersectPos;
	m_OperatorType = GetOperatorType(x, y, plane, intersectPos);
	if (!IsTriggered()) return;

	m_PickPos = glm::vec2(x, y);
	m_PickTransform = m_Transform;
	switch (m_OperatorType)
	{
	case ScaleOperator::SCALE_X: m_PickScaleVec = glm::vec3(1.0f, 0.0f, 0.0f); break;
	case ScaleOperator::SCALE_Y: m_PickScaleVec = glm::vec3(0.0f, 1.0f, 0.0f); break;
	case ScaleOperator::SCALE_Z: m_PickScaleVec = glm::vec3(0.0f, 0.0f, 1.0f); break;
	case ScaleOperator::SCALE_XY: m_PickScaleVec = glm::vec3(1.0f, 1.0f, 0.0f); break;
	case ScaleOperator::SCALE_XZ: m_PickScaleVec = glm::vec3(1.0f, 0.0f, 1.0f); break;
	case ScaleOperator::SCALE_YZ: m_PickScaleVec = glm::vec3(0.0f, 1.0f, 1.0f); break;
	case ScaleOperator::SCALE_XYZ: m_PickScaleVec = glm::vec3(1.0f); break;
	default: return;
	}

	m_PickDirection = glm::vec2(1.0f, 0.0f);
	if (m_OperatorType != ScaleOperator::SCALE_XYZ)
	{
		glm::vec3 worldAxis = GetAxis(GizmoAxis::AXIS_X) * m_PickScaleVec.x +
			GetAxis(GizmoAxis::AXIS_Y) * m_PickScaleVec.y +
			GetAxis(GizmoAxis::AXIS_Z) * m_PickScaleVec.z;
		glm::mat4 viewProjection = m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix();
		glm::vec4 origin = viewProjection * glm::vec4(TransformPos(), 1.0f);
		glm::vec4 axis = viewProjection * glm::vec4(worldAxis, 0.0f);
		// Project the selected axis into screen space at the gizmo origin.
		glm::vec2 direction = glm::vec2((axis.x * origin.w - origin.x * axis.w) * m_ScreenWidth,
			(axis.y * origin.w - origin.y * axis.w) * m_ScreenHeight);
		float length = glm::length(direction);
		if (fabs(origin.w) < 0.000001f || length < 0.000001f)
		{
			m_OperatorType = ScaleOperator::SCALE_NONE;
			return;
		}
		m_PickDirection = direction / length;
	}
	OnTriggerCallback(true);
}

void KScaleGizmo::OnMouseMove(unsigned int x, unsigned int y)
{
	glm::vec2 curPickPos = glm::vec2(x, y);

	if (m_OperatorType == ScaleOperator::SCALE_NONE)
	{
		KPlane plane;
		glm::vec3 intersectPos;
		ScaleOperator predictType = GetOperatorType(x, y, plane, intersectPos);

		SetEntityColor(m_XAxisEntity, X_AXIS_COLOR);
		SetEntityColor(m_YAxisEntity, Y_AXIS_COLOR);
		SetEntityColor(m_ZAxisEntity, Z_AXIS_COLOR);
		SetEntityColor(m_XZPlaneEntity, PLANE_COLOR);
		SetEntityColor(m_XYPlaneEntity, PLANE_COLOR);
		SetEntityColor(m_YZPlaneEntity, PLANE_COLOR);
		SetEntityColor(m_XYZPlaneEntity, PLANE_EMPYE_COLOR);

		switch (predictType)
		{
		case KScaleGizmo::ScaleOperator::SCALE_X:
			SetEntityColor(m_XAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KScaleGizmo::ScaleOperator::SCALE_Y:
			SetEntityColor(m_YAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KScaleGizmo::ScaleOperator::SCALE_Z:
			SetEntityColor(m_ZAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KScaleGizmo::ScaleOperator::SCALE_XY:
			SetEntityColor(m_XYPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KScaleGizmo::ScaleOperator::SCALE_XZ:
			SetEntityColor(m_XZPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KScaleGizmo::ScaleOperator::SCALE_YZ:
			SetEntityColor(m_YZPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KScaleGizmo::ScaleOperator::SCALE_XYZ:
			SetEntityColor(m_XYZPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KScaleGizmo::ScaleOperator::SCALE_NONE:
			break;
		default:
			break;
		}
	}
	else
	{
		constexpr float SCREEN_DIV_FACTOR = 100.0f;
		float len = glm::dot(curPickPos - m_PickPos, m_PickDirection) / SCREEN_DIV_FACTOR;
		glm::vec3 releatedScale = glm::vec3(1.0f) + m_PickScaleVec * len;
		if (releatedScale.x > 0.0f && releatedScale.y > 0.0f && releatedScale.z > 0.0f)
		{
			glm::vec3 newScale = KMath::ExtractScale(m_PickTransform) * releatedScale;
			constexpr float zeroExp = 0.0001f;
			if (fabs(newScale.x) > zeroExp && fabs(newScale.y) > zeroExp && fabs(newScale.z) > zeroExp)
			{
				m_Transform = m_PickTransform * glm::scale(glm::mat4(1.0f), releatedScale);
				OnTransformCallback(m_Transform);
			}
		}
	}
}

void KScaleGizmo::OnMouseUp(unsigned int x, unsigned int y)
{
	if (IsTriggered())
	{
		OnTriggerCallback(false);
	}
	m_OperatorType = ScaleOperator::SCALE_NONE;
}

bool KScaleGizmo::IsTriggered() const
{
	return m_OperatorType != ScaleOperator::SCALE_NONE;
}