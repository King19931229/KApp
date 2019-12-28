#include "KVulkanSwapChain.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KBase/Publish/KConfig.h"

KVulkanSwapChain::KVulkanSwapChain()
	: m_MaxFramesInFight(0),
	m_CurrentFlightIndex(0)
{
	m_Device = VK_NULL_HANDLE;
	m_PhysicalDevice = VK_NULL_HANDLE;
	m_SwapChain = VK_NULL_HANDLE;

	ZERO_MEMORY(m_Extend);
	ZERO_MEMORY(m_Surface);
	ZERO_MEMORY(m_SurfaceFormat);

	m_PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
}

KVulkanSwapChain::~KVulkanSwapChain()
{
	ASSERT_RESULT(m_SwapChain == VK_NULL_HANDLE);
	ASSERT_RESULT(m_SwapChainImages.empty());
	ASSERT_RESULT(m_ImageAvailableSemaphores.empty());
	ASSERT_RESULT(m_RenderFinishedSemaphores.empty());
	ASSERT_RESULT(m_InFlightFences.empty());
}

bool KVulkanSwapChain::QuerySwapChainSupport()
{
	SwapChainSupportDetails details = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &details.capabilities);

	uint32_t formatCount = 0;
	VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr));

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, details.formats.data()));
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, details.presentModes.data()));
	}

	m_SwapChainSupportDetails = std::move(details);

	return true;
}

 bool KVulkanSwapChain::ChooseSwapSurfaceFormat()
{
	const auto& formats = m_SwapChainSupportDetails.formats;
	for(const VkSurfaceFormatKHR& surfaceFormat : formats)
	{
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			m_SurfaceFormat = surfaceFormat;
			return true;
		}
	}

	// 其实这时应该对format进行排序 这里把第一个最为最佳选择
	m_SurfaceFormat = m_SwapChainSupportDetails.formats[0];
	return true;
}

bool KVulkanSwapChain::ChooseSwapPresentMode()
{
	m_PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;

	const auto& presentModes = m_SwapChainSupportDetails.presentModes;
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		// 有三重缓冲就使用三重缓冲
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			m_PresentMode = presentMode;
			break;
		}
		// 双重缓冲
		if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
		{
			m_PresentMode = presentMode;
		}
	}

	// 其实Vulkan保证至少有双重缓冲可以使用
	if(m_PresentMode == VK_PRESENT_MODE_MAX_ENUM_KHR)
	{
		m_PresentMode = m_SwapChainSupportDetails.presentModes[0];
	}

	return m_PresentMode != VK_PRESENT_MODE_MAX_ENUM_KHR;
}

bool KVulkanSwapChain::ChooseSwapExtent(uint32_t windowWidth, uint32_t windowHeight)
{
	const VkSurfaceCapabilitiesKHR& capabilities = m_SwapChainSupportDetails.capabilities;
	// 如果Vulkan设置了currentExtent 那么交换链的extent就必须与之一致
	if(capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
	{
		m_Extend = capabilities.currentExtent;
	}
	// 这里可以选择与窗口大小的最佳匹配
	else
	{
		VkExtent2D actualExtent = { windowWidth, windowHeight };
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		m_Extend = std::move(actualExtent);
	}
	return true;
}

bool KVulkanSwapChain::CreateSwapChain(uint32_t windowWidth, uint32_t windowHeight,
	uint32_t graphIndex, uint32_t presentIndex)
{
	ASSERT_RESULT(ChooseSwapSurfaceFormat());
	ASSERT_RESULT(ChooseSwapPresentMode());
	ASSERT_RESULT(ChooseSwapExtent(windowWidth, windowHeight));

	const SwapChainSupportDetails& swapChainSupport = m_SwapChainSupportDetails;
	// 设置为最小值可能必须等待驱动程序完成内部操作才能获取另一个要渲染的图像 因此作+1处理
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// Vulkan会把maxImageCount设置为0表示没有最大值限制 这里检查一下有没有超过最大值
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	// 询问交换链支持格式
	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities));

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = m_SurfaceFormat.format;
	createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
	createInfo.imageExtent = m_Extend;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	ASSERT_RESULT(createInfo.imageUsage & surfaceCapabilities.supportedUsageFlags && "imageUsage must be supported");

	uint32_t queueFamilyIndices[] = {graphIndex, presentIndex};

	// 如果图像队列家族与表现队列家族不一样 需要并行模式支持
	if (graphIndex != presentIndex)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	// 否则坚持独占模式
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	ASSERT_RESULT(createInfo.preTransform & surfaceCapabilities.supportedTransforms && "preTransform must be supported");

	const VkCompositeAlphaFlagBitsKHR compositeCandidata[] =
	{
		// 避免当前窗口与系统其它窗口发生alpha混合
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};

	for(uint32_t i = 0; i < ARRAY_SIZE(compositeCandidata); ++i)
	{
		if(compositeCandidata[i] & surfaceCapabilities.supportedCompositeAlpha)
		{
			createInfo.compositeAlpha = compositeCandidata[i];
			break;
		}
	}
	ASSERT_RESULT(createInfo.compositeAlpha != 0 && "compositeAlpha must be selected");

	createInfo.presentMode = m_PresentMode;
	createInfo.clipped = VK_TRUE;

	// 这里第一次创建交换链 设置为空即可
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_ASSERT_RESULT(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));

	VK_ASSERT_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr));
	m_SwapChainImages.resize(imageCount);
	VK_ASSERT_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data()));

	// 创建ImageView
	m_SwapChainImageViews.resize(imageCount);
	for(size_t i = 0; i < m_SwapChainImageViews.size(); ++i)
	{
		KVulkanInitializer::CreateVkImageView(m_SwapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, m_SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_SwapChainImageViews[i]);
	}

	return true;
}

bool KVulkanSwapChain::CreateSyncObjects()
{
	//InFlightFrame数目不超过Image数目
	//m_MaxFramesInFight = std::min(m_SwapChainImages.size(), m_MaxFramesInFight);

	m_CurrentFlightIndex = 0;

	m_ImageAvailableSemaphores.resize(m_MaxFramesInFight);
	m_RenderFinishedSemaphores.resize(m_MaxFramesInFight);
	m_InFlightFences.resize(m_MaxFramesInFight);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for(size_t i = 0; i < m_MaxFramesInFight; ++i)
	{
		VK_ASSERT_RESULT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
		VK_ASSERT_RESULT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
		VK_ASSERT_RESULT(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]));
	}
	return true;
}

bool KVulkanSwapChain::CleanupSwapChain()
{
	for(VkImageView vkImageView : m_SwapChainImageViews)
	{
		vkDestroyImageView(KVulkanGlobal::device, vkImageView, nullptr);
	}
	m_SwapChainImageViews.clear();
	m_SwapChainImages.clear();
	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	m_SwapChain = VK_NULL_HANDLE;
	return true;
}

bool KVulkanSwapChain::DestroySyncObjects()
{
	for(size_t i = 0; i < m_ImageAvailableSemaphores.size(); ++i)
	{
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
	}
	m_ImageAvailableSemaphores.clear();

	for(size_t i = 0; i < m_RenderFinishedSemaphores.size(); ++i)
	{
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
	}
	m_RenderFinishedSemaphores.clear();

	for(size_t i = 0; i < m_InFlightFences.size(); ++i)
	{
		vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
	}
	m_InFlightFences.clear();

	return true;
}

bool KVulkanSwapChain::Init(VkDevice device,
		VkPhysicalDevice physicalDevice,
		uint32_t graphIndex,
		uint32_t presentIndex,
		VkSurfaceKHR surface,
		uint32_t windowWidth,
		uint32_t windowHeight,
		size_t frameInFlight)
{
	ASSERT_RESULT(m_SwapChain == VK_NULL_HANDLE);

	m_Device = device;
	m_PhysicalDevice = physicalDevice;
	m_Surface = surface;
	m_MaxFramesInFight = frameInFlight;

	ASSERT_RESULT(QuerySwapChainSupport());
	ASSERT_RESULT(CreateSwapChain(windowWidth, windowHeight, graphIndex, presentIndex));
	ASSERT_RESULT(CreateSyncObjects());

	return true;
}

bool KVulkanSwapChain::UnInit()
{
	ASSERT_RESULT(m_SwapChain != VK_NULL_HANDLE);
	ASSERT_RESULT(CleanupSwapChain());
	ASSERT_RESULT(DestroySyncObjects());

	return true;
}

VkResult KVulkanSwapChain::WaitForInfightFrame(size_t& frameIndex)
{
	VkResult result = vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFlightIndex], VK_TRUE, UINT64_MAX);
	frameIndex = m_CurrentFlightIndex;
	return result;
}

VkResult KVulkanSwapChain::AcquireNextImage(uint32_t& imageIndex)
{
	// 获取可用交换链Image索引 同时促发Image可用信号量
	VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFlightIndex], VK_NULL_HANDLE, &imageIndex);
	return result;
}

VkResult KVulkanSwapChain::PresentQueue(VkQueue graphicsQueue, VkQueue presentQueue, uint32_t imageIndex, VkCommandBuffer commandBuffer)
{
	VkResult vkResult = VK_RESULT_MAX_ENUM;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// 命令缓冲管线COLOR_ATTACHMENT_OUTPUT可用时 等待交换链Image可用信号量
	VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFlightIndex]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// 指定命令缓冲
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// 命令缓冲提交完成后将促发此信号量
	VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFlightIndex]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// vkResetFences放置到vkQueueSubmit之前 保证调用vkWaitForFences都不会无限死锁
	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFlightIndex]);

	// 提交该绘制命令
	vkResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFlightIndex]);
	VK_ASSERT_RESULT(vkResult);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	// 交换链Present前等待命令缓冲提交完成
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {m_SwapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	vkResult = vkQueuePresentKHR(presentQueue, &presentInfo);

#if 0
	/*
	这时候挂起当前线程等待Present执行完毕.
	否则可能出现下一次调用Present时候
	vkAcquireNextImageKHR获取到的imageIndex对应的交换链Image正提交绘制命令.
	这就出现了CommandBuffer与Semaphore重用.
	vkAcquireNextImageKHR获取到的imageIndex只保证该Image不是正在Present
	但并不能保证该Image没有准备提交的绘制命令
	*/
	vkQueueWaitIdle(m_PresentQueue);
#endif	

	m_CurrentFlightIndex = (m_CurrentFlightIndex + 1) %  m_MaxFramesInFight;

	return vkResult;
}

bool KVulkanSwapChain::GetImageView(size_t imageIndex, ImageView& imageView)
{
	assert(imageIndex < m_SwapChainImageViews.size());
	if(imageIndex < m_SwapChainImageViews.size())
	{
		imageView.imageViewHandle = (void*)m_SwapChainImageViews[imageIndex];
		imageView.imageForamt = m_SurfaceFormat.format;
		imageView.fromSwapChain = true;
		imageView.fromDepthStencil = false;
		return true;
	}
	return false;
}