#include "KRenderComponent.h"
#include "Internal/KRenderGlobal.h"

KRenderComponent::KRenderComponent()
	: KComponentBase(CT_RENDER),
	m_Mesh(nullptr)
{}

KRenderComponent::~KRenderComponent()
{
	assert(m_Mesh == nullptr);
	m_Mesh = nullptr;
}

bool KRenderComponent::Init(const char* path)
{
	return KRenderGlobal::MeshManager.Acquire(path, m_Mesh);
}

bool KRenderComponent::InitFromAsset(const char* path)
{
	return KRenderGlobal::MeshManager.AcquireFromAsset(path, m_Mesh);
}

bool KRenderComponent::UnInit()
{
	if(m_Mesh)
	{
		KRenderGlobal::MeshManager.Release(m_Mesh);
		m_Mesh = nullptr;
		return true;
	}
	return false;
}