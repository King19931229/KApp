#pragma once
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKRayTracePipeline.h"
#include "KRender/Interface/IKRenderCommand.h"

struct IKRayTraceScene
{
	virtual ~IKRayTraceScene() {}
	virtual bool Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline) = 0;
	virtual bool UnInit() = 0;
	virtual bool UpdateCamera() = 0;
	virtual bool EnableDebugDraw() = 0;
	virtual bool DisableDebugDraw() = 0;
	virtual bool EnableAutoUpdateImageSize(float scale) = 0;
	virtual bool EnableCustomImageSize(uint32_t width, uint32_t height) = 0;
	virtual bool GetDebugRenderCommand(KRenderCommandList& commands) = 0;
	virtual bool Execute(IKCommandBufferPtr primaryBuffer) = 0;

	virtual IKRayTracePipeline* GetRayTracePipeline() = 0;
	virtual const KCamera* GetCamera() = 0;
};

typedef std::shared_ptr<IKRayTraceScene> IKRayTraceScenePtr;

struct IKRayTraceManager
{
	virtual ~IKRayTraceManager() {}
	virtual bool AcquireRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene) = 0;
	virtual bool GetAllRayTraceScene(std::unordered_set<IKRayTraceScenePtr>& scenes) = 0;
	virtual bool GetDebugRenderCommand(KRenderCommandList& commands) = 0;
};