#pragma once
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKRayTracePipline.h"

struct IKRayTraceScene
{
	virtual ~IKRayTraceScene() {}
	virtual bool Init(IKRenderScene* scene, IKRayTracePipline& pipline) = 0;
	virtual bool UnInit() = 0;
};

typedef std::shared_ptr<IKRayTraceScene> IKRayTraceScenePtr;

struct IKRayTraceManager
{
	virtual ~IKRayTraceManager() {}
	virtual bool AddRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene) = 0;
};