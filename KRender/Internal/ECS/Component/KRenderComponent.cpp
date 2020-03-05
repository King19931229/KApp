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

bool KRenderComponent::InitAsQuad(float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV)
{
	return KRenderGlobal::MeshManager.CreateQuad(lengthU, lengthV, axisU, axisV, m_Mesh);
}

bool KRenderComponent::InitAsCone(const glm::vec3& org, float height, float radius)
{
	return KRenderGlobal::MeshManager.CreateCone(org, height, radius, m_Mesh);
}

bool KRenderComponent::InitAsCylinder(const glm::vec3& org, float height, float radius)
{
	return KRenderGlobal::MeshManager.CreateCylinder(org, height, radius, m_Mesh);
}

bool KRenderComponent::InitAsCircle(float radius)
{
	return KRenderGlobal::MeshManager.CreateCircle(radius, m_Mesh);
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