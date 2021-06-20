#include "KRayTraceManager.h"

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
	return true;
}

bool KRayTraceManager::Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChain* swapChain, IKCommandBufferPtr primaryCommandBuffer)
{
	return true;
}

bool KRayTraceManager::AddRayTraceScene(IKRayTraceScenePtr& scene)
{
	return false;
}

bool KRayTraceManager::RemoveRayTraceScene(IKRayTraceScenePtr& scene)
{
	return false;
}