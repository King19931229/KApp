#pragma once
#include "Interface/IKRenderTarget.h"

#include <mutex>
#include <unordered_set>

class KPipelineManager
{
protected:
	typedef std::unordered_set<IKPipelinePtr> PipelineSet;

	IKRenderDevice* m_Device;	
	PipelineSet m_Pipelines;
public:
	KPipelineManager();
	~KPipelineManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool InvaildateHandleByRt(IKRenderTargetPtr target);
	bool InvaildateHandleByPipeline(IKPipelinePtr pipeline);

	bool CreatePipeline(IKPipelinePtr& pipeline);
	bool DestroyPipeline(IKPipelinePtr& pipeline);
};