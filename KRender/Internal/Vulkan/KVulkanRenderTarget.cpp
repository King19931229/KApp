#include "KVulkanRenderTarget.h"
#include "KVulkanSwapChain.h"
#include "KVulkanTexture.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"

#include "Internal/KRenderGlobal.h"

IKRenderTargetPtr KVulkanRenderTarget::CreateRenderTarget()
{
	return IKRenderTargetPtr(new KVulkanRenderTarget());
}

KVulkanRenderTarget::KVulkanRenderTarget()
{
	m_MsaaFlag = VK_SAMPLE_COUNT_1_BIT;

	m_RenderPass = VK_NULL_HANDLE;
	m_FrameBuffer = VK_NULL_HANDLE;
	m_ColorFormat = VK_FORMAT_UNDEFINED;

	ZERO_MEMORY(m_Extend);
	m_ColorImageView = VK_NULL_HANDLE;
	m_ColorFormat = VK_FORMAT_UNDEFINED;

	m_MsaaImage = VK_NULL_HANDLE;
	m_MsaaImageView = VK_NULL_HANDLE;
	ZERO_MEMORY(m_MsaaAlloc);

	m_DepthFormat = VK_FORMAT_UNDEFINED;
	m_DepthImage = VK_NULL_HANDLE;
	m_DepthImageView = VK_NULL_HANDLE;
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

bool KVulkanRenderTarget::CreateImage(VkImageView imageView,
									  VkFormat imageForamt,
									  bool bDepth,
									  bool bStencil,
									  unsigned short uMsaaCount)
{
	ASSERT_RESULT(m_MsaaImage == VK_NULL_HANDLE);
	ASSERT_RESULT(m_DepthImage == VK_NULL_HANDLE);

	m_ColorImageView = imageView;
	m_ColorFormat = imageForamt;

	if(uMsaaCount > 1)
	{
		ASSERT_RESULT(KVulkanHelper::QueryMSAASupport(bDepth ? KVulkanHelper::MST_BOTH : KVulkanHelper::MST_COLOR,
			uMsaaCount , m_MsaaFlag));

		KVulkanInitializer::CreateVkImage(m_Extend.width, m_Extend.height, 1,
			1,1,
			m_MsaaFlag,
			VK_IMAGE_TYPE_2D, m_ColorFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			0,
			m_MsaaImage, m_MsaaAlloc);

		KVulkanInitializer::CreateVkImageView(m_MsaaImage, VK_IMAGE_VIEW_TYPE_2D,
			m_ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_MsaaImageView);

		KVulkanHelper::TransitionImageLayout(m_MsaaImage,
			m_ColorFormat, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	else
	{
		m_MsaaFlag = VK_SAMPLE_COUNT_1_BIT;
		m_MsaaImage = VK_NULL_HANDLE;
	}

	if(bDepth)
	{
		m_DepthFormat = FindDepthFormat(bStencil);

		KVulkanInitializer::CreateVkImage(m_Extend.width,
			m_Extend.height,
			1,
			1,1,
			m_MsaaFlag,
			VK_IMAGE_TYPE_2D,
			m_DepthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			0,
			m_DepthImage,
			m_DepthAlloc);

		KVulkanInitializer::CreateVkImageView(m_DepthImage,
			VK_IMAGE_VIEW_TYPE_2D,
			m_DepthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			1,
			m_DepthImageView);
	}

	return true;
}

bool KVulkanRenderTarget::CreateFramebuffer(bool fromSwapChain)
{	
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	VkImageLayout finalLayout = fromSwapChain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	bool massCreated = m_MsaaImage != VK_NULL_HANDLE;
	bool depthStencilCreated = m_DepthImage != VK_NULL_HANDLE;

	// 声明 Color Attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_ColorFormat;
	colorAttachment.samples = massCreated ? m_MsaaFlag : VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = massCreated ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : finalLayout;

	// 声明 Depth Attachment 不一定用到
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_DepthFormat;
	depthAttachment.samples = m_MsaaFlag;

	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	// 声明 MSAA Resolve Attachment 不一定用到
	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = m_ColorFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = finalLayout;

	// 声明Attachment引用结构
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = m_DepthImage != VK_NULL_HANDLE ? 2 : 1;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// 声明子通道
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = depthStencilCreated ? &depthAttachmentRef : nullptr;
	subpass.pResolveAttachments = massCreated ? &colorAttachmentResolveRef : nullptr;

	// 从属依赖
	VkSubpassDependency dependencies[2] = {};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// 创建渲染通道	
	std::vector<VkAttachmentDescription> descs;
	descs.push_back(colorAttachment);
	if(depthStencilCreated)
	{
		descs.push_back(depthAttachment);
	}
	if(massCreated)
	{
		descs.push_back(colorAttachmentResolve);
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = (uint32_t)descs.size();
	renderPassInfo.pAttachments = descs.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = ARRAY_SIZE(dependencies);
	renderPassInfo.pDependencies = dependencies;

	VK_ASSERT_RESULT(vkCreateRenderPass(KVulkanGlobal::device, &renderPassInfo, nullptr, &m_RenderPass));

	std::vector<VkImageView> imageViews;

	if(massCreated)
	{
		imageViews.push_back(m_MsaaImageView);
		if(depthStencilCreated)
		{
			imageViews.push_back(m_DepthImageView);
		}
		imageViews.push_back(m_ColorImageView);
	}
	else
	{
		imageViews.push_back(m_ColorImageView);
		if(depthStencilCreated)
		{
			imageViews.push_back(m_DepthImageView);
		}
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

bool KVulkanRenderTarget::InitFromSwapChain(IKSwapChain* swapChain, size_t imageIndex, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	KVulkanSwapChain* vulkanSwapChain = (KVulkanSwapChain*)swapChain;

	VkImageView imageView = vulkanSwapChain->GetImageView(imageIndex);
	VkFormat format = vulkanSwapChain->GetImageFormat();

	m_Extend.width = static_cast<uint32_t>(swapChain->GetWidth());
	m_Extend.height = static_cast<uint32_t>(swapChain->GetHeight());

	ASSERT_RESULT(imageView != VK_NULL_HANDLE);

	ASSERT_RESULT(CreateImage(imageView, format, bDepth, bStencil, uMsaaCount));
	ASSERT_RESULT(CreateFramebuffer(true));

	return true;
}

bool KVulkanRenderTarget::InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	KVulkanTexture* vulkanTexture = (KVulkanTexture*)texture;

	VkImageView imageView = vulkanTexture->GetImageView();
	VkFormat format = vulkanTexture->GetImageFormat();

	m_Extend.width = static_cast<uint32_t>(texture->GetWidth());
	m_Extend.height = static_cast<uint32_t>(texture->GetHeight());

	ASSERT_RESULT(imageView != VK_NULL_HANDLE);

	ASSERT_RESULT(CreateImage(imageView, format, bDepth, bStencil, uMsaaCount));
	ASSERT_RESULT(CreateFramebuffer(false));

	return true;
}

bool KVulkanRenderTarget::InitFromDepthStencil(size_t width, size_t height, bool bStencil)
{
	m_Extend.width = static_cast<uint32_t>(width);
	m_Extend.height = static_cast<uint32_t>(height);

	ASSERT_RESULT(CreateDepthImage(bStencil));
	ASSERT_RESULT(CreateDepthBuffer());	

	return true;
}

bool KVulkanRenderTarget::CreateDepthImage(bool bStencil)
{
	m_MsaaFlag = VK_SAMPLE_COUNT_1_BIT;
	m_MsaaImage = VK_NULL_HANDLE;

	m_DepthFormat = FindDepthFormat(bStencil);

	KVulkanInitializer::CreateVkImage(m_Extend.width,
		m_Extend.height,
		1,
		1,1,
		m_MsaaFlag,
		VK_IMAGE_TYPE_2D,
		m_DepthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		0,
		m_DepthImage,
		m_DepthAlloc);

	KVulkanInitializer::CreateVkImageView(m_DepthImage,
		VK_IMAGE_VIEW_TYPE_2D,
		m_DepthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1,
		m_DepthImageView);

	return true;
}

bool KVulkanRenderTarget::CreateDepthBuffer()
{
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_DepthFormat;
	depthAttachment.samples = m_MsaaFlag;

	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	// 声明Attachment引用结构
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// 声明子通道
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 0;
	subpass.pColorAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = nullptr;

	// 从属依赖
	VkSubpassDependency dependencies[2] = {};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// 创建渲染通道	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &depthAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = ARRAY_SIZE(dependencies);
	renderPassInfo.pDependencies = dependencies;

	VK_ASSERT_RESULT(vkCreateRenderPass(KVulkanGlobal::device, &renderPassInfo, nullptr, &m_RenderPass));

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_RenderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &m_DepthImageView;
	framebufferInfo.width = m_Extend.width;
	framebufferInfo.height = m_Extend.height;
	framebufferInfo.layers = 1;

	VK_ASSERT_RESULT(vkCreateFramebuffer(KVulkanGlobal::device, &framebufferInfo, nullptr, &m_FrameBuffer));

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

	m_ColorImageView = VK_NULL_HANDLE;

	if(m_MsaaImage != VK_NULL_HANDLE)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_MsaaImageView, nullptr);
		KVulkanInitializer::FreeVkImage(m_MsaaImage, m_MsaaAlloc);

		m_MsaaImageView = VK_NULL_HANDLE;
		m_MsaaImage = VK_NULL_HANDLE;
	}

	if(m_DepthImage != VK_NULL_HANDLE)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_DepthImageView, nullptr);
		KVulkanInitializer::FreeVkImage(m_DepthImage, m_DepthAlloc);

		m_DepthImageView = VK_NULL_HANDLE;
		m_DepthImage = VK_NULL_HANDLE;
	}

	KRenderGlobal::PipelineManager.InvaildateHandleByRt(shared_from_this());

	return true;
}

bool KVulkanRenderTarget::GetSize(size_t& width, size_t& height)
{
	width = m_Extend.width;
	height = m_Extend.height;
	return true;
}

bool KVulkanRenderTarget::HasColorAttachment()
{
	return m_ColorImageView != nullptr;
}

bool KVulkanRenderTarget::HasDepthStencilAttachment()
{
	return m_DepthImageView != nullptr;
}

bool KVulkanRenderTarget::GetImageViewInformation(RenderTargetComponent component, VkFormat& format, VkImageView& imageView)
{
	switch (component)
	{
	case RTC_COLOR:
		if(m_ColorImageView != VK_NULL_HANDLE)
		{
			format = m_ColorFormat;
			imageView = m_ColorImageView;
			return true;
		}
		else
		{
			return false;
		}
	case RTC_DEPTH_STENCIL:
		if(m_DepthImageView != VK_NULL_HANDLE)
		{
			if(m_MsaaImageView == VK_NULL_HANDLE)
			{
				format = m_DepthFormat;
				imageView = m_DepthImageView;
				return true;
			}
			else
			{
				// http://aicdg.com/ue4-msaa-depth/
				assert(false && "depth stencil msaa can't not be resolved in vulkan");
			}
		}
		return false;
	default:
		assert(false && "unknown component");
		return false;
	}
}