#pragma once
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKRayTracePipeline.h"
#include "KRender/Interface/IKRenderCommand.h"

struct IKRayTraceManager
{
	virtual ~IKRayTraceManager() {}
	virtual bool CreateRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool GetAllRayTraceScene(std::unordered_set<IKRayTraceScenePtr>& scenes) = 0;
	virtual bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) = 0;
};