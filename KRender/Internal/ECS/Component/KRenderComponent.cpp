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

bool KRenderComponent::InitAsUnility(const KMeshUnilityInfoPtr& info)
{
	return KRenderGlobal::MeshManager.AcquireAsUnility(info, m_Mesh);
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