#pragma once
#include "Interface/IKCommandBuffer.h"
#include "KVulkanConfig.h"

class KVulkanCommandPool : public IKCommandPool
{
protected:
	VkCommandPool m_CommandPool;
public:
	KVulkanCommandPool();
	~KVulkanCommandPool();

	virtual bool Init(QueueCategory queue, uint32_t index);
	virtual bool UnInit();
	virtual bool Reset();

	inline VkCommandPool GetVkHandle() { return m_CommandPool; }
};

class KVulkanCommandBuffer : public IKCommandBuffer
{
protected:
	std::vector<VkCommandBuffer> m_CommandBuffers;
	VkCommandPool m_ParentPool;
	VkCommandBufferLevel m_CommandLevel;
public:
	KVulkanCommandBuffer();
	virtual ~KVulkanCommandBuffer();

	virtual bool Init(IKCommandPoolPtr pool, CommandBufferLevel level);
	virtual bool UnInit();

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
	virtual bool Translate(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout layout);

	virtual bool TranslateMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	virtual bool TranslateMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout layout);

	virtual bool Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest);

	VkCommandBuffer GetVkHandle();
	inline VkCommandBufferLevel GetVkBufferLevel() { return m_CommandLevel; }
};