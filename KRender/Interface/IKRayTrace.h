#pragma once
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKRayTracePipeline.h"
#include "KRender/Interface/IKRenderCommand.h"

struct IKRayTraceScene
{
	virtual ~IKRayTraceScene() {}
	virtual bool Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline) = 0;
	virtual bool UnInit() = 0;
	virtual bool UpdateCamera(uint32_t frameIndex) = 0;
	virtual bool EnableDebugDraw(float x, float y, float width, float height) = 0;
	virtual bool DisableDebugDraw() = 0;
	virtual bool GetDebugRenderCommand(KRenderCommandList& commands) = 0;
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex) = 0;
};

typedef std::shared_ptr<IKRayTraceScene> IKRayTraceScenePtr;

struct IKRayTraceManager
{
	virtual ~IKRayTraceManager() {}
	virtual bool AcquireRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool GetDebugRenderCommand(KRenderCommandList& commands) = 0;
};