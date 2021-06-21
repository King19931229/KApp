#pragma once
#include "Interface/IKRayTrace.h"
#include "Interface/IKBuffer.h"

class KRayTraceManager : public IKRayTraceManager
{
protected:
	std::unordered_set<IKRayTraceScenePtr> m_Scenes;
public:
	KRayTraceManager();
	~KRayTraceManager();

	bool Init();
	bool UnInit();
	bool Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChain* swapChain, IKCommandBufferPtr primaryCommandBuffer);

	virtual bool AcquireRayTraceScene(IKRayTraceScenePtr& scene);
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene);
};