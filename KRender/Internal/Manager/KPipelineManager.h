#pragma once
#include "Interface/IKRenderTarget.h"

#include <mutex>
#include <map>
#include <set>

class KPipelineManager
{
protected:
	IKRenderDevice* m_Device;

	typedef std::map<IKRenderTargetPtr, IKPipelineHandlePtr> RtPipelineHandleMap;
	typedef std::map<IKPipelinePtr, RtPipelineHandleMap> PipelineHandleMap;
	PipelineHandleMap m_RenderPipelineMap;
	std::mutex m_Lock;
public:
	KPipelineManager();
	~KPipelineManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool GetPipelineHandle(IKPipelinePtr pipeline, IKRenderTargetPtr target, IKPipelineHandlePtr& handle);
	bool InvaildateHandleByRt(IKRenderTargetPtr target);
	bool InvaildateHandleByPipeline(IKPipelinePtr pipeline);
	bool InvaildateAllHandle();

	bool CreatePipeline(IKPipelinePtr& pipeline);
	bool DestroyPipeline(IKPipelinePtr& pipeline);
};