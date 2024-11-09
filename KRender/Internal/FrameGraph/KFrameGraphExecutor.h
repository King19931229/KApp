#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Render/KRHICommandList.h"
#include "KFrameGraphHandle.h"

typedef std::unordered_map<std::string, IKRenderPassPtr> KFrameGraphRenderPassMap;

class KFrameGraphExecutor
{
protected:
	KFrameGraphRenderPassMap& m_RenderPassMap;
	KRHICommandList& m_CommandList;
	uint32_t m_SwapChainImageIndex;
public:
	KFrameGraphExecutor(KFrameGraphRenderPassMap& map, KRHICommandList& commandList, uint32_t chainIndex);
	~KFrameGraphExecutor();

	inline KRHICommandList& GetCommandList() const { return m_CommandList; }
	inline uint32_t GetChainIndex() const { return m_SwapChainImageIndex; }

	IKRenderPassPtr GetRenderPass(const std::string& name) const;
	bool SetRenderPass(const std::string& name, IKRenderPassPtr renderPass);
};