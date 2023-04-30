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
	m_UserComponentPool.Init(1024);

	KECS::ComponentManager->RegisterFunc(CT_TRANSFORM,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_TransformComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_TransformComponentPool.Free((KTransformComponent*)component);
	}));

	KECS::ComponentManager->RegisterFunc(CT_RENDER,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_RenderComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_RenderComponentPool.Free((KRenderComponent*)component);
	}));

	KECS::ComponentManager->RegisterFunc(CT_DEBUG,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_DebugComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_DebugComponentPool.Free((KDebugComponent*)component);
	}));

	KECS::ComponentManager->RegisterFunc(CT_USER,
		IKComponentCreateDestroyFuncPair(
			[this]()->IKComponentBase*
	{
		return m_UserComponentPool.Alloc();
	},
			[this](IKComponentBase* component)
	{
		m_UserComponentPool.Free((KUserComponent*)component);
	}));
}

void KRenderComponentManager::UnInit()
{
	if (KECS::ComponentManager)
	{
		KECS::ComponentManager->UnRegisterFunc(CT_TRANSFORM);
		KECS::ComponentManager->UnRegisterFunc(CT_RENDER);
		KECS::ComponentManager->UnRegisterFunc(CT_DEBUG);
		KECS::ComponentManager->UnRegisterFunc(CT_USER);
	}

	m_TransformComponentPool.UnInit();
	m_RenderComponentPool.UnInit();
	m_DebugComponentPool.UnInit();
	m_UserComponentPool.UnInit();
}