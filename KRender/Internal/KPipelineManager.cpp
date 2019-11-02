#include "KPipelineManager.h"
#include "Interface/IKPipeline.h"

KPipelineManager::KPipelineManager()
{

}

KPipelineManager::~KPipelineManager()
{

}

bool KPipelineManager::Manage(IKRenderTargetPtr target, IKPipelinePtr pipeline)
{
	RenderPipelineMap::iterator it = m_RenderPipelineMap.find(target);
	if(it == m_RenderPipelineMap.end())
	{
		PipelineSet set;
		set.insert(pipeline);
		m_RenderPipelineMap[target] = std::move(set);
	}
	else
	{
		PipelineSet& set = it->second;
		set.insert(pipeline);
	}
	return true;
}

bool KPipelineManager::UnManage(IKRenderTargetPtr target, IKPipelinePtr pipeline)
{
	RenderPipelineMap::iterator it = m_RenderPipelineMap.find(target);
	if(it == m_RenderPipelineMap.end())
	{
		return false;
	}
	PipelineSet& set = it->second;
	PipelineSet::iterator setIt = set.find(pipeline);
	if(setIt != set.end())
	{
		set.erase(setIt);
		if(set.empty())
		{
			m_RenderPipelineMap.erase(it);
		}
		return true;
	}
	return false;
}

bool KPipelineManager::ReInit(IKRenderTargetPtr target)
{
	RenderPipelineMap::iterator it = m_RenderPipelineMap.find(target);
	if(it == m_RenderPipelineMap.end())
	{
		return false;
	}
	for(IKPipelinePtr pipeline : it->second)
	{
		pipeline->UnInit();
		pipeline->Init();
	}
	return true;
}