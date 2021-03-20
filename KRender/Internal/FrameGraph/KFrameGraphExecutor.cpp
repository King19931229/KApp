#include "KFrameGraphExecutor.h"

KFrameGraphExecutor::KFrameGraphExecutor(KFrameGraphRenderPassMap& map, IKCommandBufferPtr primaryBuffer, uint32_t frameIndex, uint32_t chainIndex)
	: m_RenderPassMap(map),
	m_PrimaryBuffer(primaryBuffer),
	m_FlightFrameIndex(frameIndex),
	m_SwapChainImageIndex(chainIndex)
{
}

KFrameGraphExecutor::~KFrameGraphExecutor()
{
}

IKRenderPassPtr KFrameGraphExecutor::GetRenderPass(const std::string& name) const
{
	auto it = m_RenderPassMap.find(name);
	if (it != m_RenderPassMap.end())
	{
		return it->second;
	}
	return nullptr;
}

bool KFrameGraphExecutor::SetRenderPass(const std::string& name, IKRenderPassPtr renderPass)
{
	auto it = m_RenderPassMap.find(name);
	if (renderPass)
	{
		if (it != m_RenderPassMap.end())
		{
			it->second = renderPass;
		}
		else
		{
			m_RenderPassMap.insert({ name, renderPass });
		}
	}
	else
	{
		m_RenderPassMap.erase(it);
	}
	return true;
}