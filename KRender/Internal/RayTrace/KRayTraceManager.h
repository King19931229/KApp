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
	bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex, uint32_t chainIndex);
	bool UpdateCamera(uint32_t frameIndex);

	virtual bool AcquireRayTraceScene(IKRayTraceScenePtr& scene);
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene);
};