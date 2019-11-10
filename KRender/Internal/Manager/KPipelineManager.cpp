#include "KPipelineManager.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include <assert.h>

KPipelineManager::KPipelineManager()
	: m_Device(nullptr)
{

}

KPipelineManager::~KPipelineManager()
{
	ASSERT_RESULT(m_RenderPipelineMap.empty());
}

bool KPipelineManager::Init(IKRenderDevice* device)
{
	m_Device = device;
	return true;
}

bool KPipelineManager::UnInit()
{
	m_Device = nullptr;
	return true;
}

bool KPipelineManager::GetPipelineHandle(IKPipeline* pipeline, IKRenderTarget* target, IKPipelineHandlePtr& handle)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	PipelineHandleMap::iterator it = m_RenderPipelineMap.find(pipeline);
	if(it == m_RenderPipelineMap.end())
	{
		RtPipelineHandleMap handleMap;
		if(m_Device->CreatePipelineHandle(handle))
		{
			if(handle->Init(pipeline, target))
			{
				handleMap.insert(RtPipelineHandleMap::value_type(target, handle));
				m_RenderPipelineMap.insert(PipelineHandleMap::value_type(pipeline, std::move(handleMap)));
				return true;
			}
		}
		return false;
	}
	else
	{
		RtPipelineHandleMap& handleMap = it->second;
		RtPipelineHandleMap::iterator it2 = handleMap.find(target);
		if(it2 == handleMap.end())
		{
			if(m_Device->CreatePipelineHandle(handle))
			{
				if(handle->Init(pipeline, target))
				{
					handleMap.insert(RtPipelineHandleMap::value_type(target, handle));
					return true;
				}
			}
			return false;
		}
		else
		{
			handle = it2->second;
		}
		return true;
	}
}

bool KPipelineManager::InvaildateHandleByRt(IKRenderTarget* target)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	for(PipelineHandleMap::iterator it = m_RenderPipelineMap.begin();
		it != m_RenderPipelineMap.end();)
	{
		RtPipelineHandleMap& handleMap = it->second;
		RtPipelineHandleMap::iterator it2 = handleMap.find(target);
		if(it2 != handleMap.end())
		{
			IKPipelineHandlePtr& handle = it2->second;
			handle->UnInit();

			handleMap.erase(it2);
		}

		if(handleMap.empty())
		{
			it = m_RenderPipelineMap.erase(it);
		}
		else
		{
			++it;
		}
	}
	return true;
}

bool KPipelineManager::InvaildateHandleByPipeline(IKPipeline* pipeline)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	PipelineHandleMap::iterator it = m_RenderPipelineMap.find(pipeline);
	if(it != m_RenderPipelineMap.end())
	{
		RtPipelineHandleMap& handleMap = it->second;
		for(RtPipelineHandleMap::iterator it2 = handleMap.begin(), it2End = handleMap.end();
			it2 != it2End; ++it2)
		{
			IKPipelineHandlePtr& handle = it2->second;
			handle->UnInit();
		}
		handleMap.clear();

		m_RenderPipelineMap.erase(it);
	}
	return true;
}

bool KPipelineManager::InvaildateAllHandle()
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	for(PipelineHandleMap::iterator it = m_RenderPipelineMap.begin(), itEnd = m_RenderPipelineMap.end();
		it != itEnd; ++it)
	{
		RtPipelineHandleMap& handleMap = it->second;
		for(RtPipelineHandleMap::iterator it2 = handleMap.begin(), it2End = handleMap.end();
			it2 != it2End; ++it2)
		{
			IKPipelineHandlePtr& handle = it2->second;
			handle->UnInit();
		}
		handleMap.clear();
	}
	m_RenderPipelineMap.clear();
	return true;
}

bool KPipelineManager::CreatePipeline(IKPipelinePtr& pipeline)
{
	return m_Device->CreatePipeline(pipeline);
}

bool KPipelineManager::DestroyPipeline(IKPipelinePtr& pipeline)
{
	if(pipeline)
	{
		pipeline->UnInit();
		pipeline = nullptr;
	}
	return true;
}