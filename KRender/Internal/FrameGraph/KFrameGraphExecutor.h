#pragma once
#include "Interface/IKRenderDevice.h"
#include "KFrameGraphHandle.h"

typedef std::unordered_map<std::string, IKRenderPassPtr> KFrameGraphRenderPassMap;

class KFrameGraphExecutor
{
protected:
	KFrameGraphRenderPassMap& m_RenderPassMap;
	IKCommandBufferPtr m_PrimaryBuffer;
	uint32_t m_FlightFrameIndex;
	uint32_t m_SwapChainImageIndex;
public:
	KFrameGraphExecutor(KFrameGraphRenderPassMap& map, IKCommandBufferPtr primaryBuffer, uint32_t frameIndex, uint32_t chainIndex);
	~KFrameGraphExecutor();

	inline IKCommandBufferPtr GetPrimaryBuffer() const { return m_PrimaryBuffer; }
	inline uint32_t GetFrameIndex() const { return m_FlightFrameIndex; }
	inline uint32_t GetChainIndex() const { return m_SwapChainImageIndex; }

	IKRenderPassPtr GetRenderPass(const std::string& name) const;
	bool SetRenderPass(const std::string& name, IKRenderPassPtr renderPass);
};