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

	KECS::ComponentManager->RegisterFunc(CT_TRANSFORM,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_TransformComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_TransformComponentPool.Free((KTransformComponent*)component);
	}), IKComponentSaveLoadFuncPair());

	KECS::ComponentManager->RegisterFunc(CT_RENDER,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_RenderComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_RenderComponentPool.Free((KRenderComponent*)component);
	}), IKComponentSaveLoadFuncPair());

	KECS::ComponentManager->RegisterFunc(CT_DEBUG,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_DebugComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_DebugComponentPool.Free((KDebugComponent*)component);
	}), IKComponentSaveLoadFuncPair());
}

void KRenderComponentManager::UnInit()
{
	if (KECS::ComponentManager)
	{
		KECS::ComponentManager->UnRegisterFunc(CT_TRANSFORM);
		KECS::ComponentManager->UnRegisterFunc(CT_RENDER);
		KECS::ComponentManager->UnRegisterFunc(CT_DEBUG);
	}

	m_TransformComponentPool.UnInit();
	m_RenderComponentPool.UnInit();
	m_DebugComponentPool.UnInit();
}