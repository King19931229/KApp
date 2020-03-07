#pragma once

#include "Component/KRenderComponent.h"
#include "Component/KTransformComponent.h"
#include "Component/KDebugComponent.h"

#include "KBase/Publish/KObjectPool.h"

class KComponentManager
{
protected:
	KObjectPool<KTransformComponent> m_TransformComponentPool;
	KObjectPool<KRenderComponent> m_RenderComponentPool;
	KObjectPool<KDebugComponent> m_DebugComponentPool;
public:
	KComponentManager();
	~KComponentManager();

	void Init();
	void UnInit();

	KComponentBase* Alloc(ComponentType type);
	void Free(KComponentBase* component);
};