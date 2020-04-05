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
	ASSERT_RESULT(m_Pipelines.empty());
}

bool KPipelineManager::Init(IKRenderDevice* device)
{
	m_Device = device;
	return true;
}

bool KPipelineManager::UnInit()
{
	for (IKPipelinePtr pipeline : m_Pipelines)
	{
		pipeline->UnInit();
	}
	m_Pipelines.clear();
	m_Device = nullptr;
	return true;
}

bool KPipelineManager::Reload()
{
	for (IKPipelinePtr pipeline : m_Pipelines)
	{
		pipeline->Reload();
	}
	return true;
}

bool KPipelineManager::InvaildateHandleByRt(IKRenderTargetPtr target)
{
	if (target)
	{
		for (IKPipelinePtr pipeline : m_Pipelines)
		{
			pipeline->InvaildHandle(target);
		}
		return true;
	}
	return false;
}

bool KPipelineManager::InvaildateHandleByPipeline(IKPipelinePtr pipeline)
{
	if (pipeline)
	{
		pipeline->Reload();
		return true;
	}
	return false;
}

bool KPipelineManager::CreatePipeline(IKPipelinePtr& pipeline)
{
	if (m_Device->CreatePipeline(pipeline))
	{
		m_Pipelines.insert(pipeline);
		return true;
	}
	return false;
}

bool KPipelineManager::DestroyPipeline(IKPipelinePtr& pipeline)
{
	if(pipeline)
	{
		auto it = m_Pipelines.find(pipeline);
		if (it != m_Pipelines.end())
		{
			m_Pipelines.erase(it);
		}
		pipeline->UnInit();
		pipeline = nullptr;
		return true;
	}
	return false;
}