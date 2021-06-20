#pragma once
#include "Interface/IKRayTrace.h"

class KRayTraceScene : public IKRayTraceScene
{
protected:

public:
	KRayTraceScene();
	~KRayTraceScene();

	virtual bool Init(IKRenderScene* scene, IKRayTracePipline& pipline);
	virtual bool UnInit();
};