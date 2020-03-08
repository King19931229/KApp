#include "KMoveGizmo.h"
#include "Internal/KRenderGlobal.h"

KMoveGizmo::KMoveGizmo()
	: m_Transform(glm::mat4(1.0f)),
	m_Mode(GizmoManipulateMode::GIZMO_MANIPULATE_WORLD),
	m_ScreenWidth(1),
	m_ScreenHeight(1),
	m_DisplayScale(1.0f),
	m_ScreenScaleFactor(1.0f),
	m_Camera(nullptr)	
{
}

#define ALL_SCENE_ENTITY { m_OriginEntity, m_XAxisEntity, m_YAxisEntity, m_ZAxisEntity, m_XArrowEntity, m_YArrowEntity, m_ZArrowEntity, m_XZPlaneEntity, m_YZPlaneEntity, m_XYPlaneEntity }

KMoveGizmo::~KMoveGizmo()
{
	assert(!m_Camera);
}

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

	constexpr float scale = 1.0f;
	constexpr float originRaidus = 0.08f;

	constexpr float axisLength = 1.0f;
	constexpr float axisRadius = 0.025f;

	constexpr float arrowLength = 0.15f;
	constexpr float arrowRadius = 0.045f;

	constexpr float planeSize = 0.5f;

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

}

void KMoveGizmo::OnMouseUp(unsigned int x, unsigned int y)
{

}