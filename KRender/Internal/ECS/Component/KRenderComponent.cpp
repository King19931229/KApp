#include "KRenderComponent.h"
#include "Internal/KRenderGlobal.h"

KRenderComponent::KRenderComponent()
	: m_Mesh(nullptr),
	m_Type(NONE)
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

bool KRenderComponent::SetPathMesh(const char* path)
{
	if (path)
	{
		m_Type = MESH;
		m_Path = path;
		return true;
	}
	return false;
}

bool KRenderComponent::SetPathAsset(const char* path)
{
	if (path)
	{
		m_Type = ASSET;
		m_Path = path;
		return true;
	}
	return false;
}

bool KRenderComponent::Init()
{
	UnInit();
	if (m_Type == MESH)
	{
		return KRenderGlobal::MeshManager.Acquire(m_Path.c_str(), m_Mesh);
	}
	else if (m_Type == ASSET)
	{
		return KRenderGlobal::MeshManager.AcquireFromAsset(m_Path.c_str(), m_Mesh);
	}
	else if (m_Type == UTILITY)
	{
		return KRenderGlobal::MeshManager.AcquireAsUtility(m_UtilityInfo, m_Mesh);
	}
	else
	{
		return false;
	}
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

bool KRenderComponent::InitUtility(const KMeshUtilityInfoPtr& info)
{
	UnInit();
	m_Type = UTILITY;
	m_UtilityInfo = info;
	return KRenderGlobal::MeshManager.AcquireAsUtility(info, m_Mesh);
}

bool KRenderComponent::UpdateUtility(const KMeshUtilityInfoPtr& info)
{
	ASSERT_RESULT(m_Type == UTILITY);
	ASSERT_RESULT(m_UtilityInfo->GetType() == info->GetType());
	m_UtilityInfo = info;
	return KRenderGlobal::MeshManager.UpdateUtility(info, m_Mesh);
}