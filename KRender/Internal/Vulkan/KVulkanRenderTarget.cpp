#include "KVulkanRenderTarget.h"

#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"

KVulkanRenderTarget::KVulkanRenderTarget()
	: m_bMsaaCreated(false),
	m_bDepthStencilCreated(false),
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

	ZERO_MEMORY(m_DepthFormat);
	ZERO_MEMORY(m_DepthImage);
	ZERO_MEMORY(m_DepthImageView);
}

KVulkanRenderTarget::~KVulkanRenderTarget()
{
	ASSERT_RESULT(m_RenderPass == VK_NULL_HANDLE);
	ASSERT_RESULT(m_FrameBuffer == VK_NULL_HANDLE);
	ASSERT_RESULT(m_MsaaImage == VK_NULL_HANDLE);	
	ASSERT_RESULT(m_ColorImageView == VK_NULL_HANDLE);
	ASSERT_RESULT(m_DepthImage == VK_NULL_HANDLE);
	ASSERT_RESULT(m_DepthImageView == VK_NULL_HANDLE);	
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

VkFormat KVulkanRenderTarget::FindDepthFormat(bool bStencil)
{
	VkFormat format = VK_FORMAT_MAX_ENUM;
	std::vector<VkFormat> candidates;

	if(!bStencil)
	{
		candidates.push_back(VK_FORMAT_D32_SFLOAT);
	}
	candidates.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
	candidates.push_back(VK_FORMAT_D24_UNORM_S8_UINT);

	ASSERT_RESULT(KVulkanHelper::FindSupportedFormat(candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format));

	return format;
}

bool KVulkanRenderTarget::CreateImage(void* imageHandle, void* imageFormatHandle, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	ASSERT_RESULT(imageHandle != nullptr);
	ASSERT_RESULT(imageFormatHandle != nullptr);

	ASSERT_RESULT(!m_bMsaaCreated);
	ASSERT_RESULT(!m_bDepthStencilCreated);

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

	m_bDepthStencilCreated = bDepth;
	if(bDepth)
	{
		m_DepthFormat = FindDepthFormat(bStencil);

		KVulkanInitializer::CreateVkImage(m_Extend.width,
			m_Extend.height,
			1,
			1,
			m_MsaaFlag,
			VK_IMAGE_TYPE_2D,
			m_DepthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_DepthImage,
			m_AllocInfo);

		KVulkanInitializer::CreateVkImageView(m_DepthImage,
			m_DepthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT | (bStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0),
			1,
			m_DepthImageView);
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
	depthAttachment.format = m_DepthFormat;
	depthAttachment.samples = m_MsaaFlag;

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
		imageViews.push_back(m_DepthImageView);
		imageViews.push_back(m_ColorImageView);
	}
	else
	{
		imageViews.push_back(m_ColorImageView);
		imageViews.push_back(m_DepthImageView);
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


bool KVulkanRenderTarget::InitFromImage(void* imageHandle, void* imageFormatHandle, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	ASSERT_RESULT(CreateImage(imageHandle, imageFormatHandle, bDepth, bStencil, uMsaaCount));
	ASSERT_RESULT(CreateFramebuffer());	
	return true;
}

bool KVulkanRenderTarget::UnInit()
{
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	if(m_RenderPass)
	{
		vkDestroyRenderPass(KVulkanGlobal::device, m_RenderPass, nullptr);
		m_RenderPass = VK_NULL_HANDLE;
	}

	if(m_FrameBuffer)
	{
		vkDestroyFramebuffer(KVulkanGlobal::device, m_FrameBuffer, nullptr);
		m_FrameBuffer = VK_NULL_HANDLE;
	}

	if(m_ColorImageView)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_ColorImageView, nullptr);
		m_ColorImageView = VK_NULL_HANDLE;
	}

	if(m_bMsaaCreated)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_MsaaImageView, nullptr);
		KVulkanInitializer::FreeVkImage(m_MsaaImage, m_MsaaAlloc);

		m_MsaaImageView = VK_NULL_HANDLE;
		m_MsaaImage = VK_NULL_HANDLE;

		m_bMsaaCreated = false;
	}

	if(m_bDepthStencilCreated)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_DepthImageView, nullptr);
		KVulkanInitializer::FreeVkImage(m_DepthImage, m_AllocInfo);

		m_DepthImageView = VK_NULL_HANDLE;
		m_DepthImage = VK_NULL_HANDLE;

		m_bDepthStencilCreated = false;
	}

	return true;
}