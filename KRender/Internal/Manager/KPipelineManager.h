#pragma once
#include "Interface/IKRenderTarget.h"

#include <mutex>
#include <unordered_map>

class KPipelineManager
{
protected:
	IKRenderDevice* m_Device;

	typedef std::unordered_map<IKRenderTargetPtr, IKPipelineHandlePtr> RtPipelineHandleMap;
	typedef std::unordered_map<IKPipelinePtr, RtPipelineHandleMap> PipelineHandleMap;
	PipelineHandleMap m_RenderPipelineMap;
	std::mutex m_Lock;
public:
	KPipelineManager();
	~KPipelineManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool GetPipelineHandle(IKPipelinePtr pipeline, IKRenderTargetPtr target, IKPipelineHandlePtr& handle, bool async);
	bool InvaildateHandleByRt(IKRenderTargetPtr target);
	bool InvaildateHandleByPipeline(IKPipelinePtr pipeline);
	bool InvaildateAllHandle();

	bool CreatePipeline(IKPipelinePtr& pipeline);
	bool DestroyPipeline(IKPipelinePtr& pipeline);
};