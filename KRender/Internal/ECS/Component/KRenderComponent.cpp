#include "KRenderComponent.h"
#include "Internal/KRenderGlobal.h"

KRenderComponent::KRenderComponent()
	: KComponentBase(CT_RENDER),
	m_Mesh(nullptr)
{}

KRenderComponent::~KRenderComponent()
{
	UnInit();
}

bool KRenderComponent::Init(const char* path)
{
	return KRenderGlobal::MeshManager.Acquire(path, m_Mesh);
}

bool KRenderComponent::InitFromAsset(const char* path)
{
	return KRenderGlobal::MeshManager.AcquireFromAsset(path, m_Mesh);
}

bool KRenderComponent::InitAsBox(const glm::vec3& halfExtent)
{
	return KRenderGlobal::MeshManager.CreateBox(halfExtent, m_Mesh);
}

bool KRenderComponent::InitAsQuad(const glm::mat4& transform, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV)
{
	return KRenderGlobal::MeshManager.CreateQuad(transform, lengthU, lengthV, axisU, axisV, m_Mesh);
}

bool KRenderComponent::InitAsCone(const glm::mat4& transform, float height, float radius)
{
	return KRenderGlobal::MeshManager.CreateCone(transform, height, radius, m_Mesh);
}

bool KRenderComponent::InitAsCylinder(const glm::mat4& transform, float height, float radius)
{
	return KRenderGlobal::MeshManager.CreateCylinder(transform, height, radius, m_Mesh);
}

bool KRenderComponent::InitAsCircle(const glm::mat4& transform, float radius)
{
	return KRenderGlobal::MeshManager.CreateCircle(transform, radius, m_Mesh);
}

bool KRenderComponent::InitAsArc(const glm::vec3& axis, const glm::vec3& normal, float radius, float theta)
{
	return KRenderGlobal::MeshManager.CreateArc(axis, normal, radius, theta, m_Mesh);
}

bool KRenderComponent::InitAsSphere(const glm::mat4& transform, float radius)
{
	return KRenderGlobal::MeshManager.CreateSphere(transform, radius, m_Mesh);
}

bool KRenderComponent::UnInit()
{
	if (m_Mesh)
	{
		KRenderGlobal::MeshManager.Release(m_Mesh);
		m_Mesh = nullptr;
		return true;
	}
	return true;
}