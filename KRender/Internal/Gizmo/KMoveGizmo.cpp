#include "KMoveGizmo.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KPlane.h"
#include "KBase/Interface/IKLog.h"

KMoveGizmo::KMoveGizmo()
	: KGizmoBase(),
	m_OperatorType(MoveOperator::MOVE_NONE)
{
}

KMoveGizmo::~KMoveGizmo()
{
	assert(!m_Camera);
}

#define ALL_SCENE_ENTITY { m_OriginEntity, m_XAxisEntity, m_YAxisEntity, m_ZAxisEntity, m_XArrowEntity, m_YArrowEntity, m_ZArrowEntity, m_XZPlaneEntity, m_YZPlaneEntity, m_XYPlaneEntity }
#define NO_ENTITY {}

constexpr float ORIGIN_RADIUS = 0.08f;

constexpr float AXIS_LENGTH = 1.0f;
constexpr float AXIS_RADIUS = 0.025f;

constexpr float ARROW_LENGTH = 0.15f;
constexpr float ARROW_RADIUS = 0.045f;

constexpr float PLANE_SIZE = 0.5f;

static const glm::vec4 X_AXIS_COLOR = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
static const glm::vec4 Y_AXIS_COLOR = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
static const glm::vec4 Z_AXIS_COLOR = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

static const glm::vec4 SELECT_AXIS_COLOR = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

static const glm::vec4 PLANE_COLOR = glm::vec4(1.0f, 1.0f, 0.0f, 0.5f);
static const glm::vec4 SELECT_PLANE_COLOR = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

bool KMoveGizmo::Init(const KCamera* camera)
{
	UnInit();

	KGizmoBase::Init(camera);

	m_OriginEntity = KECS::EntityManager->CreateEntity();

	m_XAxisEntity = KECS::EntityManager->CreateEntity();
	m_YAxisEntity = KECS::EntityManager->CreateEntity();
	m_ZAxisEntity = KECS::EntityManager->CreateEntity();

	m_XArrowEntity = KECS::EntityManager->CreateEntity();
	m_YArrowEntity = KECS::EntityManager->CreateEntity();
	m_ZArrowEntity = KECS::EntityManager->CreateEntity();

	m_XZPlaneEntity = KECS::EntityManager->CreateEntity();
	m_YZPlaneEntity = KECS::EntityManager->CreateEntity();
	m_XYPlaneEntity = KECS::EntityManager->CreateEntity();

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

	// XArrow
	m_XArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateCone({
		glm::translate(glm::mat4(1.0f), glm::vec3(AXIS_LENGTH, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)),
		ARROW_LENGTH,
		ARROW_RADIUS }));

	m_XArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_XArrowEntity->RegisterComponent(CT_TRANSFORM);

	// YArrow
	m_YArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateCone({
		glm::translate(glm::mat4(1.0f), glm::vec3(0, AXIS_LENGTH, 0.0f)),
		ARROW_LENGTH,
		ARROW_RADIUS }));

	m_YArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_YArrowEntity->RegisterComponent(CT_TRANSFORM);

	// ZArrow
	m_ZArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateCone({
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, AXIS_LENGTH)) *
		glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)),
		ARROW_LENGTH,
		ARROW_RADIUS }));

	m_ZArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_ZArrowEntity->RegisterComponent(CT_TRANSFORM);

	// XZPlane
	m_XZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateQuad({
		glm::mat4(1.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));

	m_XZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(PLANE_COLOR);

	m_XZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_YZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateQuad({ glm::mat4(1.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));

	m_YZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(PLANE_COLOR);

	m_YZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_XYPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitUtility(
		KMeshUtility::CreateQuad({ glm::mat4(1.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }));

	m_XYPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

	m_XYPlaneEntity->RegisterComponent(CT_TRANSFORM);

	m_AllEntity = ALL_SCENE_ENTITY;

	return true;
}

bool KMoveGizmo::UnInit()
{
	KGizmoBase::UnInit();
	m_AllEntity = NO_ENTITY;
	return true;
}

KMoveGizmo::MoveOperator KMoveGizmo::GetOperatorType(unsigned int x, unsigned int y, KPlane& plane, glm::vec3& intersectPos)
{
	glm::vec3 origin, dir;
	if (CalcPickRay(x, y, origin, dir))
	{
		glm::vec3 transformPos = TransformPos();

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
				if (fabs(normalizePos.x) < AXIS_RADIUS && normalizePos.y < AXIS_LENGTH && normalizePos.y >= 0)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick y axis");
					return MoveOperator::MOVE_Y;
				}
				else if (fabs(normalizePos.y) < AXIS_RADIUS && normalizePos.x < AXIS_LENGTH && normalizePos.x >= 0)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick x axis");
					return MoveOperator::MOVE_X;
				}
				else if (normalizePos.x < PLANE_SIZE && normalizePos.x > 0.0f && normalizePos.y < PLANE_SIZE && normalizePos.y > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick xy plane");
					return MoveOperator::MOVE_XY;
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
				if (fabs(normalizePos.y) < AXIS_RADIUS && normalizePos.z < AXIS_LENGTH && normalizePos.z >= 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return MoveOperator::MOVE_Z;
				}
				else if (fabs(normalizePos.z) < AXIS_RADIUS && normalizePos.y < AXIS_LENGTH && normalizePos.y >= 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick y axis");
					return MoveOperator::MOVE_Y;
				}
				else if (normalizePos.y < PLANE_SIZE && normalizePos.y > 0.0f && normalizePos.z < PLANE_SIZE && normalizePos.z > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick yz plane");
					return MoveOperator::MOVE_YZ;
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
				if (fabs(normalizePos.x) < AXIS_RADIUS && normalizePos.z < AXIS_LENGTH && normalizePos.z >= 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return MoveOperator::MOVE_Z;
				}
				else if (fabs(normalizePos.z) < AXIS_RADIUS && normalizePos.x < AXIS_LENGTH && normalizePos.x >= 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick x axis");
					return MoveOperator::MOVE_X;
				}
				else if (normalizePos.x < PLANE_SIZE && normalizePos.x > 0.0f && normalizePos.z < PLANE_SIZE && normalizePos.z > 0.0f)
				{
					//KLog::Logger->Log(LL_DEBUG, "pick xz plane");
					return MoveOperator::MOVE_XZ;
				}
			}
		}
	}

	return MoveOperator::MOVE_NONE;
}

void KMoveGizmo::OnMouseDown(unsigned int x, unsigned int y)
{
	m_OperatorType = GetOperatorType(x, y, m_PickPlane, m_IntersectPos);
	if (IsTriggered())
	{
		OnTriggerCallback(true);
	}
}

void KMoveGizmo::OnMouseMove(unsigned int x, unsigned int y)
{
	if (m_OperatorType == MoveOperator::MOVE_NONE)
	{
		KPlane plane;
		glm::vec3 intersectPos;
		MoveOperator predictType = GetOperatorType(x, y, plane, intersectPos);

		SetEntityColor(m_XAxisEntity, X_AXIS_COLOR);
		SetEntityColor(m_YAxisEntity, Y_AXIS_COLOR);
		SetEntityColor(m_ZAxisEntity, Z_AXIS_COLOR);
		SetEntityColor(m_XZPlaneEntity, PLANE_COLOR);
		SetEntityColor(m_XYPlaneEntity, PLANE_COLOR);
		SetEntityColor(m_YZPlaneEntity, PLANE_COLOR);

		switch (predictType)
		{
		case KMoveGizmo::MoveOperator::MOVE_X:
			SetEntityColor(m_XAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_Y:
			SetEntityColor(m_YAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_Z:
			SetEntityColor(m_ZAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_XY:
			SetEntityColor(m_XYPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_XZ:
			SetEntityColor(m_XZPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_YZ:
			SetEntityColor(m_YZPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_NONE:
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
				glm::vec3 diff = intersectPos - m_IntersectPos;
				glm::vec3 moveVec;

				glm::vec3 xAxis = GetAxis(GizmoAxis::AXIS_X);
				glm::vec3 yAxis = GetAxis(GizmoAxis::AXIS_Y);
				glm::vec3 zAxis = GetAxis(GizmoAxis::AXIS_Z);

				switch (m_OperatorType)
				{
				case KMoveGizmo::MoveOperator::MOVE_X:
					moveVec = glm::vec3(glm::dot(xAxis, diff), 0.0f, 0.0f);
					break;
				case KMoveGizmo::MoveOperator::MOVE_Y:
					moveVec = glm::vec3(0.0f, glm::dot(yAxis, diff), 0.0f);
					break;
				case KMoveGizmo::MoveOperator::MOVE_Z:
					moveVec = glm::vec3(0.0f, 0.0f, glm::dot(zAxis, diff));
					break;
				case KMoveGizmo::MoveOperator::MOVE_XY:
					moveVec = glm::vec3(glm::dot(xAxis, diff), glm::dot(yAxis, diff), 0.0f);
					break;
				case KMoveGizmo::MoveOperator::MOVE_XZ:
					moveVec = glm::vec3(glm::dot(xAxis, diff), 0.0f, glm::dot(zAxis, diff));
					break;
				case KMoveGizmo::MoveOperator::MOVE_YZ:
					moveVec = glm::vec3(0.0f, glm::dot(yAxis, diff), glm::dot(zAxis, diff));
					break;
				case KMoveGizmo::MoveOperator::MOVE_NONE:
				default:
					assert(false && "impossible to reach");
					break;
				}

				moveVec = moveVec.x * xAxis + moveVec.y * yAxis + moveVec.z * zAxis;
				m_Transform = glm::translate(glm::mat4(1.0f), moveVec) * m_Transform;

				OnTransformCallback(m_Transform);

				m_IntersectPos = intersectPos;
			}
		}
	}
}

void KMoveGizmo::OnMouseUp(unsigned int x, unsigned int y)
{
	if (IsTriggered())
	{
		OnTriggerCallback(false);
	}
	m_OperatorType = MoveOperator::MOVE_NONE;
}

bool KMoveGizmo::IsTriggered() const
{
	return m_OperatorType != MoveOperator::MOVE_NONE;
}