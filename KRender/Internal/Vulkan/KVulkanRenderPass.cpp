#include "KVulkanRenderPass.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"

KVulkanRenderPass::KVulkanRenderPass()
	: m_RenderPass(VK_NULL_HANDLE),
	m_FrameBuffer(VK_NULL_HANDLE),
	m_ColorFrameBuffer(nullptr),
	m_DepthFrameBuffer(nullptr),
	m_ToSwapChain(false)
{
}

KVulkanRenderPass::~KVulkanRenderPass()
{
	ASSERT_RESULT(m_RenderPass == VK_NULL_HANDLE);
	ASSERT_RESULT(m_FrameBuffer == VK_NULL_HANDLE);
}

bool KVulkanRenderPass::SetColor(IKFrameBufferPtr color)
{
	m_ColorFrameBuffer = color;
	return true;
}

bool KVulkanRenderPass::SetDepthStencil(IKFrameBufferPtr depthStencil)
{
	m_DepthFrameBuffer = depthStencil;
	return true;
}

bool KVulkanRenderPass::SetAsSwapChainPass(bool swapChain)
{
	m_ToSwapChain = swapChain;
	return true;
}

bool KVulkanRenderPass::Init()
{
	UnInit();

	if (m_ColorFrameBuffer || m_DepthFrameBuffer)
	{
		if (m_ColorFrameBuffer && m_DepthFrameBuffer)
		{
			if (m_ColorFrameBuffer->GetWidth() != m_DepthFrameBuffer->GetWidth())
				return false;
			if (m_ColorFrameBuffer->GetHeight() != m_DepthFrameBuffer->GetHeight())
				return false;
			if (m_ColorFrameBuffer->GetMSAA() != m_DepthFrameBuffer->GetMSAA())
				return false;
		}

		KVulkanFrameBuffer* colorBuffer = m_ColorFrameBuffer ? (KVulkanFrameBuffer*)(m_ColorFrameBuffer.get()) : nullptr;
		KVulkanFrameBuffer* depthBuffer = m_DepthFrameBuffer ? (KVulkanFrameBuffer*)(m_DepthFrameBuffer.get()) : nullptr;

		if (m_ColorFrameBuffer)
		{
			VkImageLayout finalLayout = m_ToSwapChain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			bool massCreated = m_ColorFrameBuffer->GetMSAA() > 1;
			bool depthStencilCreated = m_DepthFrameBuffer != nullptr;

			// 声明 Color Attachment
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = colorBuffer->GetForamt();
			colorAttachment.samples = colorBuffer->GetMSAAFlag();

			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = massCreated ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : finalLayout;

			// 声明 Depth Attachment (不一定用到)
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = depthBuffer ? depthBuffer->GetForamt() : colorBuffer->GetForamt() /*让编译器开心*/;
			depthAttachment.samples = depthBuffer ? depthBuffer->GetMSAAFlag() : colorBuffer->GetMSAAFlag() /*其实两个一定是一样的*/;

			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			// 声明 MSAA Resolve Attachment (不一定用到)
			VkAttachmentDescription colorAttachmentResolve = {};
			colorAttachmentResolve.format = colorBuffer->GetForamt();
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
			colorAttachmentResolveRef.attachment = depthStencilCreated ? 2 : 1;
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
			if (depthStencilCreated)
			{
				descs.push_back(depthAttachment);
			}
			if (massCreated)
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
			ASSERT_RESULT(m_RenderPass != VK_NULL_HANDLE);

			std::vector<VkImageView> imageViews;

			if (massCreated)
			{
				imageViews.push_back(colorBuffer->GetMSAAImageView());
				if (depthStencilCreated)
				{
					imageViews.push_back(depthBuffer->GetImageView());
				}
				imageViews.push_back(colorBuffer->GetImageView());
			}
			else
			{
				imageViews.push_back(colorBuffer->GetImageView());
				if (depthStencilCreated)
				{
					imageViews.push_back(depthBuffer->GetImageView());
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = (uint32_t)imageViews.size();
			framebufferInfo.pAttachments = imageViews.data();
			framebufferInfo.width = colorBuffer->GetWidth();
			framebufferInfo.height = colorBuffer->GetHeight();
			framebufferInfo.layers = 1;

			VK_ASSERT_RESULT(vkCreateFramebuffer(KVulkanGlobal::device, &framebufferInfo, nullptr, &m_FrameBuffer));
		}
		else if (m_DepthFrameBuffer)
		{
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = depthBuffer->GetForamt();
			depthAttachment.samples = depthBuffer->GetMSAAFlag();

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

			VkImageView imageViews[] = { depthBuffer->GetImageView() };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = imageViews;
			framebufferInfo.width = depthBuffer->GetWidth();
			framebufferInfo.height = depthBuffer->GetHeight();
			framebufferInfo.layers = 1;

			VK_ASSERT_RESULT(vkCreateFramebuffer(KVulkanGlobal::device, &framebufferInfo, nullptr, &m_FrameBuffer));
		}

		return true;
	}
	return false;
}

bool KVulkanRenderPass::UnInit()
{
	if (m_RenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(KVulkanGlobal::device, m_RenderPass, nullptr);
		m_RenderPass = VK_NULL_HANDLE;
	}

	if (m_FrameBuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(KVulkanGlobal::device, m_FrameBuffer, nullptr);
		m_FrameBuffer = VK_NULL_HANDLE;
	}

	return true;
}