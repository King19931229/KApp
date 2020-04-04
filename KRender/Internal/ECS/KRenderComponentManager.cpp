#include "KRenderComponentManager.h"
#include "KBase/Interface/Component/IKComponentManager.h"

KRenderComponentManager::KRenderComponentManager()
{
}

KRenderComponentManager::~KRenderComponentManager()
{
}

void KRenderComponentManager::Init()
{
	m_TransformComponentPool.Init(1024);
	m_RenderComponentPool.Init(1024);
	m_DebugComponentPool.Init(1024);

	KComponent::Manager->RegisterFunc(CT_TRANSFORM,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_TransformComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_TransformComponentPool.Free((KTransformComponent*)component);
	}));

	KComponent::Manager->RegisterFunc(CT_RENDER,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_RenderComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_RenderComponentPool.Free((KRenderComponent*)component);
	}));

	KComponent::Manager->RegisterFunc(CT_DEBUG,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_DebugComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_DebugComponentPool.Free((KDebugComponent*)component);
	}));
}

void KRenderComponentManager::UnInit()
{
	KComponent::Manager->UnRegisterFunc(CT_TRANSFORM);
	KComponent::Manager->UnRegisterFunc(CT_RENDER);
	KComponent::Manager->UnRegisterFunc(CT_DEBUG);

	m_TransformComponentPool.UnInit();
	m_RenderComponentPool.UnInit();
	m_DebugComponentPool.UnInit();
}