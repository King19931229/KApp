#pragma once
#include "Interface/IKFrameBuffer.h"
#include "KVulkanConfig.h"
#include <array>

class KVulkanRenderPass : public IKRenderPass
{
	static const uint32_t MAX_ATTACHMENT = 8;
protected:
	// Vulkan里面RenderPass与Framebuffer是绑死在一起的
	VkRenderPass								m_RenderPass;
	VkFramebuffer								m_FrameBuffer;
	std::array<IKFrameBufferPtr, MAX_ATTACHMENT>m_ColorFrameBuffers;
	std::array<KClearColor, MAX_ATTACHMENT>		m_ClearColors;
	std::vector<RenderPassInvalidCallback*>		m_InvalidCallbacks;
	IKFrameBufferPtr							m_DepthFrameBuffer;
	KClearDepthStencil							m_ClearDepthStencil;
	VkExtent2D									m_Extent;
	VkSampleCountFlagBits						m_MSAAFlag;
	bool										m_ToSwapChain;
public:
	KVulkanRenderPass();
	~KVulkanRenderPass();

	bool SetColorAttachment(uint32_t attachment, IKFrameBufferPtr color) override;
	bool SetDepthStencilAttachment(IKFrameBufferPtr depthStencil) override;

	bool SetClearColor(uint32_t attachment, const KClearColor& clearColor) override;
	bool SetClearDepthStencil(const KClearDepthStencil& clearDepthStencil) override;

	bool SetAsSwapChainPass(bool swapChain) override;
	bool HasColorAttachment() override;
	bool HasDepthStencilAttachment() override;
	uint32_t GetColorAttachmentCount() override;

	bool GetSize(uint32_t& width, uint32_t& height) override;

	bool RegisterInvalidCallback(RenderPassInvalidCallback* callback) override;
	bool UnRegisterInvalidCallback(RenderPassInvalidCallback* callback) override;

	bool Init() override;
	bool UnInit() override;

	inline VkRenderPass GetVkRenderPass() const { return m_RenderPass; }
	inline VkFramebuffer GetVkFrameBuffer() const { return m_FrameBuffer; }
	inline VkExtent2D GetVkExtent() const { return m_Extent; }
	inline VkSampleCountFlagBits GetMSAAFlag() const { return m_MSAAFlag; }

	typedef std::vector<VkClearValue> VkClearValueArray;
	bool GetVkClearValues(VkClearValueArray& clearValues);
};