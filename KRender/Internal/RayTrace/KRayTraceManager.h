#pragma once
#include "Interface/IKRayTrace.h"

class KRayTraceManager : public IKRayTraceManager
{
protected:

public:
	KRayTraceManager();
	~KRayTraceManager();

	bool Init();
	bool UnInit();
	bool Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChain* swapChain, IKCommandBufferPtr primaryCommandBuffer);

	virtual bool AddRayTraceScene(IKRayTraceScenePtr& scene);
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene);
};