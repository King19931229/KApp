#pragma once
#include "Interface/IKCommandBuffer.h"
#include "KVulkanConfig.h"

class KVulkanCommandPool : public IKCommandPool
{
protected:
	std::vector<VkCommandPool> m_CommandPools;
public:
	KVulkanCommandPool();
	~KVulkanCommandPool();

	virtual bool Init(QueueCategory queue, uint32_t index);
	virtual bool UnInit();
	virtual bool Reset();
	virtual bool SetDebugName(const char* name);

	inline std::vector<VkCommandPool> GetVkHandles() { return m_CommandPools; }
};

class KVulkanCommandBuffer : public IKCommandBuffer
{
protected:
	std::vector<VkCommandBuffer> m_CommandBuffers;
	std::vector<VkCommandPool> m_ParentPools;
	VkCommandBufferLevel m_CommandLevel;
public:
	KVulkanCommandBuffer();
	virtual ~KVulkanCommandBuffer();

	virtual bool Init(IKCommandPoolPtr pool, CommandBufferLevel level);
	virtual bool UnInit();

	virtual bool SetDebugName(const char* name);

	virtual bool SetViewport(const KViewPortArea& area);
	virtual bool SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope);

	virtual bool Render(const KRenderCommand& command);

	virtual bool Execute(IKCommandBufferPtr buffer);
	virtual bool ExecuteAll(KCommandBufferList& commandBuffers, bool clearAfterExecute);

	virtual bool BeginPrimary();
	virtual bool BeginSecondary(IKRenderPassPtr renderPass);
	virtual bool End();

	virtual bool Flush();

	virtual bool BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents conent);

	virtual bool ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color);
	virtual bool ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil);

	virtual bool EndRenderPass();

	virtual bool BeginDebugMarker(const std::string& marker, const glm::vec4 color);
	virtual bool EndDebugMarker();

	virtual bool BeginQuery(IKQueryPtr query);
	virtual bool EndQuery(IKQueryPtr query);
	virtual bool ResetQuery(IKQueryPtr query);

	virtual bool Translate(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	virtual bool TranslateOwnership(IKFrameBufferPtr buf, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	virtual bool TranslateMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);

	virtual bool Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest);

	VkCommandBuffer GetVkHandle();
	inline VkCommandBufferLevel GetVkBufferLevel() { return m_CommandLevel; }
};