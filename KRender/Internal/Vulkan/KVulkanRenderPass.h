#pragma once
#include "Interface/IKRenderPass.h"
#include "KVulkanConfig.h"
#include <array>

class KVulkanRenderPass : public IKRenderPass
{
	static const uint32_t MAX_ATTACHMENT = 8;
	
protected:
	// Vulkan里面RenderPass与Framebuffer是绑死在一起的
	VkRenderPass										m_RenderPass;
	VkFramebuffer										m_FrameBuffer;

	std::array<IKFrameBufferPtr, MAX_ATTACHMENT>		m_ColorFrameBuffers;
	std::array<KClearColor, MAX_ATTACHMENT>				m_ClearColors;
	std::array<KRenderPassOperation, MAX_ATTACHMENT>	m_OpColors;

	std::vector<RenderPassInvalidCallback*>				m_InvalidCallbacks;

	IKFrameBufferPtr									m_DepthFrameBuffer;
	KClearDepthStencil									m_ClearDepthStencil;
	KRenderPassOperation								m_OpDepth;
	KRenderPassOperation								m_OpStencil;

	KViewPortArea										m_ViewPortArea;
	VkSampleCountFlagBits								m_MSAAFlag;
	bool												m_ToSwapChain;
	size_t												m_AttachmentHash;

	std::string											m_Name;

	size_t CalcAttachmentHash(uint32_t mipmap);
public:
	KVulkanRenderPass();
	~KVulkanRenderPass();

	bool SetColorAttachment(uint32_t attachment, IKFrameBufferPtr color) override;
	bool SetDepthStencilAttachment(IKFrameBufferPtr depthStencil) override;

	bool SetClearColor(uint32_t attachment, const KClearColor& clearColor) override;
	bool SetClearDepthStencil(const KClearDepthStencil& clearDepthStencil) override;

	bool SetOpColor(uint32_t attachment, LoadOperation loadOp, StoreOperation storeOp) override;;
	bool SetOpDepthStencil(LoadOperation depthLoadOp, StoreOperation depthStoreOp, LoadOperation stencilLoadOp, StoreOperation stencilStoreOp) override;

	bool SetAsSwapChainPass(bool swapChain) override;
	bool HasColorAttachment() override;
	bool HasDepthStencilAttachment() override;
	uint32_t GetColorAttachmentCount() override;

	const KViewPortArea& GetViewPort() override;

	bool RegisterInvalidCallback(RenderPassInvalidCallback* callback) override;
	bool UnRegisterInvalidCallback(RenderPassInvalidCallback* callback) override;

	bool Init(uint32_t mipmap) override;
	bool UnInit() override;

	bool SetDebugName(const char* name) override;
	const char* GetDebugName() const override;

	inline VkRenderPass GetVkRenderPass() const { return m_RenderPass; }
	inline VkFramebuffer GetVkFrameBuffer() const { return m_FrameBuffer; }
	inline VkSampleCountFlagBits GetMSAAFlag() const { return m_MSAAFlag; }

	typedef std::vector<VkClearValue> VkClearValueArray;
	bool GetVkClearValues(VkClearValueArray& clearValues);
};