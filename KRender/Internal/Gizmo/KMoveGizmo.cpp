#include "KMoveGizmo.h"
#include "Internal/KRenderGlobal.h"
#include "Publish/KPlane.h"

#include "KBase/Interface/IKLog.h"

KMoveGizmo::KMoveGizmo()
	: m_Transform(glm::mat4(1.0f)),
	m_Mode(GizmoManipulateMode::GIZMO_MANIPULATE_WORLD),
	m_ScreenWidth(0),
	m_ScreenHeight(0),
	m_DisplayScale(1.0f),
	m_ScreenScaleFactor(1.0f),
	m_Camera(nullptr),
	m_OperatorType(MoveOperator::MOVE_NONE)
{
}

#define ALL_SCENE_ENTITY { m_OriginEntity, m_XAxisEntity, m_YAxisEntity, m_ZAxisEntity, m_XArrowEntity, m_YArrowEntity, m_ZArrowEntity, m_XZPlaneEntity, m_YZPlaneEntity, m_XYPlaneEntity }

KMoveGizmo::~KMoveGizmo()
{
	assert(!m_Camera);
}

constexpr float scale = 1.0f;
constexpr float originRaidus = 0.08f;

constexpr float axisLength = 1.0f;
constexpr float axisRadius = 0.025f;

constexpr float arrowLength = 0.15f;
constexpr float arrowRadius = 0.045f;

constexpr float planeSize = 0.5f;

bool KMoveGizmo::Init(const KCamera* camera)
{
	UnInit();

	m_Camera = camera;

	m_OriginEntity = KECSGlobal::EntityManager.CreateEntity();

	m_XAxisEntity = KECSGlobal::EntityManager.CreateEntity();
	m_YAxisEntity = KECSGlobal::EntityManager.CreateEntity();
	m_ZAxisEntity = KECSGlobal::EntityManager.CreateEntity();

	m_XArrowEntity = KECSGlobal::EntityManager.CreateEntity();
	m_YArrowEntity = KECSGlobal::EntityManager.CreateEntity();
	m_ZArrowEntity = KECSGlobal::EntityManager.CreateEntity();

	m_XZPlaneEntity = KECSGlobal::EntityManager.CreateEntity();
	m_YZPlaneEntity = KECSGlobal::EntityManager.CreateEntity();
	m_XYPlaneEntity = KECSGlobal::EntityManager.CreateEntity();

	KRenderComponent* renderComponent = nullptr;
	KDebugComponent* debugComponent = nullptr;

	// Origin
	m_OriginEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsSphere(glm::mat4(scale), originRaidus);

	m_OriginEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_OriginEntity->RegisterComponent(CT_TRANSFORM);

	// XAxis
	m_XAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::rotate(glm::mat4(scale), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)), axisLength, axisRadius);

	m_XAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	m_XAxisEntity->RegisterComponent(CT_TRANSFORM);

	// YAxis
	m_YAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::mat4(scale), axisLength, axisRadius);

	m_YAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

	m_YAxisEntity->RegisterComponent(CT_TRANSFORM);

	// ZAxis
	m_ZAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::rotate(glm::mat4(scale), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), axisLength, axisRadius);

	m_ZAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

	m_ZAxisEntity->RegisterComponent(CT_TRANSFORM);

	// XArrow
	m_XArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(1.0f), glm::vec3(axisLength, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(scale), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)),
		arrowLength,
		arrowRadius);

	m_XArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_XArrowEntity->RegisterComponent(CT_TRANSFORM);

	// YArrow
	m_YArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(scale), glm::vec3(0, axisLength, 0.0f)),
		arrowLength,
		arrowRadius);

	m_YArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_YArrowEntity->RegisterComponent(CT_TRANSFORM);

	// ZArrow
	m_ZArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, axisLength)) *
		glm::rotate(glm::mat4(scale), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)),
		arrowLength,
		arrowRadius);

	m_ZArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_ZArrowEntity->RegisterComponent(CT_TRANSFORM);

	// XZPlane
	m_XZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(scale), planeSize, planeSize, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_XZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

	m_XZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_YZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(scale), planeSize, planeSize, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_YZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

	m_YZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_XYPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(scale), planeSize, planeSize, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	m_XYPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

	m_XYPlaneEntity->RegisterComponent(CT_TRANSFORM);

	return true;
}

bool KMoveGizmo::UnInit()
{
	m_Camera = nullptr;
	for (KEntityPtr entity : ALL_SCENE_ENTITY)
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
void KMoveGizmo::Enter()
{
	for (KEntityPtr entity : ALL_SCENE_ENTITY)
	{
		assert(entity);
		KRenderGlobal::Scene.Add(entity);
	}
}

void KMoveGizmo::Leave()
{
	for (KEntityPtr entity : ALL_SCENE_ENTITY)
	{
		assert(entity);
		KRenderGlobal::Scene.Remove(entity);
	}
}

void KMoveGizmo::Update()
{
	glm::vec3 transformPos = glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);
	glm::mat3 rotate;

	if (m_Mode == GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL)
	{
		glm::vec3 xAxis = m_Transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 yAxis = m_Transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec3 zAxis = m_Transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

		xAxis = glm::normalize(xAxis);
		yAxis = glm::normalize(yAxis);
		zAxis = glm::normalize(zAxis);

		rotate = glm::mat3(xAxis, yAxis, zAxis);
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

	for (KEntityPtr entity : ALL_SCENE_ENTITY)
	{
		if (entity)
		{
			KTransformComponent* transform = nullptr;
			entity->GetComponent(CT_TRANSFORM, &transform);

			glm::vec3 previousPos = transform->GetPosition();

			transform->SetPosition(transformPos);
			transform->SetRotate(rotate);
			transform->SetScale(glm::vec3(m_ScreenScaleFactor));

			if (previousPos != transformPos)
			{
				KRenderGlobal::Scene.Move(entity);
			}
		}
	}
}

bool KMoveGizmo::CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir)
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

glm::vec3 KMoveGizmo::GetAxis(MoveGizmoAxis axis)
{
	glm::vec3 axisVec = glm::vec3(0.0f);
	switch (axis)
	{
	case KMoveGizmo::MoveGizmoAxis::AXIS_X:
		axisVec = glm::vec3(1.0f, 0.0f, 0.0f);
		break;
	case KMoveGizmo::MoveGizmoAxis::AXIS_Y:
		axisVec = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case KMoveGizmo::MoveGizmoAxis::AXIS_Z:
		axisVec = glm::vec3(0.0f, 0.0f, 1.0f);
		break;
	default:
		break;
	}

	if (m_Mode == GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL)
	{
		axisVec = m_Transform * glm::vec4(axisVec, 0.0f);
	}

	return axisVec;
}

KMoveGizmo::MoveOperator KMoveGizmo::GetOperatorType(unsigned int x, unsigned int y)
{
	glm::vec3 origin, dir;
	if (CalcPickRay(x, y, origin, dir))
	{
		glm::vec3 transformPos = glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);

		KPlane plane;
		glm::vec3 intersectPos;

		// XY plane
		plane.Init(transformPos, GetAxis(MoveGizmoAxis::AXIS_Z));
		if (plane.Intersect(origin, dir, intersectPos))
		{
			intersectPos /= m_ScreenScaleFactor;
			if (intersectPos.x > 0.0f && intersectPos.y > 0.0f)
			{
				if (intersectPos.x < axisRadius && intersectPos.y < axisLength)
				{
					KLog::Logger->Log(LL_DEBUG, "pick y axis");
					return MoveOperator::MOVE_Y;
				}
				else if (intersectPos.y < axisRadius && intersectPos.x < axisLength)
				{
					KLog::Logger->Log(LL_DEBUG, "pick x axis");
					return MoveOperator::MOVE_X;
				}
				else if (intersectPos.x < planeSize && intersectPos.y < planeSize)
				{
					KLog::Logger->Log(LL_DEBUG, "pick xy plane");
					return MoveOperator::MOVE_XY;
				}
			}
		}

		// YZ plane
		plane.Init(transformPos, GetAxis(MoveGizmoAxis::AXIS_X));
		if (plane.Intersect(origin, dir, intersectPos))
		{
			intersectPos /= m_ScreenScaleFactor;
			if (intersectPos.y > 0.0f && intersectPos.z > 0.0f)
			{
				if (intersectPos.y < axisRadius && intersectPos.z < axisLength)
				{
					KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return MoveOperator::MOVE_Z;
				}
				else if (intersectPos.z < axisRadius && intersectPos.y < axisLength)
				{
					KLog::Logger->Log(LL_DEBUG, "pick y axis");
					return MoveOperator::MOVE_Y;
				}
				else if (intersectPos.y < planeSize && intersectPos.z < planeSize)
				{
					KLog::Logger->Log(LL_DEBUG, "pick yz plane");
					return MoveOperator::MOVE_YZ;
				}
			}
		}

		// XZ plane
		plane.Init(transformPos, GetAxis(MoveGizmoAxis::AXIS_Y));
		if (plane.Intersect(origin, dir, intersectPos))
		{
			intersectPos /= m_ScreenScaleFactor;
			if (intersectPos.x > 0.0f && intersectPos.z > 0.0f)
			{
				if (intersectPos.x < axisRadius && intersectPos.z < axisLength)
				{
					KLog::Logger->Log(LL_DEBUG, "pick z axis");
					return MoveOperator::MOVE_Z;
				}
				else if (intersectPos.z < axisRadius && intersectPos.x < axisLength)
				{
					KLog::Logger->Log(LL_DEBUG, "pick x axis");
					return MoveOperator::MOVE_X;
				}
				else if (intersectPos.x < planeSize && intersectPos.z < planeSize)
				{
					KLog::Logger->Log(LL_DEBUG, "pick xz plane");
					return MoveOperator::MOVE_XZ;
				}
			}
		}
	}
}

const glm::mat4& KMoveGizmo::GetMatrix() const
{
	return m_Transform;
}

void KMoveGizmo::SetMatrix(const glm::mat4& matrix)
{
	m_Transform = matrix;
}

GizmoManipulateMode KMoveGizmo::GetManipulateMode() const
{
	return m_Mode;
}

void KMoveGizmo::SetManipulateMode(GizmoManipulateMode mode)
{
	m_Mode = mode;
}

float KMoveGizmo::GetDisplayScale() const
{
	return m_DisplayScale;
}

void KMoveGizmo::SetDisplayScale(float scale)
{
	m_DisplayScale = scale;
}

void KMoveGizmo::SetScreenSize(unsigned int width, unsigned int height)
{
	m_ScreenWidth = width;
	m_ScreenHeight = height;
}

void KMoveGizmo::OnMouseDown(unsigned int x, unsigned int y)
{

}

void KMoveGizmo::OnMouseMove(unsigned int x, unsigned int y)
{
	// test
	GetOperatorType(x, y);
}

void KMoveGizmo::OnMouseUp(unsigned int x, unsigned int y)
{

}