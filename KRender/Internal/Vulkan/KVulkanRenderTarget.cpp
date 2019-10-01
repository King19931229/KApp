#include "KVulkanRenderTarget.h"

#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"

KVulkanRenderTarget::KVulkanRenderTarget()
	: m_bMsaaCreated(false),
	m_MsaaFlag(VK_SAMPLE_COUNT_1_BIT)
{
	ZERO_MEMORY(m_RenderPass);
	ZERO_MEMORY(m_FrameBuffer);
	ZERO_MEMORY(m_ColorFormat);
	ZERO_MEMORY(m_ColorClear);
	ZERO_MEMORY(m_DepthStencilClear);
	ZERO_MEMORY(m_Extend);

	ZERO_MEMORY(m_ColorImage);
	ZERO_MEMORY(m_ColorImageView);

	ZERO_MEMORY(m_MsaaImage);
	ZERO_MEMORY(m_MsaaImageView);

	ZERO_MEMORY(m_MsaaAlloc);
}

KVulkanRenderTarget::~KVulkanRenderTarget()
{
	ASSERT_RESULT(m_RenderPass == (VkRenderPass)nullptr);
	ASSERT_RESULT(m_FrameBuffer == (VkFramebuffer)nullptr);
	ASSERT_RESULT(m_ColorImageView == (VkImageView)nullptr);
	ASSERT_RESULT(!m_bMsaaCreated);
	ASSERT_RESULT(m_pDepthBuffer == nullptr);
}

bool KVulkanRenderTarget::SetSize(size_t width, size_t height)
{
	m_Extend.width = static_cast<uint32_t>(width);
	m_Extend.height = static_cast<uint32_t>(height);
	return true;
}

bool KVulkanRenderTarget::SetColorClear(float r, float g, float b, float a)
{
	VkClearColorValue clear = {r, g, b, a};
	m_ColorClear.color = clear;
	return true;
}

bool KVulkanRenderTarget::SetDepthStencilClear(float depth, unsigned int stencil)
{
	VkClearDepthStencilValue clear = {depth, stencil};
	m_DepthStencilClear.depthStencil = clear;
	return true;
}

bool KVulkanRenderTarget::CreateImage(void* imageHandle, void* imageFormatHandle, bool bDepthStencil, unsigned short uMsaaCount)
{
	ASSERT_RESULT(imageHandle != nullptr);
	ASSERT_RESULT(imageFormatHandle != nullptr);

	ASSERT_RESULT(!m_pDepthBuffer);

	m_ColorImage = *((VkImage*)imageHandle);
	m_ColorFormat = *((VkFormat*)imageFormatHandle);

	KVulkanInitializer::CreateVkImageView(m_ColorImage, m_ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_ColorImageView);

	if(uMsaaCount > 1)
	{
		ASSERT_RESULT(KVulkanHelper::QueryMSAASupport(KVulkanHelper::MST_BOTH, uMsaaCount , m_MsaaFlag));

		KVulkanInitializer::CreateVkImage(m_Extend.width, m_Extend.height, 1,
			1,
			m_MsaaFlag,
			VK_IMAGE_TYPE_2D, m_ColorFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_MsaaImage, m_MsaaAlloc);

		KVulkanInitializer::CreateVkImageView(m_MsaaImage,
			m_ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_MsaaImageView);

		KVulkanHelper::TransitionImageLayout(m_MsaaImage,
			m_ColorFormat, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		m_bMsaaCreated = true;
	}
	else
	{
		m_MsaaFlag = VK_SAMPLE_COUNT_1_BIT;
		m_bMsaaCreated = false;
	}

	if(bDepthStencil)
	{
		m_pDepthBuffer = KVulkanDepthBufferPtr(new KVulkanDepthBuffer());
		ASSERT_RESULT(m_pDepthBuffer->InitDevice(m_Extend.width, m_Extend.height, uMsaaCount, true));
	}

	return true;
}

bool KVulkanRenderTarget::CreateFramebuffer()
{	
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	// 声明 Color Attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_ColorFormat;
	colorAttachment.samples = m_bMsaaCreated ? m_MsaaFlag : VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = m_bMsaaCreated ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// 声明 Depth Attachment
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_pDepthBuffer->GetVkFormat();
	depthAttachment.samples = m_pDepthBuffer->GetVkSampleCountFlagBits();

	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// 声明 MSAA Resolve Attachment 不一定用到
	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = m_ColorFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// 声明Attachment引用结构
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// 声明子通道
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = m_bMsaaCreated ? &colorAttachmentResolveRef : nullptr;

	// 从属依赖
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// 创建渲染通道	
	std::vector<VkAttachmentDescription> descs;
	descs.push_back(colorAttachment);
	descs.push_back(depthAttachment);
	if(m_bMsaaCreated)
	{
		descs.push_back(colorAttachmentResolve);
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = (uint32_t)descs.size();
	renderPassInfo.pAttachments = descs.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_ASSERT_RESULT(vkCreateRenderPass(KVulkanGlobal::device, &renderPassInfo, nullptr, &m_RenderPass));

	std::vector<VkImageView> imageViews;

	if(m_bMsaaCreated)
	{
		imageViews.push_back(m_MsaaImageView);
		imageViews.push_back(m_pDepthBuffer->GetVkImageView());
		imageViews.push_back(m_ColorImageView);
	}
	else
	{
		imageViews.push_back(m_ColorImageView);
		imageViews.push_back(m_pDepthBuffer->GetVkImageView());
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_RenderPass;
	framebufferInfo.attachmentCount = (uint32_t)imageViews.size();
	framebufferInfo.pAttachments = imageViews.data();
	framebufferInfo.width = m_Extend.width;
	framebufferInfo.height = m_Extend.height;
	framebufferInfo.layers = 1;

	VK_ASSERT_RESULT(vkCreateFramebuffer(KVulkanGlobal::device, &framebufferInfo, nullptr, &m_FrameBuffer));
	
	return true;
}


bool KVulkanRenderTarget::InitFromImage(void* imageHandle, void* imageFormatHandle, bool bDepthStencil, unsigned short uMsaaCount)
{
	ASSERT_RESULT(CreateImage(imageHandle, imageFormatHandle, bDepthStencil, uMsaaCount));
	ASSERT_RESULT(CreateFramebuffer());	
	return true;
}

bool KVulkanRenderTarget::UnInit()
{
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	if(m_RenderPass)
	{
		vkDestroyRenderPass(KVulkanGlobal::device, m_RenderPass, nullptr);
		ZERO_MEMORY(m_RenderPass);
	}

	if(m_FrameBuffer)
	{
		vkDestroyFramebuffer(KVulkanGlobal::device, m_FrameBuffer, nullptr);
		ZERO_MEMORY(m_FrameBuffer);
	}

	if(m_ColorImageView)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_ColorImageView, nullptr);
		ZERO_MEMORY(m_ColorImageView);
	}

	if(m_bMsaaCreated)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_MsaaImageView, nullptr);
		KVulkanInitializer::FreeVkImage(m_MsaaImage, m_MsaaAlloc);
		m_bMsaaCreated = false;
	}

	if(m_pDepthBuffer)
	{
		m_pDepthBuffer->UnInit();
		m_pDepthBuffer = nullptr;
	}

	return true;
}