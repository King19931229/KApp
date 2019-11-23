#include "KComponentManager.h"

KComponentManager::KComponentManager()
{

}

KComponentManager::~KComponentManager()
{

}

void KComponentManager::Init()
{
	m_TransformComponentPool.Init(1024);
	m_RenderComponentPool.Init(1024);
}

void KComponentManager::UnInit()
{
	m_TransformComponentPool.UnInit();
	m_RenderComponentPool.UnInit();
}

KComponentBase* KComponentManager::Alloc(ComponentType type)
{
	switch (type)
	{
	case CT_TRANSFORM:
		return m_TransformComponentPool.Alloc();
	case CT_RENDER:
		return m_RenderComponentPool.Alloc();
	default:
		assert(false && "unknown component");
		return nullptr;
	}
}

void KComponentManager::Free(KComponentBase* component)
{
	if(component)
	{
		ComponentType type = component->GetType();
		switch (type)
		{
		case CT_TRANSFORM:
			m_TransformComponentPool.Free((KTransformComponent*)component);
			return;
		case CT_RENDER:
			m_RenderComponentPool.Free((KRenderComponent*)component);
			return;
		default:
			assert(false && "unknown component");
		}
	}
}