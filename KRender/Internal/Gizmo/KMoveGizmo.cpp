#include "KMoveGizmo.h"
#include "Internal/KRenderGlobal.h"

KMoveGizmo::KMoveGizmo()
	: m_Camera(nullptr),
	m_Mode(GizmoManipulateMode::GIZMO_MANIPULATE_WORLD)
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

	constexpr float scale = 5.0f;
	constexpr float originRaidus = 1.0f;

	constexpr float axisLength = 10.0f;
	constexpr float axisRadius = 0.5f;

	constexpr float arrowLength = 2.0f;
	constexpr float arrowRadius = 1.0f;

	constexpr float planeSize = 5.0f;

	// Origin
	m_OriginEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsSphere(glm::mat4(scale), originRaidus);
	m_OriginEntity->RegisterComponent(CT_TRANSFORM);

	// XAxis
	m_XAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::rotate(glm::mat4(scale), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)), axisLength, axisRadius);
	m_XAxisEntity->RegisterComponent(CT_TRANSFORM);

	// YAxis
	m_YAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::mat4(scale), axisLength, axisRadius);
	m_YAxisEntity->RegisterComponent(CT_TRANSFORM);

	// ZAxis
	m_ZAxisEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCylinder(glm::rotate(glm::mat4(scale), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), axisLength, axisRadius);
	m_ZAxisEntity->RegisterComponent(CT_TRANSFORM);

	// XArrow
	m_XArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(1.0f), glm::vec3(axisLength, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(scale), -glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)),
		arrowLength,
		arrowRadius);
	m_XArrowEntity->RegisterComponent(CT_TRANSFORM);

	// YArrow
	m_YArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(scale), glm::vec3(0, axisLength, 0.0f)),
		arrowLength,
		arrowRadius);
	m_YArrowEntity->RegisterComponent(CT_TRANSFORM);

	// ZArrow
	m_ZArrowEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsCone(
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, axisLength)) *
		glm::rotate(glm::mat4(scale), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)),
		arrowLength,
		arrowRadius);
	m_ZArrowEntity->RegisterComponent(CT_TRANSFORM);

	// XZPlane
	m_XZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(scale), planeSize, planeSize, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_XZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_YZPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(scale), planeSize, planeSize, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_YZPlaneEntity->RegisterComponent(CT_TRANSFORM);

	// YZPlane
	m_XYPlaneEntity->RegisterComponent(CT_RENDER, &renderComponent);
	renderComponent->InitAsQuad(glm::mat4(scale), planeSize, planeSize, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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

const glm::mat4& KMoveGizmo::GetMatrix() const
{
	return m_Transform;
}

GizmoManipulateMode KMoveGizmo::GetManipulateMode() const
{
	return m_Mode;
}

void KMoveGizmo::SetManipulateMode(GizmoManipulateMode mode)
{
	m_Mode = mode;
}