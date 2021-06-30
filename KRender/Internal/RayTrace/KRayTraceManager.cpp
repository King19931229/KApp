#include "KRayTraceManager.h"
#include "KRayTraceScene.h"
#include "Internal/KRenderGlobal.h"

KRayTraceManager::KRayTraceManager()
{
}

KRayTraceManager::~KRayTraceManager()
{
}

bool KRayTraceManager::Init()
{
	return true;
}

bool KRayTraceManager::UnInit()
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->UnInit();
	}
	m_Scenes.clear();
	return true;
}

bool KRayTraceManager::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex, uint32_t chainIndex)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->Execute(primaryBuffer, frameIndex);
	}
	return true;
}

bool KRayTraceManager::UpdateCamera(uint32_t frameIndex)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->UpdateCamera(frameIndex);
	}
	return true;
}

bool KRayTraceManager::AcquireRayTraceScene(IKRayTraceScenePtr& scene)
{
	scene = IKRayTraceScenePtr(KNEW KRayTraceScene());
	m_Scenes.insert(scene);
	return true;
}

bool KRayTraceManager::RemoveRayTraceScene(IKRayTraceScenePtr& scene)
{
	auto it = m_Scenes.find(scene);
	if (it != m_Scenes.end())
		m_Scenes.erase(it);
	return true;
}