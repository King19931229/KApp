#pragma once
#include "Interface/IKRayTrace.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKRenderCommand.h"
#include "Internal/KVertexDefinition.h"

class KRayTraceManager
{
	friend class KRayTraceScene;
protected:
	std::unordered_set<IKRayTraceScenePtr> m_Scenes;
public:
	KRayTraceManager();
	~KRayTraceManager();

	bool Init();
	bool UnInit();
	bool Execute(KRHICommandList& commandList);
	bool UpdateCamera();
	bool Resize(uint32_t width, uint32_t height);
	bool Reload();

	virtual bool CreateRayTraceScene(IKRayTraceScenePtr& scene);
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene);
	virtual bool GetAllRayTraceScene(std::unordered_set<IKRayTraceScenePtr>& scenes);
	virtual bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);
};