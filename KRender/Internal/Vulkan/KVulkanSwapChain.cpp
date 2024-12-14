#include "KVulkanSwapChain.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanRenderPass.h"
#include "KVulkanQueue.h"
#include "Internal/KRenderGlobal.h"
#include "Interface/IKRenderWindow.h"
#include "KBase/Publish/KConfig.h"
#include <algorithm>

KVulkanSwapChain::KVulkanSwapChain()
	: m_MaxFramesInFight(0)
	, m_CurrentFlightIndex(0)
	, m_CurrentImageIndex(0)
{
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
}

bool KVulkanSwapChain::QuerySwapChainSupport()
{
	SwapChainSupportDetails details = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(KVulkanGlobal::physicalDevice, m_Surface, &details.capabilities);

	uint32_t formatCount = 0;
	VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(KVulkanGlobal::physicalDevice, m_Surface, &formatCount, nullptr));

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(KVulkanGlobal::physicalDevice, m_Surface, &formatCount, details.formats.data()));
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(KVulkanGlobal::physicalDevice, m_Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(KVulkanGlobal::physicalDevice, m_Surface, &presentModeCount, details.presentModes.data()));
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

bool KVulkanSwapChain::CreateSwapChain()
{
	ASSERT_RESULT(QuerySwapChainSupport());
	ASSERT_RESULT(ChooseQueue());

	uint32_t windowWidth = 0;
	uint32_t windowHeight = 0;

	{
		size_t width = 0, height = 0;
		m_pWindow->GetSize(width, height);
		windowWidth = (uint32_t)width;
		windowHeight = (uint32_t)height;
	}

	uint32_t graphIndex = KVulkanGlobal::graphicsFamilyIndices[0];
	uint32_t presentIndex = graphIndex;

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
	VK_ASSERT_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(KVulkanGlobal::physicalDevice, m_Surface, &surfaceCapabilities));

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

	VK_ASSERT_RESULT(vkCreateSwapchainKHR(KVulkanGlobal::device, &createInfo, nullptr, &m_SwapChain));

	KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_SwapChain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "SwapChain");

	VK_ASSERT_RESULT(vkGetSwapchainImagesKHR(KVulkanGlobal::device, m_SwapChain, &imageCount, nullptr));
	m_SwapChainImages.resize(imageCount);
	VK_ASSERT_RESULT(vkGetSwapchainImagesKHR(KVulkanGlobal::device, m_SwapChain, &imageCount, m_SwapChainImages.data()));

	// 创建ImageView
	m_SwapChainImageViews.resize(imageCount);
	for(size_t i = 0; i < m_SwapChainImageViews.size(); ++i)
	{
		KVulkanInitializer::TransitionImageLayout(m_SwapChainImages[i], m_SurfaceFormat.format, 0, 1, 0, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		KVulkanInitializer::CreateVkImageView(m_SwapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, m_SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, m_SwapChainImageViews[i]);

		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_SwapChainImages[i], VK_OBJECT_TYPE_IMAGE, (std::string("SwapChainImage_") + std::to_string(i)).c_str());
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_SwapChainImageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW, (std::string("SwapChainImageView_") + std::to_string(i)).c_str());
	}

	return true;
}

bool KVulkanSwapChain::CreateSyncObjects()
{
	m_CurrentFlightIndex = 0;

	KRenderGlobal::RenderDevice->CreateSemaphore(m_ImageAvailableSemaphore);
	KRenderGlobal::RenderDevice->CreateSemaphore(m_RenderFinishedSemaphore);
	KRenderGlobal::RenderDevice->CreateFence(m_InFlightFence);

	m_ImageAvailableSemaphore->Init();
	m_ImageAvailableSemaphore->SetDebugName("ImageAvailableSemaphore");
	m_RenderFinishedSemaphore->Init();
	m_RenderFinishedSemaphore->SetDebugName("RenderFinishedSemaphore");
	m_InFlightFence->Init(true);
	m_InFlightFence->SetDebugName("InFlightFence");

	return true;
}

bool KVulkanSwapChain::CreateFrameBuffers()
{
	m_FrameBuffers.resize(m_SwapChainImages.size());
	m_RenderPasses.resize(m_SwapChainImages.size());

	for (size_t i = 0; i < m_FrameBuffers.size(); ++i)
	{
		FrameBuffer& frameBuffer = m_FrameBuffers[i];

		frameBuffer.colorFrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
		((KVulkanFrameBuffer*)frameBuffer.colorFrameBuffer.get())->InitExternal(KVulkanFrameBuffer::ET_SWAPCHAIN, m_SwapChainImages[i], m_SwapChainImageViews[i],
			VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D,
			m_SurfaceFormat.format, m_Extend.width, m_Extend.height, 1, 1, 1);

		frameBuffer.depthStencilFrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
		((KVulkanFrameBuffer*)frameBuffer.depthStencilFrameBuffer.get())->InitDepthStencil(m_Extend.width, m_Extend.height, 1, true);
		
		IKRenderPassPtr& renderPass = m_RenderPasses[i];

		renderPass = IKRenderPassPtr(KNEW KVulkanRenderPass());
		renderPass->SetAsSwapChainPass(true);
		renderPass->SetColorAttachment(0, frameBuffer.colorFrameBuffer);
		renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		renderPass->SetDepthStencilAttachment(frameBuffer.depthStencilFrameBuffer);
		renderPass->SetClearDepthStencil({ 1.0f, 0 });
		renderPass->Init();
	}

	return true;
}

bool KVulkanSwapChain::CleanupSwapChain()
{
	if (m_SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(KVulkanGlobal::device, m_SwapChain, nullptr);
		m_SwapChain = VK_NULL_HANDLE;
	}
	if (m_Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(KVulkanGlobal::instance, m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
	}
	for(VkImageView vkImageView : m_SwapChainImageViews)
	{
		vkDestroyImageView(KVulkanGlobal::device, vkImageView, nullptr);
	}
	m_SwapChainImageViews.clear();
	m_SwapChainImages.clear();
	return true;
}

bool KVulkanSwapChain::DestroySyncObjects()
{
	SAFE_UNINIT(m_ImageAvailableSemaphore);
	SAFE_UNINIT(m_RenderFinishedSemaphore);
	SAFE_UNINIT(m_InFlightFence);
	return true;
}

bool KVulkanSwapChain::DestroyFrameBuffers()
{
	for (size_t i = 0; i < m_FrameBuffers.size(); ++i)
	{
		FrameBuffer& frameBuffer = m_FrameBuffers[i];
		((KVulkanFrameBuffer*)frameBuffer.colorFrameBuffer.get())->UnInit();
		((KVulkanFrameBuffer*)frameBuffer.depthStencilFrameBuffer.get())->UnInit();
	}
	m_FrameBuffers.clear();

	for (size_t i = 0; i < m_RenderPasses.size(); ++i)
	{
		SAFE_UNINIT(m_RenderPasses[i]);
	}
	m_RenderPasses.clear();

	return true;
}

bool KVulkanSwapChain::Init(IKRenderWindow* window)
{
	UnInit();

	ASSERT_RESULT(m_SwapChain == VK_NULL_HANDLE);
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	m_MaxFramesInFight = KRenderGlobal::NumFramesInFlight;

	m_pWindow = window;

	ASSERT_RESULT(CreateSurface());
	ASSERT_RESULT(CreateSwapChain());
	ASSERT_RESULT(CreateSyncObjects());
	ASSERT_RESULT(CreateFrameBuffers());

	return true;
}

bool KVulkanSwapChain::UnInit()
{
	ASSERT_RESULT(DestroyFrameBuffers());
	ASSERT_RESULT(CleanupSwapChain());
	ASSERT_RESULT(DestroySyncObjects());
	m_MaxFramesInFight = 0;
	m_pWindow = nullptr;

	m_PrePresentCallback.clear();
	m_PostPresentCallback.clear();
	m_SwapChainCallback.clear();

	return true;
}

IKRenderWindow* KVulkanSwapChain::GetWindow()
{
	return m_pWindow;
}

uint32_t KVulkanSwapChain::GetFrameInFlight()
{
	return m_MaxFramesInFight;
}

IKSemaphorePtr KVulkanSwapChain::GetImageAvailableSemaphore()
{
	return m_ImageAvailableSemaphore;
}

IKSemaphorePtr KVulkanSwapChain::GetRenderFinishSemaphore()
{
	return m_RenderFinishedSemaphore;
}

IKFencePtr KVulkanSwapChain::GetInFlightFence()
{
	return m_InFlightFence;
}

IKRenderPassPtr KVulkanSwapChain::GetRenderPass(uint32_t chainIndex)
{
	if (chainIndex < m_RenderPasses.size())
	{
		return m_RenderPasses[chainIndex];
	}
	return nullptr;
}

IKFrameBufferPtr KVulkanSwapChain::GetColorFrameBuffer(uint32_t chainIndex)
{
	if (chainIndex < m_FrameBuffers.size())
	{
		return m_FrameBuffers[chainIndex].colorFrameBuffer;
	}
	return nullptr;
}

IKFrameBufferPtr KVulkanSwapChain::GetDepthStencilFrameBuffer(uint32_t chainIndex)
{
	if (chainIndex < m_FrameBuffers.size())
	{
		return m_FrameBuffers[chainIndex].depthStencilFrameBuffer;
	}
	return nullptr;
}

void KVulkanSwapChain::WaitForInFlightFrame(uint32_t& frameIndex)
{
	// 这个Wait保证Queue已经被执行完
	VkFence fence = ((KVulkanFence*)(m_InFlightFence.get()))->GetVkFence();
	VkResult vkResult = vkWaitForFences(KVulkanGlobal::device, 1, &fence, VK_TRUE, UINT64_MAX);
	VK_ASSERT_RESULT(vkResult);
	frameIndex = m_CurrentFlightIndex;
}

void KVulkanSwapChain::AcquireNextImage(uint32_t& imageIndex)
{
	// 获取可用交换链Image索引 促发交换链Image可用信号量
	VkSemaphore singalSemaphore = ((KVulkanSemaphore*)m_ImageAvailableSemaphore.get())->GetVkSemaphore();
	VkResult vkResult = vkAcquireNextImageKHR(KVulkanGlobal::device, m_SwapChain, UINT64_MAX, singalSemaphore, VK_NULL_HANDLE, &imageIndex);
	m_CurrentImageIndex = imageIndex;
	VK_ASSERT_RESULT(vkResult);
}

void KVulkanSwapChain::PresentQueue(bool& needResize)
{
	for (KDevicePresentCallback* callback : m_PrePresentCallback)
	{
		(*callback)(m_CurrentImageIndex, m_CurrentFlightIndex);
	}

	VkResult vkResult = VK_RESULT_MAX_ENUM;

	// Present等待此信号量
	VkSemaphore signalSemaphores[] = { ((KVulkanSemaphore*)m_RenderFinishedSemaphore.get())->GetVkSemaphore() };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	// 交换链Present前等待命令缓冲提交完成
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {m_SwapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &m_CurrentImageIndex;

	presentInfo.pResults = nullptr;
	vkResult = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

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

	for (KDevicePresentCallback* callback : m_PostPresentCallback)
	{
		(*callback)(m_CurrentImageIndex, m_CurrentFlightIndex);
	}

	m_CurrentFlightIndex = (m_CurrentFlightIndex + 1) %  m_MaxFramesInFight;

	assert(vkResult == VK_SUCCESS || vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR);

	needResize = (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR);
}

bool KVulkanSwapChain::ChooseQueue()
{
	m_PresentQueue = KVulkanGlobal::presentQueue;
	return true;
}

bool KVulkanSwapChain::RegisterPrePresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		m_PrePresentCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanSwapChain::UnRegisterPrePresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		auto it = m_PrePresentCallback.find(callback);
		if (it != m_PrePresentCallback.end())
		{
			m_PrePresentCallback.erase(it);
			return true;
		}
	}
	return false;
}

bool KVulkanSwapChain::RegisterPostPresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		m_PostPresentCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanSwapChain::UnRegisterPostPresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		auto it = m_PostPresentCallback.find(callback);
		if (it != m_PostPresentCallback.end())
		{
			m_PostPresentCallback.erase(it);
			return true;
		}
	}
	return false;
}

bool KVulkanSwapChain::RegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback)
{
	if (callback)
	{
		m_SwapChainCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanSwapChain::UnRegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback)
{
	if (callback)
	{
		auto it = m_SwapChainCallback.find(callback);
		if (it != m_SwapChainCallback.end())
		{
			m_SwapChainCallback.erase(it);
			return true;
		}
	}
	return false;
}

#if defined(_WIN32)
#	pragma warning (disable : 4005)
#	include <Windows.h>
#	include "vulkan/vulkan_win32.h"
#elif defined(__ANDROID__)
#	include "vulkan/vulkan_android.h"
#	include "android_native_app_glue.h"
#endif

bool KVulkanSwapChain::CreateSurface()
{
#ifdef _WIN32
	HWND hwnd = (HWND)(m_pWindow->GetHWND());
	HINSTANCE hInstance = GetModuleHandle(NULL);
	if (hwnd != NULL && hInstance != NULL)
	{
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = (HINSTANCE)hInstance;
		surfaceCreateInfo.hwnd = (HWND)hwnd;
		if (vkCreateWin32SurfaceKHR(KVulkanGlobal::instance, &surfaceCreateInfo, nullptr, &m_Surface) == VK_SUCCESS)
		{
			return true;
		}
	}
#else
	android_app* app = m_pWindow->GetAndroidApp();
	if (app)
	{
		VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.window = app->window;
		if (vkCreateAndroidSurfaceKHR(m_Instance, &surfaceCreateInfo, NULL, &m_Surface) == VK_SUCCESS)
		{
			return true;
		}
	}
#endif
	return false;
}