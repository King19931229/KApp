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
	renderComponent->InitAsSphere(glm::mat4(1.0f), ORIGIN_RADIUS);

	m_OriginEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_OriginEntity->RegisterComponent(CT_TRANSFORM);

	// XAxis
	m_XAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)), AXIS_LENGTH, AXIS_RADIUS);

	m_XAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(X_AXIS_COLOR);

	m_XAxisEntity->RegisterComponent(CT_TRANSFORM);

	// YAxis
	m_YAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::mat4(1.0f), AXIS_LENGTH, AXIS_RADIUS);

	m_YAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(Y_AXIS_COLOR);

	m_YAxisEntity->RegisterComponent(CT_TRANSFORM);

	// ZAxis
	m_ZAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), AXIS_LENGTH, AXIS_RADIUS);

	m_ZAxisEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(Z_AXIS_COLOR);

	m_ZAxisEntity->RegisterComponent(CT_TRANSFORM);

	// XArrow
	m_XArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(1.0f), glm::vec3(AXIS_LENGTH, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)),
		ARROW_LENGTH,
		ARROW_RADIUS);

	m_XArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_XArrowEntity->RegisterComponent(CT_TRANSFORM);

	// YArrow
	m_YArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(1.0f), glm::vec3(0, AXIS_LENGTH, 0.0f)),
		ARROW_LENGTH,
		ARROW_RADIUS);

	m_YArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_YArrowEntity->RegisterComponent(CT_TRANSFORM);

	// ZArrow
	m_ZArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, AXIS_LENGTH)) *
		glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)),
		ARROW_LENGTH,
		ARROW_RADIUS);

	m_ZArrowEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	m_ZArrowEntity->RegisterComponent(CT_TRANSFORM);

	// XZPlane
	m_XZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(1.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_XZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(PLANE_COLOR);

	m_XZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_YZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(1.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_YZPlaneEntity->RegisterComponent(CT_DEBUG, &debugComponent);
	debugComponent->SetColor(PLANE_COLOR);

	m_YZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_XYPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(1.0f), PLANE_SIZE, PLANE_SIZE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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

			// TODO ÐÞÕýMoveÂß¼­
			KRenderGlobal::Scene.Remove(entity);

			transform->SetPosition(transformPos);
			transform->SetRotate(rotate);
			transform->SetScale(glm::vec3(m_ScreenScaleFactor));

			KRenderGlobal::Scene.Add(entity);
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

KMoveGizmo::MoveOperator KMoveGizmo::GetOperatorType(unsigned int x, unsigned int y, KPlane& plane, glm::vec3& intersectPos)
{
	glm::vec3 origin, dir;
	if (CalcPickRay(x, y, origin, dir))
	{
		glm::vec3 transformPos = glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);

		glm::vec3 xAxis = GetAxis(MoveGizmoAxis::AXIS_X);
		glm::vec3 yAxis = GetAxis(MoveGizmoAxis::AXIS_Y);
		glm::vec3 zAxis = GetAxis(MoveGizmoAxis::AXIS_Z);

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
	m_OperatorType = GetOperatorType(x, y, m_PickPlane, m_IntersectPos);
}

void KMoveGizmo::OnMouseMove(unsigned int x, unsigned int y)
{
	if (m_OperatorType == MoveOperator::MOVE_NONE)
	{
		KPlane plane;
		glm::vec3 intersectPos;
		MoveOperator predictType = GetOperatorType(x, y, plane, intersectPos);

		auto setEntityColor = [](KEntityPtr entity, const glm::vec4& color)
		{
			KDebugComponent* debugComponent = nullptr;
			if (entity && entity->GetComponent(CT_DEBUG, &debugComponent))
			{
				debugComponent->SetColor(color);
			}
		};

		setEntityColor(m_XAxisEntity, X_AXIS_COLOR);
		setEntityColor(m_YAxisEntity, Y_AXIS_COLOR);
		setEntityColor(m_ZAxisEntity, Z_AXIS_COLOR);
		setEntityColor(m_XZPlaneEntity, PLANE_COLOR);
		setEntityColor(m_XYPlaneEntity, PLANE_COLOR);
		setEntityColor(m_YZPlaneEntity, PLANE_COLOR);

		switch (predictType)
		{
		case KMoveGizmo::MoveOperator::MOVE_X:
			setEntityColor(m_XAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_Y:
			setEntityColor(m_YAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_Z:
			setEntityColor(m_ZAxisEntity, SELECT_AXIS_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_XY:
			setEntityColor(m_XYPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_XZ:
			setEntityColor(m_XZPlaneEntity, SELECT_PLANE_COLOR);
			break;
		case KMoveGizmo::MoveOperator::MOVE_YZ:
			setEntityColor(m_YZPlaneEntity, SELECT_PLANE_COLOR);
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
			glm::vec3 transformPos = glm::vec3(m_Transform[3][0], m_Transform[3][1], m_Transform[3][2]);
			glm::vec3 intersectPos;
			if (m_PickPlane.Intersect(origin, dir, intersectPos))
			{
				glm::vec3 diff = intersectPos - m_IntersectPos;
				glm::vec3 moveVec;

				glm::vec3 xAxis = GetAxis(MoveGizmoAxis::AXIS_X);
				glm::vec3 yAxis = GetAxis(MoveGizmoAxis::AXIS_Y);
				glm::vec3 zAxis = GetAxis(MoveGizmoAxis::AXIS_Z);

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

				m_IntersectPos = intersectPos;
			}
		}
	}
}

void KMoveGizmo::OnMouseUp(unsigned int x, unsigned int y)
{
	m_OperatorType = MoveOperator::MOVE_NONE;
}