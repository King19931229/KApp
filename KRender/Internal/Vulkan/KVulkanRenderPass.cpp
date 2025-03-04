#include "KVulkanRenderPass.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KBase/Publish/KHash.h"

KVulkanRenderPass::KVulkanRenderPass()
	: m_RenderPass(VK_NULL_HANDLE),
	m_FrameBuffer(VK_NULL_HANDLE),
	m_DepthFrameBuffer(nullptr),
	m_MSAAFlag(VK_SAMPLE_COUNT_1_BIT),
	m_ToSwapChain(false),
	m_AttachmentHash(0)
{
	for (uint32_t attachment = 0; attachment < MAX_ATTACHMENT; ++attachment)
	{
		m_ColorFrameBuffers[attachment] = nullptr;
	}
}

KVulkanRenderPass::~KVulkanRenderPass()
{
	ASSERT_RESULT(m_RenderPass == VK_NULL_HANDLE);
	ASSERT_RESULT(m_FrameBuffer == VK_NULL_HANDLE);
}

bool KVulkanRenderPass::SetColorAttachment(uint32_t attachment, IKFrameBufferPtr color)
{
	if (attachment < MAX_ATTACHMENT)
	{
		m_ColorFrameBuffers[attachment] = color;
		return true;
	}
	return false;
}

bool KVulkanRenderPass::SetDepthStencilAttachment(IKFrameBufferPtr depthStencil)
{
	m_DepthFrameBuffer = depthStencil;
	return true;
}

bool KVulkanRenderPass::SetClearColor(uint32_t attachment, const KClearColor& clearColor)
{
	if (attachment < MAX_ATTACHMENT)
	{
		m_ClearColors[attachment] = clearColor;
		return true;
	}
	return false;
}

bool KVulkanRenderPass::SetClearDepthStencil(const KClearDepthStencil& clearDepthStencil)
{
	m_ClearDepthStencil = clearDepthStencil;
	return true;
}

bool KVulkanRenderPass::SetOpColor(uint32_t attachment, LoadOperation loadOp, StoreOperation storeOp)
{
	if (attachment < MAX_ATTACHMENT)
	{
		m_OpColors[attachment] = KRenderPassOperation(loadOp , storeOp);
		return true;
	}
	return false;
}

bool KVulkanRenderPass::SetOpDepthStencil(LoadOperation depthLoadOp, StoreOperation depthStoreOp, LoadOperation stencilLoadOp, StoreOperation stencilStoreOp)
{
	m_OpDepth = KRenderPassOperation(depthLoadOp, depthStoreOp);
	m_OpStencil = KRenderPassOperation(stencilLoadOp, stencilStoreOp);
	return true;
}

bool KVulkanRenderPass::SetAsSwapChainPass(bool swapChain)
{
	m_ToSwapChain = swapChain;
	return true;
}

bool KVulkanRenderPass::HasColorAttachment()
{
	for (uint32_t attachment = 0; attachment < MAX_ATTACHMENT; ++attachment)
	{
		if (m_ColorFrameBuffers[attachment] != nullptr)
		{
			return true;
		}
	}
	return false;
}

bool KVulkanRenderPass::HasDepthStencilAttachment()
{
	return m_DepthFrameBuffer != nullptr;
}

uint32_t KVulkanRenderPass::GetColorAttachmentCount()
{
	uint32_t count = 0;
	for (uint32_t attachment = 0; attachment < MAX_ATTACHMENT; ++attachment)
	{
		if (m_ColorFrameBuffers[attachment] != nullptr)
		{
			++count;
		}
	}
	return count;
}

const KViewPortArea& KVulkanRenderPass::GetViewPort()
{
	return m_ViewPortArea;
}

bool KVulkanRenderPass::RegisterInvalidCallback(RenderPassInvalidCallback* callback)
{
	if (callback)
	{
		auto it = std::find(m_InvalidCallbacks.begin(), m_InvalidCallbacks.end(), callback);
		if (it == m_InvalidCallbacks.end())
		{
			m_InvalidCallbacks.push_back(callback);
		}
		return true;
	}
	return false;
}

bool KVulkanRenderPass::UnRegisterInvalidCallback(RenderPassInvalidCallback* callback)
{
	if (callback)
	{
		auto it = std::find(m_InvalidCallbacks.begin(), m_InvalidCallbacks.end(), callback);
		if (it != m_InvalidCallbacks.end())
		{
			m_InvalidCallbacks.erase(it);
		}
		return true;
	}
	return false;
}



size_t KVulkanRenderPass::CalcAttachmentHash(uint32_t mipmap)
{
	using namespace KHash;

	size_t hash = 0;

	for (uint32_t attachment = 0; attachment < MAX_ATTACHMENT; ++attachment)
	{
		if (m_ColorFrameBuffers[attachment] != nullptr)
		{
			KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)m_ColorFrameBuffers[attachment].get();

			HashCombine(hash, HashCompute(vulkanFrameBuffer->GetWidth()));
			HashCombine(hash, HashCompute(vulkanFrameBuffer->GetHeight()));

			HashCombine(hash, HashCompute(vulkanFrameBuffer->GetImage()));
			HashCombine(hash, HashCompute(vulkanFrameBuffer->GetMipmapImageView(mipmap, 1)));
			HashCombine(hash, HashCompute(vulkanFrameBuffer->GetMSAAImage()));
			HashCombine(hash, HashCompute(vulkanFrameBuffer->GetMSAAImageView()));

			HashCombine(hash, HashCompute(m_OpColors[attachment]));
		}
		else
		{
			hash ^= ((size_t)1 << attachment);
		}
	}

	if (m_DepthFrameBuffer)
	{
		KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)m_DepthFrameBuffer.get();

		HashCombine(hash, HashCompute(vulkanFrameBuffer->GetWidth()));
		HashCombine(hash, HashCompute(vulkanFrameBuffer->GetHeight()));

		HashCombine(hash, HashCompute(vulkanFrameBuffer->GetImage()));
		HashCombine(hash, HashCompute(vulkanFrameBuffer->GetImageView()));
		HashCombine(hash, HashCompute(vulkanFrameBuffer->GetMSAAImage()));
		HashCombine(hash, HashCompute(vulkanFrameBuffer->GetMSAAImageView()));

		HashCombine(hash, HashCompute(m_OpDepth));
		HashCombine(hash, HashCompute(m_OpStencil));
	}

	if (m_ToSwapChain)
	{
		HashCombine(hash, ((size_t)1 << (MAX_ATTACHMENT + 1)));
	}

	return hash;
}

bool KVulkanRenderPass::Init(uint32_t mipmap)
{
	ASSERT_RESULT(mipmap == 0 || (!m_DepthFrameBuffer && m_MSAAFlag == VK_SAMPLE_COUNT_1_BIT));

	size_t currentHash = CalcAttachmentHash(mipmap);
	if (currentHash != m_AttachmentHash)
	{
		UnInit();

		if (HasColorAttachment() || HasDepthStencilAttachment())
		{
			IKFrameBufferPtr compareRef = nullptr;

			if (m_DepthFrameBuffer)
			{
				compareRef = m_DepthFrameBuffer;
			}

			if (!compareRef)
			{
				for (uint32_t attachment = 0; attachment < MAX_ATTACHMENT; ++attachment)
				{
					if (m_ColorFrameBuffers[attachment])
					{
						compareRef = m_ColorFrameBuffers[attachment];
						break;
					}
				}
			}

#define ASSERT_AND_RETURN(expr)	if(!(expr)) { ASSERT_RESULT(false); return false;}		

			for (uint32_t attachment = 0; attachment < MAX_ATTACHMENT; ++attachment)
			{
				if (m_ColorFrameBuffers[attachment])
				{
					ASSERT_AND_RETURN(m_ColorFrameBuffers[attachment]->GetWidth() == compareRef->GetWidth());
					ASSERT_AND_RETURN(m_ColorFrameBuffers[attachment]->GetHeight() == compareRef->GetHeight());
					ASSERT_AND_RETURN(m_ColorFrameBuffers[attachment]->GetMSAA() == compareRef->GetMSAA());
				}
			}

			if (m_DepthFrameBuffer)
			{
				ASSERT_AND_RETURN(m_DepthFrameBuffer->GetWidth() == compareRef->GetWidth());
				ASSERT_AND_RETURN(m_DepthFrameBuffer->GetHeight() == compareRef->GetHeight());
				ASSERT_AND_RETURN(m_DepthFrameBuffer->GetMSAA() == compareRef->GetMSAA());
			}

			m_ViewPortArea.x = 0;
			m_ViewPortArea.y = 0;
			m_ViewPortArea.width = std::max(1U, compareRef->GetWidth() >> mipmap);
			m_ViewPortArea.height = std::max(1U, compareRef->GetHeight() >> mipmap);
			m_MSAAFlag = ((KVulkanFrameBuffer*)compareRef.get())->GetMSAAFlag();

#undef ASSERT_AND_RETURN

			VkImageLayout color_0_finalLayout = (m_ToSwapChain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			VkImageLayout color_x_finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			bool massCreated = compareRef->GetMSAA() > 1;
			bool depthStencilCreated = m_DepthFrameBuffer != nullptr;

			std::vector<VkAttachmentDescription> descs;
			std::vector<VkImageView> imageViews;

			uint32_t maxAttachment = 0;
			uint32_t maxColorAttachment = 0;

			uint32_t colorRefCounts = 0;
			std::array<VkAttachmentReference, MAX_ATTACHMENT> colorRefs;
			for (uint32_t i = 0; i < MAX_ATTACHMENT; ++i)
			{
				// 声明 Color Attachment
				if (m_ColorFrameBuffers[i])
				{
					KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)m_ColorFrameBuffers[i].get();

					VkAttachmentDescription colorAttachment = {};
					colorAttachment.format = vulkanFrameBuffer->GetForamt();
					colorAttachment.samples = vulkanFrameBuffer->GetMSAAFlag();

					KVulkanHelper::LoadOpToVkAttachmentLoadOp(m_OpColors[i].loadOp, colorAttachment.loadOp);
					KVulkanHelper::StoreOpToVkAttachmentStoreOp(m_OpColors[i].storeOp, colorAttachment.storeOp);

					colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

					if (m_OpColors[i].loadOp == LO_LOAD)
					{
						colorAttachment.initialLayout = massCreated ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : vulkanFrameBuffer->GetImageLayout();
					}
					else
					{
						colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					}
					colorAttachment.finalLayout = massCreated ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : (i == 0 ? color_0_finalLayout : color_x_finalLayout);

					descs.push_back(colorAttachment);
					imageViews.push_back(vulkanFrameBuffer->GetMipmapImageView(mipmap, 1));

					VkAttachmentReference colorAttachmentRef = {};
					colorAttachmentRef.attachment = i;
					colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					colorRefs[colorRefCounts++] = colorAttachmentRef;
					maxColorAttachment = i;

					maxAttachment = maxColorAttachment;
				}
			}

			VkAttachmentReference depthAttachmentRef = {};
			// 先判断有没有color attachment
			depthAttachmentRef.attachment = colorRefCounts ? maxColorAttachment + 1 : 0;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			if (m_DepthFrameBuffer)
			{
				KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)m_DepthFrameBuffer.get();

				// 声明 Depth Attachment
				VkAttachmentDescription depthAttachment = {};
				depthAttachment.format = vulkanFrameBuffer->GetForamt();
				depthAttachment.samples = vulkanFrameBuffer->GetMSAAFlag();

				KVulkanHelper::LoadOpToVkAttachmentLoadOp(m_OpDepth.loadOp, depthAttachment.loadOp);
				KVulkanHelper::StoreOpToVkAttachmentStoreOp(m_OpDepth.storeOp, depthAttachment.storeOp);

				KVulkanHelper::LoadOpToVkAttachmentLoadOp(m_OpStencil.loadOp, depthAttachment.stencilLoadOp);
				KVulkanHelper::StoreOpToVkAttachmentStoreOp(m_OpStencil.storeOp, depthAttachment.stencilStoreOp);

				if (m_OpDepth.loadOp == LO_LOAD || m_OpStencil.loadOp == LO_LOAD)
				{
					depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else
				{
					depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				}
				depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

				descs.push_back(depthAttachment);
				imageViews.push_back(vulkanFrameBuffer->GetImageView());

				maxAttachment = depthAttachmentRef.attachment;
			}

			uint32_t colorResolveAttachmentBegin = maxAttachment + 1;
			uint32_t colorResolveRefCounts = 0;
			std::array<VkAttachmentReference, MAX_ATTACHMENT> colorResolveRefs;
			if (massCreated)
			{
				for (uint32_t i = 0; i < MAX_ATTACHMENT; ++i)
				{
					// 声明 MSAA Resolve Attachment
					if (m_ColorFrameBuffers[i])
					{
						KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)m_ColorFrameBuffers[i].get();

						VkAttachmentDescription colorAttachmentResolve = {};
						colorAttachmentResolve.format = vulkanFrameBuffer->GetForamt();
						colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;

						// KVulkanHelper::LoadOpToVkAttachmentLoadOp(m_OpColors[i].loadOp, colorAttachmentResolve.loadOp);
						// KVulkanHelper::StoreOpToVkAttachmentStoreOp(m_OpColors[i].storeOp, colorAttachmentResolve.storeOp);

						colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

						colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

						colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						colorAttachmentResolve.finalLayout = (i == 0 ? color_0_finalLayout : color_x_finalLayout);

						descs.push_back(colorAttachmentResolve);
						imageViews.push_back(vulkanFrameBuffer->GetMSAAImageView());

						VkAttachmentReference colorAttachmentResolveRef = {};
						colorAttachmentResolveRef.attachment = colorResolveAttachmentBegin + i;
						colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						colorResolveRefs[colorResolveRefCounts++] = colorAttachmentResolveRef;

						maxAttachment = colorAttachmentResolveRef.attachment;
					}
				}
			}

			// 声明subpass
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

			subpass.colorAttachmentCount = colorRefCounts;
			subpass.pColorAttachments = &colorRefs[0];
			subpass.pDepthStencilAttachment = depthStencilCreated ? &depthAttachmentRef : nullptr;
			subpass.pResolveAttachments = massCreated ? &colorResolveRefs[0] : nullptr;

			// subpass依赖
			VkSubpassDependency dependencies[1] = {};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			if (HasColorAttachment())
			{
				dependencies[0].srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[0].srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependencies[0].dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			}

			if (HasDepthStencilAttachment())
			{
				dependencies[0].srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				dependencies[0].srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dependencies[0].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
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

			// 这里不要忘了颠倒过来
			if (massCreated)
			{
				for (uint32_t i = 0; i < colorRefCounts; ++i)
				{
					std::swap(imageViews[i], imageViews[i + colorRefCounts + (m_DepthFrameBuffer ? 1 : 0)]);
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = (uint32_t)imageViews.size();
			framebufferInfo.pAttachments = imageViews.data();
			framebufferInfo.width = std::max(1U, compareRef->GetWidth() >> mipmap);
			framebufferInfo.height = std::max(1U, compareRef->GetHeight() >> mipmap);
			framebufferInfo.layers = 1;

			VK_ASSERT_RESULT(vkCreateFramebuffer(KVulkanGlobal::device, &framebufferInfo, nullptr, &m_FrameBuffer));
			ASSERT_RESULT(m_FrameBuffer != VK_NULL_HANDLE);

			if (!m_Name.empty())
			{
				SetDebugName(m_Name.c_str());
			}

			m_AttachmentHash = currentHash;
			return true;
		}
		return false;
	}
	return true;
}

bool KVulkanRenderPass::UnInit()
{
	for (RenderPassInvalidCallback* callback : m_InvalidCallbacks)
	{
		(*callback)(this);
	}
	m_InvalidCallbacks.clear();

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

	m_AttachmentHash = 0;

	return true;
}

bool KVulkanRenderPass::SetDebugName(const char* name)
{
	m_Name = name;
	if (m_RenderPass != VK_NULL_HANDEL)
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_RenderPass, VK_OBJECT_TYPE_RENDER_PASS, name);
	}
	if (m_FrameBuffer != VK_NULL_HANDEL)
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_FrameBuffer, VK_OBJECT_TYPE_FRAMEBUFFER, (m_Name + "FrameBuffer").c_str());
	}
	return true;
}

const char* KVulkanRenderPass::GetDebugName() const
{
	return m_Name.size() ? m_Name.c_str() : nullptr;
}

bool KVulkanRenderPass::GetVkClearValues(VkClearValueArray& clearValues)
{
	clearValues.clear();
	clearValues.reserve(MAX_ATTACHMENT + 1);

	for (uint32_t i = 0; i < MAX_ATTACHMENT; ++i)
	{
		if (m_ColorFrameBuffers[i])
		{
			VkClearValue vkClearValue;
			vkClearValue.color = { m_ClearColors[i].r, m_ClearColors[i].g, m_ClearColors[i].b, m_ClearColors[i].a };
			clearValues.push_back(vkClearValue);
		}
	}

	if (m_DepthFrameBuffer)
	{
		VkClearValue vkClearValue;
		vkClearValue.depthStencil = { m_ClearDepthStencil.depth, (uint32_t)m_ClearDepthStencil.stencil };
		clearValues.push_back(vkClearValue);
	}

	return true;
}