#pragma once

#include "Component/KRenderComponent.h"
#include "Component/KTransformComponent.h"
#include "Component/KDebugComponent.h"
#include "Component/KUserComponent.h"

#include "KBase/Publish/KObjectPool.h"

class KRenderComponentManager
{
protected:
	KObjectPool<KTransformComponent> m_TransformComponentPool;
	KObjectPool<KRenderComponent> m_RenderComponentPool;
	KObjectPool<KDebugComponent> m_DebugComponentPool;
	KObjectPool<KUserComponent> m_UserComponentPool;
public:
	KRenderComponentManager();
	~KRenderComponentManager();

	void Init();
	void UnInit();
};