#pragma once
#include "Interface/IKFrameBuffer.h"
#include "KVulkanConfig.h"

class KVulkanRenderPass : public IKRenderPass
{
protected:
	// Vulkan里面RenderPass与Framebuffer是绑死在一起的
	VkRenderPass		m_RenderPass;
	VkFramebuffer		m_FrameBuffer;

	IKFrameBufferPtr	m_ColorFrameBuffer;
	IKFrameBufferPtr	m_DepthFrameBuffer;
	bool				m_ToSwapChain;
public:
	KVulkanRenderPass();
	~KVulkanRenderPass();

	bool SetColor(IKFrameBufferPtr color);
	bool SetDepthStencil(IKFrameBufferPtr depthStencil);
	bool SetAsSwapChainPass(bool swapChain);
	bool Init();
	bool UnInit();

	inline VkRenderPass GetVkRenderPass() { return m_RenderPass; }
	inline VkFramebuffer GetVkFrameBuffer() { return m_FrameBuffer; }
};