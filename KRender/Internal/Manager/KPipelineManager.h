#pragma once
#include "Interface/IKRenderTarget.h"

#include <mutex>
#include <map>
#include <set>

class KPipelineManager
{
protected:
	IKRenderDevice* m_Device;

	typedef std::map<IKRenderTarget*, IKPipelineHandlePtr> RtPipelineHandleMap;
	typedef std::map<IKPipeline*, RtPipelineHandleMap> PipelineHandleMap;
	PipelineHandleMap m_RenderPipelineMap;
	std::mutex m_Lock;
public:
	KPipelineManager();
	~KPipelineManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool GetPipelineHandle(IKPipeline* pipeline, IKRenderTarget* target, IKPipelineHandlePtr& handle);
	bool InvaildateHandleByRt(IKRenderTarget* target);
	bool InvaildateHandleByPipeline(IKPipeline* pipeline);
	bool InvaildateAllHandle();

	bool CreatePipeline(IKPipelinePtr& pipeline);
	bool DestroyPipeline(IKPipelinePtr& pipeline);
};