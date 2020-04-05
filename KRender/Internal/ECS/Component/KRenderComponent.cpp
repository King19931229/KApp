#include "KRenderComponent.h"
#include "Internal/KRenderGlobal.h"

KRenderComponent::KRenderComponent()
	: m_Mesh(nullptr)
{}

KRenderComponent::~KRenderComponent()
{
	UnInit();
}

bool KRenderComponent::GetLocalBound(KAABBBox& bound) const
{
	if (m_Mesh)
	{
		bound = m_Mesh->GetLocalBound();
		return true;
	}
	return false;
}

bool KRenderComponent::Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const
{
	if (m_Mesh)
	{
		const KTriangleMesh& triangleMesh = m_Mesh->GetTriangleMesh();
		if (triangleMesh.CloestPickPoint(localOrigin, localDir, result))
		{
			return true;
		}
	}
	return false;
}

bool KRenderComponent::CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const
{
	if (m_Mesh)
	{
		const KTriangleMesh& triangleMesh = m_Mesh->GetTriangleMesh();
		if (triangleMesh.Pick(localOrigin, localDir, result))
		{
			return true;
		}
	}
	return false;
}

bool KRenderComponent::Init(const char* path)
{
	return KRenderGlobal::MeshManager.Acquire(path, m_Mesh);
}

bool KRenderComponent::InitFromAsset(const char* path)
{
	return KRenderGlobal::MeshManager.AcquireFromAsset(path, m_Mesh);
}

bool KRenderComponent::InitUtility(const KMeshUtilityInfoPtr& info)
{
	return KRenderGlobal::MeshManager.AcquireAsUtility(info, m_Mesh);
}

bool KRenderComponent::UpdateUtility(const KMeshUtilityInfoPtr& info)
{
	return KRenderGlobal::MeshManager.UpdateUtility(info, m_Mesh);
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