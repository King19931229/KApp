#pragma once
#include "Interface/IKRenderTarget.h"

#include <map>
#include <set>

class KPipelineManager
{
protected:
	typedef std::set<IKPipelinePtr> PipelineSet;
	typedef std::map<IKRenderTargetPtr, PipelineSet> RenderPipelineMap;	
	RenderPipelineMap m_RenderPipelineMap;
public:
	KPipelineManager();
	~KPipelineManager();

	bool Manage(IKRenderTargetPtr target, IKPipelinePtr pipeline);
	bool UnManage(IKRenderTargetPtr target, IKPipelinePtr pipeline);

	bool ReInit(IKRenderTargetPtr target);
};