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
	IKFrameBufferPtr							m_DepthFrameBuffer;
	bool										m_ToSwapChain;
public:
	KVulkanRenderPass();
	~KVulkanRenderPass();

	bool SetColor(uint32_t attachment, IKFrameBufferPtr color) override;
	bool SetDepthStencil(IKFrameBufferPtr depthStencil) override;
	bool SetAsSwapChainPass(bool swapChain) override;
	bool HasColorAttachment() override;
	bool HasDepthStencilAttachment() override;
	bool Init() override;
	bool UnInit() override;

	inline VkRenderPass GetVkRenderPass() {	return m_RenderPass; }
	inline VkFramebuffer GetVkFrameBuffer(uint32_t index) { return m_FrameBuffer; }
};