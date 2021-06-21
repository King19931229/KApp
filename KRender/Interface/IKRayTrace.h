#pragma once
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKRayTracePipeline.h"

struct IKRayTraceScene
{
	virtual ~IKRayTraceScene() {}
	virtual bool Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline) = 0;
	virtual bool UnInit() = 0;
};

typedef std::shared_ptr<IKRayTraceScene> IKRayTraceScenePtr;

struct IKRayTraceManager
{
	virtual ~IKRayTraceManager() {}
	virtual bool AcquireRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene) = 0;
};