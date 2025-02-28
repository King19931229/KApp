#pragma once
#include "Interface/IKCommandBuffer.h"
#include "KVulkanConfig.h"

class KVulkanCommandPool : public IKCommandPool
{
protected:
	std::vector<VkCommandPool> m_CommandPools;
	struct BufferUsage
	{
		std::vector<IKCommandBufferPtr> buffers;
		size_t currentActive;
		BufferUsage()
		{
			currentActive = 0;
		}
		void Reset(CommmandBufferReset resetMode);
	};
	BufferUsage m_PrimaryUsage;
	BufferUsage m_SecondaryUsage;
	CommmandBufferReset m_ResetMode;
	std::string m_Name;
public:
	KVulkanCommandPool();
	~KVulkanCommandPool();

	virtual bool Init(QueueCategory queue, uint32_t index, CommmandBufferReset resetMode);
	virtual bool UnInit();
	virtual bool Reset();
	virtual bool SetDebugName(const char* name);

	virtual IKCommandBufferPtr Request(CommandBufferLevel level);

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

	bool Init(KVulkanCommandPool *pool, CommandBufferLevel level);
	bool UnInit();
	bool Reset(CommmandBufferReset resetMode);

	virtual bool SetDebugName(const char* name);

	virtual bool SetViewport(const KViewPortArea& area);
	virtual bool SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope);

	virtual bool Render(const KRenderCommand& command);

	virtual bool Execute(IKCommandBufferPtr buffer);
	virtual bool ExecuteAll(KCommandBufferList& commandBuffers);

	virtual bool BeginPrimary();
	virtual bool BeginSecondary(IKRenderPassPtr renderPass);
	virtual bool End();

	virtual bool Flush();

	virtual bool BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents conent);

	virtual bool ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color);
	virtual bool ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil);

	virtual bool EndRenderPass();

	virtual bool BeginDebugMarker(const std::string& marker, const glm::vec4& color);
	virtual bool EndDebugMarker();

	virtual bool BeginQuery(IKQueryPtr query);
	virtual bool EndQuery(IKQueryPtr query);
	virtual bool ResetQuery(IKQueryPtr query);

	virtual bool TransitionIndirect(IKStorageBufferPtr buf);

	virtual bool Transition(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	virtual bool TransitionOwnership(IKFrameBufferPtr buf, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	virtual bool TransitionMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);

	virtual bool Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest);

	VkCommandBuffer GetVkHandle();
	inline VkCommandBufferLevel GetVkBufferLevel() { return m_CommandLevel; }
};