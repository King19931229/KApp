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

	virtual bool Init(IKCommandPool* pool, CommandBufferLevel level);
	virtual bool UnInit();

	virtual bool SetViewport(IKRenderTarget* target);
	virtual bool SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope);

	virtual bool Render(const KRenderCommand& command);

	virtual bool Execute(IKCommandBuffer* buffer);
	virtual bool ExecuteAll(KCommandBufferList& commandBuffers);

	virtual bool BeginPrimary();
	virtual bool BeginSecondary(IKRenderTarget* target);
	virtual bool End();

	virtual bool BeginRenderPass(IKRenderTarget* target, SubpassContents conent);
	virtual bool EndRenderPass();

	inline VkCommandBuffer GetVkHandle() { return m_CommandBuffer; }
	inline VkCommandBufferLevel GetVkBufferLevel() { return m_CommandLevel; }
};