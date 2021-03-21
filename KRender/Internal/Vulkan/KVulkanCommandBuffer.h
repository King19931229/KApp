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

	virtual bool Init(QueueFamilyIndex familyIndex);
	virtual bool UnInit();
	virtual bool Reset();

	inline VkCommandPool GetVkHandle() { return m_CommandPool; }
};

class KVulkanCommandBuffer : public IKCommandBuffer
{
protected:
	VkCommandBuffer m_CommandBuffer;
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
	virtual bool ExecuteAll(KCommandBufferList& commandBuffers);

	virtual bool BeginPrimary();
	virtual bool BeginSecondary(IKRenderPassPtr renderPass);
	virtual bool End();

	virtual bool Flush();

	virtual bool BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents conent);

	virtual bool ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color);
	virtual bool ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil);

	virtual bool EndRenderPass();

	virtual bool BeginQuery(IKQueryPtr query);
	virtual bool EndQuery(IKQueryPtr query);
	virtual bool ResetQuery(IKQueryPtr query);

	inline VkCommandBuffer GetVkHandle() { return m_CommandBuffer; }
	inline VkCommandBufferLevel GetVkBufferLevel() { return m_CommandLevel; }
};