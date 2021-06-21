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
	m_Scenes.clear();
	return true;
}

bool KRayTraceManager::Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChain* swapChain, IKCommandBufferPtr primaryCommandBuffer)
{
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