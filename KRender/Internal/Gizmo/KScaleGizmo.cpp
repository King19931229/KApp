#include "KScaleGizmo.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KPlane.h"
#include "KBase/Interface/IKLog.h"

KScaleGizmo::KScaleGizmo()
	: KGizmoBase(),
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
	KDebugComponent* debugComponent = nullptr;

	// Origin
	m_OriginEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(KMeshUtility::CreateSphere({ glm::mat4(1.0f), ORIGIN_RADIUS }));

	m_OriginEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_OriginEntity->RegisterComponent(CT_TRANSFORM);

	// XAxis
	m_XAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(KMeshUtility::CreateCylinder({
		glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)), AXIS_LENGTH, AXIS_RADIUS }));

	m_XAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(X_AXIS_COLOR);

	m_XAxisEntity->RegisterComponent(CT_TRANSFORM);

	// YAxis
	m_YAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(KMeshUtility::CreateCylinder({ glm::mat4(1.0f), AXIS_LENGTH, AXIS_RADIUS }));

	m_YAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(Y_AXIS_COLOR);

	m_YAxisEntity->RegisterComponent(CT_TRANSFORM);

	// ZAxis
	m_ZAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(KMeshUtility::CreateCylinder({
		glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), AXIS_LENGTH, AXIS_RADIUS }));

	m_ZAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(Z_AXIS_COLOR);

	m_ZAxisEntity->RegisterComponent(CT_TRANSFORM);

	// XCube
	m_XCubeEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateCube({ glm::translate(glm::mat4(1.0f), glm::vec3(AXIS_LENGTH, 0.0f, 0.0f)),
			glm::vec3(CUBE_HALF_EXTEND) }));

	m_XCubeEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_XCubeEntity->RegisterComponent(CT_TRANSFORM);

	// YCube
	m_YCubeEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateCube({ glm::translate(glm::mat4(1.0f), glm::vec3(0, AXIS_LENGTH, 0.0f)),
			glm::vec3(CUBE_HALF_EXTEND) }));

	m_YCubeEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_YCubeEntity->RegisterComponent(CT_TRANSFORM);

	// ZArrow
	m_ZCubeEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateCube({ glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, AXIS_LENGTH)),
			glm::vec3(CUBE_HALF_EXTEND) }));

	m_ZCubeEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_ZCubeEntity->RegisterComponent(CT_TRANSFORM);

	// XZPlane
	m_XZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateTriangle({
		glm::vec3(0.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));

	m_XZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(PLANE_COLOR);

	m_XZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_YZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateTriangle({ glm::vec3(0.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));

	m_YZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(PLANE_COLOR);

	m_YZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_XYPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateTriangle({ glm::vec3(0.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }));

	m_XYPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

	m_XYPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// XYZPlane
	m_XYZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateTriangle({ glm::vec3(PLANE_SIZE, 0, 0), 1.414f * PLANE_SIZE, 1.414f * PLANE_SIZE, glm::vec3(-1, 1, 0), glm::vec3(-1, 0, 1) }));

	m_XYZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

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
				if (fabs(normalizePos.x) < AXIS_RADIUS && normalizePos.y < AXIS_LENGTH && normalizePos.y >= 0)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick y axis");
					return ScaleOperator::SCALE_Y;
				}
				else if (fabs(normalizePos.y) < AXIS_RADIUS && normalizePos.x < AXIS_LENGTH && normalizePos.x >= 0)
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
				else if (fabs(normalizePos.y) < AXIS_RADIUS && normalizePos.z < AXIS_LENGTH && normalizePos.z >= 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return ScaleOperator::SCALE_Z;
				}
				else if (fabs(normalizePos.z) < AXIS_RADIUS && normalizePos.y < AXIS_LENGTH && normalizePos.y >= 0.0f)
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
				else if (fabs(normalizePos.x) < AXIS_RADIUS && normalizePos.z < AXIS_LENGTH && normalizePos.z >= 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return ScaleOperator::SCALE_Z;
				}
				else if (fabs(normalizePos.z) < AXIS_RADIUS && normalizePos.x < AXIS_LENGTH && normalizePos.x >= 0.0f)
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
	m_OperatorType = GetOperatorType(x, y, m_PickPlane, m_IntersectPos);
	m_PickPos = glm::vec2(x, y);
	if (IsTriggered())
	{
		OnTriggerCallback(true);
	}
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
		glm::vec3 origin, dir;
		if (CalcPickRay(x, y, origin, dir))
		{
			glm::vec3 transformPos = TransformPos();
			glm::vec3 intersectPos;
			if (m_PickPlane.Intersect(origin, dir, intersectPos))
			{
				glm::vec3 xAxis = GetAxis(GizmoAxis::AXIS_X);
				glm::vec3 yAxis = GetAxis(GizmoAxis::AXIS_Y);
				glm::vec3 zAxis = GetAxis(GizmoAxis::AXIS_Z);

				glm::mat4 rotate = TransformRotate();
				glm::mat4 translate = glm::translate(glm::mat4(1.0f), transformPos);
				glm::mat4 scale = glm::scale(glm::mat4(1.0f), TransformScale());

				glm::vec3 scaleVec = glm::vec3(0.0f);
				glm::vec3 worldScaleVec = glm::vec3(0.0f);

				switch (m_OperatorType)
				{
				case KScaleGizmo::ScaleOperator::SCALE_X:
					scaleVec = glm::vec3(1.0f, 0.0f, 0.0f);
					worldScaleVec = xAxis;
					break;
				case KScaleGizmo::ScaleOperator::SCALE_Y:
					scaleVec = glm::vec3(0.0f, 1.0f, 0.0f);
					worldScaleVec = yAxis;
					break;
				case KScaleGizmo::ScaleOperator::SCALE_Z:
					scaleVec = glm::vec3(0.0f, 0.0f, 1.0f);
					worldScaleVec = zAxis;
					break;
				case KScaleGizmo::ScaleOperator::SCALE_XY:
					scaleVec = glm::vec3(1.0f, 1.0f, 0.0f);
					worldScaleVec = xAxis + yAxis;
					break;
				case KScaleGizmo::ScaleOperator::SCALE_XZ:
					scaleVec = glm::vec3(1.0f, 0.0f, 1.0f);
					worldScaleVec = xAxis + zAxis;
					break;
				case KScaleGizmo::ScaleOperator::SCALE_YZ:
					scaleVec = glm::vec3(0.0f, 1.0f, 1.0f);
					worldScaleVec = yAxis + zAxis;
					break;
				case KScaleGizmo::ScaleOperator::SCALE_XYZ:
					scaleVec = glm::vec3(1.0f, 1.0f, 1.0f);
					// worldScaleVec = xAxis + yAxis + zAxis;
					break;
				case KScaleGizmo::ScaleOperator::SCALE_NONE:
					break;
				default:
					break;
				}

				glm::mat4 newScale = scale;
				glm::vec3 releatedScale = glm::vec3(1.0f);

				constexpr float SCREEN_DIV_FACTOR = 100.0f;

				if (m_OperatorType != ScaleOperator::SCALE_XYZ)
				{
					float len = glm::distance(m_PickPos, curPickPos);
					if (glm::dot((intersectPos - m_IntersectPos), worldScaleVec) < 0.0f)
					{
						len = -len;
					}
					len /= SCREEN_DIV_FACTOR;
					releatedScale = (glm::vec3(1.0f, 1.0f, 1.0f) - scaleVec) + scaleVec * (1.0f + len);
				}
				else
				{
					float len = (curPickPos.x - m_PickPos.x);// - (curPickPos.y - m_PickPos.y);
					len /= SCREEN_DIV_FACTOR;
					releatedScale = scaleVec * (1.0f + len);
				}

				if (releatedScale.x > 0.0f && releatedScale.y > 0.0f && releatedScale.z > 0.0f)
				{
					newScale = scale * glm::scale(glm::mat4(1.0f), releatedScale);

					constexpr float zeroExp = 0.0001f;
					if (fabs(newScale[0][0]) > zeroExp &&
						fabs(newScale[1][1]) > zeroExp &&
						fabs(newScale[2][2]) > zeroExp)
					{
						m_Transform = translate * rotate * newScale;
						OnTransformCallback(m_Transform);
					}

					m_IntersectPos = intersectPos;
				}
			}
		}
	}

	m_PickPos = curPickPos;
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