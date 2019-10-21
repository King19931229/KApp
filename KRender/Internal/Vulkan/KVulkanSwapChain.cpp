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

	// ��ʵ��ʱӦ�ö�format�������� ����ѵ�һ����Ϊ���ѡ��
	m_SurfaceFormat = m_SwapChainSupportDetails.formats[0];
	return true;
}

bool KVulkanSwapChain::ChooseSwapPresentMode()
{
	m_PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;

	const auto& presentModes = m_SwapChainSupportDetails.presentModes;
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		// �����ػ����ʹ�����ػ���
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			m_PresentMode = presentMode;
			break;
		}
		// ˫�ػ���
		if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
		{
			m_PresentMode = presentMode;
		}
	}

	// ��ʵVulkan��֤������˫�ػ������ʹ��
	if(m_PresentMode == VK_PRESENT_MODE_MAX_ENUM_KHR)
	{
		m_PresentMode = m_SwapChainSupportDetails.presentModes[0];
	}

	return m_PresentMode != VK_PRESENT_MODE_MAX_ENUM_KHR;
}

bool KVulkanSwapChain::ChooseSwapExtent(uint32_t windowWidth, uint32_t windowHeight)
{
	const VkSurfaceCapabilitiesKHR& capabilities = m_SwapChainSupportDetails.capabilities;
	// ���Vulkan������currentExtent ��ô��������extent�ͱ�����֮һ��
	if(capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
	{
		m_Extend = capabilities.currentExtent;
	}
	// �������ѡ���봰�ڴ�С�����ƥ��
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
	// ����Ϊ��Сֵ���ܱ���ȴ�������������ڲ��������ܻ�ȡ��һ��Ҫ��Ⱦ��ͼ�� �����+1����
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// Vulkan���maxImageCount����Ϊ0��ʾû�����ֵ���� ������һ����û�г������ֵ
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = m_SurfaceFormat.format;
	createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
	createInfo.imageExtent = m_Extend;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = {graphIndex, presentIndex};

	// ���ͼ����м�������ֶ��м��岻һ�� ��Ҫ����ģʽ֧��
	if (graphIndex != presentIndex)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	// �����ֶ�ռģʽ
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	// ���óɵ�ǰ����transform���ⷢ��������ת
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	// ���⵱ǰ������ϵͳ�������ڷ���alpha���
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = m_PresentMode;
	createInfo.clipped = VK_TRUE;

	// �����һ�δ��������� ����Ϊ�ռ���
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_ASSERT_RESULT(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));

	VK_ASSERT_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr));
	m_SwapChainImages.resize(imageCount);
	VK_ASSERT_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data()));

	// ����ImageView
	m_SwapChainImageViews.resize(imageCount);
	for(size_t i = 0; i < m_SwapChainImageViews.size(); ++i)
	{
		KVulkanInitializer::CreateVkImageView(m_SwapChainImages[i], m_SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_SwapChainImageViews[i]);
	}

	return true;
}

bool KVulkanSwapChain::CreateSyncObjects()
{
	// N����С�Ľ������趨N-1����ΪFramesInFight
	m_MaxFramesInFight = (size_t)std::max((int)m_SwapChainImages.size() - 1, 1);
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
		uint32_t windowHeight)
{
	ASSERT_RESULT(m_SwapChain == VK_NULL_HANDLE);

	m_Device = device;
	m_PhysicalDevice = physicalDevice;
	m_Surface = surface;

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

VkResult KVulkanSwapChain::WaitForInfightFrame()
{
	VkResult result = vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFlightIndex], VK_TRUE, UINT64_MAX);	
	return result;
}

VkResult KVulkanSwapChain::AcquireNextImage(uint32_t& imageIndex)
{
	// ��ȡ���ý�����Image���� ͬʱ�ٷ�Image�����ź���
	VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFlightIndex], VK_NULL_HANDLE, &imageIndex);
	return result;
}

VkResult KVulkanSwapChain::PresentQueue(VkQueue graphicsQueue, VkQueue presentQueue, uint32_t imageIndex, VkCommandBuffer commandBuffer)
{
	VkResult vkResult = VK_RESULT_MAX_ENUM;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// ��������COLOR_ATTACHMENT_OUTPUT����ʱ �ȴ�������Image�����ź���
	VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFlightIndex]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// ָ�������
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// ������ύ��ɺ󽫴ٷ����ź���
	VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFlightIndex]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// vkResetFences���õ�vkQueueSubmit֮ǰ ��֤����vkWaitForFences��������������
	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFlightIndex]);

	// �ύ�û�������
	vkResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFlightIndex]);
	VK_ASSERT_RESULT(vkResult);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	// ������Presentǰ�ȴ�������ύ���
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {m_SwapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	vkResult = vkQueuePresentKHR(presentQueue, &presentInfo);

#if 0
	/*
	��ʱ�����ǰ�̵߳ȴ�Presentִ�����.
	������ܳ�����һ�ε���Presentʱ��
	vkAcquireNextImageKHR��ȡ����imageIndex��Ӧ�Ľ�����Image���ύ��������.
	��ͳ�����CommandBuffer��Semaphore����.
	vkAcquireNextImageKHR��ȡ����imageIndexֻ��֤��Image��������Present
	�������ܱ�֤��Imageû��׼���ύ�Ļ�������
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
		imageView.imageViewHandle = m_SwapChainImageViews[imageIndex];
		imageView.imageForamt = m_SurfaceFormat.format;
		imageView.fromSwapChain = true;
		return true;
	}
	return false;
}