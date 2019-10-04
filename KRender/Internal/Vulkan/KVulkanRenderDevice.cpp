#include "KVulkanRenderDevice.h"
#include "KVulkanRenderWindow.h"

#include "KVulkanShader.h"
#include "KVulkanProgram.h"
#include "KVulkanBuffer.h"
#include "KVulkanTextrue.h"
#include "KVulkanSampler.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"

#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"

#include "KVulkanDepthBuffer.h"

#include "KVulkanHeapAllocator.h"

#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"

#include "Internal/KConstantGlobal.h"

#include <algorithm>
#include <set>
#include <functional>
#include <assert.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

//-------------------- Extensions --------------------//
const char* DEVICE_EXTENSIONS[] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#define DEVICE_EXTENSIONS_COUNT (sizeof(DEVICE_EXTENSIONS) / sizeof(const char*))

std::vector<const char*> PopulateExtensions(bool bEnableValidationLayer)
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (bEnableValidationLayer)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return std::move(extensions);
}

//-------------------- Validation Layer --------------------//
const char* VALIDATION_LAYER[] =
{
	"VK_LAYER_KHRONOS_validation"
};
#define VALIDATION_LAYER_COUNT (sizeof(VALIDATION_LAYER) / sizeof(const char*))


VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, PFN_vkDebugUtilsMessengerCallbackEXT pCallBack)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = pCallBack;
} 

//-------------------- KVulkanRenderDevice --------------------//
KVulkanRenderDevice::KVulkanRenderDevice()
	: m_pWindow(nullptr),
	m_EnableValidationLayer(
#ifdef _DEBUG
	true
#else
	false
#endif
),
	m_MaxFramesInFight(0),
	m_CurrentFlightIndex(0),
	m_VertexBuffer(nullptr),
	m_IndexBuffer(nullptr),
	m_Texture(nullptr),
	m_Sampler(nullptr)
{

}

KVulkanRenderDevice::~KVulkanRenderDevice()
{

}

bool KVulkanRenderDevice::CheckValidationLayerAvailable()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for(const char* vaildLayer : VALIDATION_LAYER)
	{
		bool bLayerFound = false;
		for(const VkLayerProperties& prop : availableLayers)
		{
			if(strcmp(prop.layerName, vaildLayer) == 0)
			{
				bLayerFound = true;
				break;
			}
		}
		if(!bLayerFound)
		{
			return false;
		}
	}
	return true;
}

KVulkanRenderDevice::QueueFamilyIndices KVulkanRenderDevice::FindQueueFamilies(VkPhysicalDevice vkDevice)
{
	QueueFamilyIndices familyIndices = {};

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &queueFamilyCount, queueFamilies.data());

	familyIndices.graphicsFamily.first = -1;
	familyIndices.graphicsFamily.second = false;
	familyIndices.presentFamily.first = -1;
	familyIndices.presentFamily.second = false;

	int idx = -1;
	for (const auto& queueFamily : queueFamilies)
	{
		++idx;
		// 检查设备索引
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			familyIndices.graphicsFamily.first = idx;
			familyIndices.graphicsFamily.second = true;
		}
		// 检查表现索引
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vkDevice, idx, m_Surface, &presentSupport);
		if(presentSupport)
		{
			familyIndices.presentFamily.first = idx;
			familyIndices.presentFamily.second = true;
		}

		if(familyIndices.IsComplete())
			break;
	}

	return familyIndices;
}

bool KVulkanRenderDevice::CheckDeviceSuitable(PhysicalDevice& device)
{
	if(!device.queueFamilyIndices.IsComplete())
		return false;

	if(!CheckExtentionsSupported(device.device))
		return false;

	if(device.swapChainSupportDetails.formats.empty() || device.swapChainSupportDetails.presentModes.empty())
		return false;

	return true;
}

KVulkanRenderDevice::PhysicalDevice KVulkanRenderDevice::GetPhysicalDeviceProperty(VkPhysicalDevice vkDevice)
{
	PhysicalDevice device;

	device.device = vkDevice;

	vkGetPhysicalDeviceProperties(vkDevice, &device.deviceProperties);
	vkGetPhysicalDeviceFeatures(vkDevice, &device.deviceFeatures);

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &queueFamilyCount, queueFamilies.data());

	device.queueFamilyIndices = FindQueueFamilies(vkDevice);

	SwapChainSupportDetails swapChainDetail = QuerySwapChainSupport(device.device);
	device.swapChainSupportDetails = swapChainDetail;

	device.suitable = CheckDeviceSuitable(device);

	if(device.suitable)
	{
		device.score = 0;

		// Discrete GPUs have a significant performance advantage
		if (device.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			device.score += 1000;
		}

		// Maximum possible size of textures affects graphics quality
		device.score += device.deviceProperties.limits.maxImageDimension2D;
	}
	else
	{
		device.score = -100;
	}

	return device;
}

VkSurfaceFormatKHR KVulkanRenderDevice::ChooseSwapSurfaceFormat()
{
	const auto& formats = m_PhysicalDevice.swapChainSupportDetails.formats;
	for(const VkSurfaceFormatKHR& surfaceFormat : formats)
	{
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return surfaceFormat;
		}
	}

	// 其实这时应该对format进行排序 这里把第一个最为最佳选择
	return m_PhysicalDevice.swapChainSupportDetails.formats[0];
}

VkPresentModeKHR KVulkanRenderDevice::ChooseSwapPresentMode()
{
	VkPresentModeKHR ret = VK_PRESENT_MODE_MAX_ENUM_KHR;

	const auto& presentModes = m_PhysicalDevice.swapChainSupportDetails.presentModes;
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		// 有三重缓冲就使用三重缓冲
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
		// 双重缓冲
		if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
		{
			ret = presentMode;
		}
	}

	// 其实Vulkan保证至少有双重缓冲可以使用
	if(ret == VK_PRESENT_MODE_MAX_ENUM_KHR)
	{
		ret = m_PhysicalDevice.swapChainSupportDetails.presentModes[0];
	}

	return ret;
}

VkExtent2D KVulkanRenderDevice::ChooseSwapExtent()
{
	const VkSurfaceCapabilitiesKHR& capabilities = m_PhysicalDevice.swapChainSupportDetails.capabilities;
	// 如果Vulkan设置了currentExtent 那么交换链的extent就必须与之一致
	if(capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	// 这里可以选择与窗口大小的最佳匹配
	else
	{
		size_t width = 0, height = 0;
		if(m_pWindow->GetSize(width, height))
		{
			VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
		else
		{
			VkExtent2D actualExtent = { capabilities.minImageExtent.width, capabilities.minImageExtent.height };
			return actualExtent;
		}
	}
}

bool KVulkanRenderDevice::CreateSwapChain()
{
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat();
	VkPresentModeKHR presentMode = ChooseSwapPresentMode();
	VkExtent2D extent = ChooseSwapExtent();

	const SwapChainSupportDetails& swapChainSupport = m_PhysicalDevice.swapChainSupportDetails;
	// 设置为最小值可能必须等待驱动程序完成内部操作才能获取另一个要渲染的图像 因此作+1处理
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// Vulkan会把maxImageCount设置为0表示没有最大值限制 这里检查一下有没有超过最大值
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const QueueFamilyIndices &indices = m_PhysicalDevice.queueFamilyIndices;
	assert(indices.IsComplete());
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.first, indices.presentFamily.first};

	// 如果图像队列家族与表现队列家族不一样 需要并行模式支持
	if (indices.graphicsFamily != indices.presentFamily)
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

	// 设置成当前窗口transform避免发生窗口旋转
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	// 避免当前窗口与系统其它窗口发生alpha混合
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	// 这里第一次创建交换链 设置为空即可
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain) == VK_SUCCESS)
	{
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data());

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;

		return true;
	}
	return false;
}

bool KVulkanRenderDevice::CreateImageViews()
{
	m_SwapChainRenderTargets.resize(m_SwapChainImages.size());

	uint32_t msaaCount = 1;

	uint32_t candidate[] = {64,32,16,8,4,2,1};
	VkSampleCountFlagBits flag = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	for(uint32_t count: candidate)
	{
		if(KVulkanHelper::QueryMSAASupport(KVulkanHelper::MST_COLOR, count , flag))
		{
			msaaCount = count;
			break;
		}
	}

	for(size_t i = 0; i < m_SwapChainImages.size(); ++i)
	{
		CreateRenderTarget(m_SwapChainRenderTargets[i]);

#define HEX_COL(NUM) ((float)NUM / float(0XFF))
		m_SwapChainRenderTargets[i]->SetColorClear(HEX_COL(0x87), HEX_COL(0xCE), HEX_COL(0xFF), HEX_COL(0XFF));
#undef HEX_COL
		m_SwapChainRenderTargets[i]->SetDepthStencilClear(1.0, 0);
		m_SwapChainRenderTargets[i]->SetSize(m_SwapChainExtent.width, m_SwapChainExtent.height);

		m_SwapChainRenderTargets[i]->InitFromImage(&m_SwapChainImages[i], &m_SwapChainImageFormat,
			true, false, msaaCount);
	}
	return true;
}

bool KVulkanRenderDevice::CreatePipelines()
{
	IKShaderPtr vertexShader = nullptr;
	IKShaderPtr fragmentShader = nullptr;

	CreateShader(vertexShader);
	CreateShader(fragmentShader);

	ASSERT_RESULT(vertexShader->InitFromFile("shader.vert") && fragmentShader->InitFromFile("shader.frag"));

	m_SwapChainPipelines.resize(m_SwapChainImages.size());
	for(size_t i = 0; i < m_SwapChainImages.size(); ++i)
	{
		CreatePipeline(m_SwapChainPipelines[i]);

		IKPipelinePtr pipeline = m_SwapChainPipelines[i];

		VertexInputDetail bindingDetail = {};
		VertexFormat formats[] = {VF_POINT_NORMAL_UV};
		bindingDetail.formats = formats;
		bindingDetail.count = ARRAY_SIZE(formats);

		pipeline->SetVertexBinding(&bindingDetail, 1);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, vertexShader);
		pipeline->SetShader(ST_FRAGMENT, fragmentShader);

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetConstantBuffer(0, ST_VERTEX, m_CameraBuffer);
		pipeline->SetTextureSampler(1, m_Texture, m_Sampler);

		pipeline->PushConstantBuffer(ST_VERTEX, m_ObjectBuffer);

		pipeline->Init(m_SwapChainRenderTargets[i]);
	}

	vertexShader->UnInit();
	fragmentShader->UnInit();

	return true;
}

bool KVulkanRenderDevice::CreateSurface()
{
	GLFWwindow *glfwWindow = m_pWindow->GetGLFWwindow();
	if(glfwCreateWindowSurface(m_Instance, glfwWindow, nullptr, &m_Surface)== VK_SUCCESS)
	{
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::PickPhysicsDevice()
{
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
	if (deviceCount > 0)
	{
		std::vector<VkPhysicalDevice> vkDevices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, vkDevices.data());

		std::vector<PhysicalDevice> devices;

		std::for_each(vkDevices.begin(), vkDevices.end(), [&](VkPhysicalDevice& device)
		{
			PhysicalDevice prop = GetPhysicalDeviceProperty(device);
			devices.push_back(prop);
		});

		for(auto it = devices.begin(); it != devices.end();)
		{
			if(it->suitable)
			{
				++it;
			}
			else
			{
				it = devices.erase(it);
			}
		}

		if(devices.size() > 0)
		{
			std::sort(devices.begin(), devices.end(), [&](PhysicalDevice& lhs, PhysicalDevice rhs)->bool
			{
				return lhs.score < rhs.score;
			});

			m_PhysicalDevice = devices[0];
			return true;
		}
	}
	return false;
}

bool KVulkanRenderDevice::CreateLogicalDevice()
{
	QueueFamilyIndices indices = m_PhysicalDevice.queueFamilyIndices;

	if(indices.IsComplete())
	{
		std::set<QueueFamilyIndices::QueueFamilyIndex> uniqueIndices;
		uniqueIndices.insert(indices.graphicsFamily);
		uniqueIndices.insert(indices.presentFamily);

		std::vector<VkDeviceQueueCreateInfo> DeviceQueueCreateInfos;

		for(auto& index : uniqueIndices)
		{
			assert(index.second);

			// 填充VkDeviceQueueCreateInfo
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = index.first;
			queueCreateInfo.queueCount = 1;

			float queuePriority = 1.0f;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			DeviceQueueCreateInfos.push_back(queueCreateInfo);
		}
		VkResult RES = VK_TIMEOUT;

		// 填充VkDeviceCreateInfo
		VkDeviceCreateInfo createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = DeviceQueueCreateInfos.data();
		createInfo.queueCreateInfoCount = (uint32_t)DeviceQueueCreateInfos.size();

		createInfo.enabledExtensionCount = DEVICE_EXTENSIONS_COUNT;
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS;

		VkPhysicalDeviceFeatures deviceFeatures = {};
		createInfo.pEnabledFeatures = &deviceFeatures;

		// 尽管最新Vulkan实例验证层与设备验证层已经统一
		// 但是最好保留代码兼容性
		if (m_EnableValidationLayer)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYER_COUNT);
			createInfo.ppEnabledLayerNames = VALIDATION_LAYER;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_PhysicalDevice.device, &createInfo, nullptr, &m_Device) == VK_SUCCESS)
		{
			vkGetDeviceQueue(m_Device, indices.graphicsFamily.first, 0, &m_GraphicsQueue);
			vkGetDeviceQueue(m_Device, indices.presentFamily.first, 0, &m_PresentQueue);
			return true;
		}
	}
	return false;
}

bool KVulkanRenderDevice::CreateCommandPool()
{
	assert(m_PhysicalDevice.queueFamilyIndices.IsComplete());

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// 指定该命令池所属的队列家族
	poolInfo.queueFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;
	// 为PushConstant所用
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) == VK_SUCCESS)
	{
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::CreateSyncObjects()
{
	// N个大小的交换链设定N-1个作为FramesInFight
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
		if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS)
		{
			return false;
		}
		if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			return false;
		}
		if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
		{
			return false;
		}
	}
	return true;
}

bool KVulkanRenderDevice::DestroySyncObjects()
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

bool KVulkanRenderDevice::UpdateFrameTime()
{
	static int numFrames = 0;

	++numFrames;
	float time = m_Timer.GetMilliseconds();
	time = time > 0 ? time : 0.00001f;

	if(time > 1000.0f)
	{
		time = time / (float)numFrames;
		float fps = 1000.0f / time;
		char szBuffer[1024] = {};
		sprintf(szBuffer, "[FPS] %f [FrameTime] %f", fps, time);
		m_pWindow->SetWindowTitle(szBuffer);
		m_Timer.Reset();
		numFrames = 0;
	}

	return true;
}

bool KVulkanRenderDevice::CreateVertexInput()
{
	using namespace KVertexDefinition;

	const POS_3F_NORM_3F_UV_2F vertices[] =
	{
		{glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
		{glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
		{glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
		{glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},
	};

	CreateVertexBuffer(m_VertexBuffer);
	m_VertexBuffer->InitMemory(sizeof(vertices) / sizeof(vertices[0]), sizeof(vertices[0]), vertices);
	m_VertexBuffer->InitDevice();

	const uint16_t indices[] = {0, 1, 2, 2, 3, 0};
	CreateIndexBuffer(m_IndexBuffer);
	m_IndexBuffer->InitMemory(IT_16, sizeof(indices) / sizeof(indices[0]), indices);
	m_IndexBuffer->InitDevice();
	return true;
}

bool KVulkanRenderDevice::CreateUniform()
{
	m_ObjectBuffer = nullptr;
	CreateUniformBuffer(m_ObjectBuffer);
	m_ObjectBuffer->InitMemory(KConstantDefinition::GetConstantBufferDetail(CBT_OBJECT).bufferSize,
		nullptr);
	m_ObjectBuffer->InitDevice(CUT_PUSH_CONSTANT);

	m_CameraBuffer = nullptr;
	CreateUniformBuffer(m_CameraBuffer);
	m_CameraBuffer->InitMemory(KConstantDefinition::GetConstantBufferDetail(CBT_CAMERA).bufferSize,
		KConstantGlobal::GetGlobalConstantData(CBT_CAMERA));
	m_CameraBuffer->InitDevice(CUT_REGULAR);

#ifdef _DEBUG
	const int width = 40;
	const int height = 40;
#else
	const int width = 400;
	const int height = 400;
#endif

	m_ObjectTransforms.clear();
	m_ObjectTransforms.reserve(width * height);

	m_ObjectFinalTransforms.clear();
	m_ObjectFinalTransforms.reserve(width * height);
	for(size_t x = 0; x < width; ++x)
	{
		for(size_t y = 0; y < height; ++y)
		{
			float xPos = ((float)x - (float)width / 2.0f);
			float yPos = ((float)y - (float)height / 2.0f);
			ObjectInitTransform transform = 
			{
				glm::rotate(glm::mat4(1.0f),
					glm::radians((1000.0f * float(rand() % 1000)) * glm::two_pi<float>()), 
					glm::vec3(0.0f, 0.0f, 1.0f)),
				glm::translate(glm::mat4(1.0f), glm::vec3(xPos, yPos, 0.0f)),
			};
			m_ObjectTransforms.push_back(transform);
			m_ObjectFinalTransforms.push_back(glm::mat4(1.0f));
		}
	}

	return true;
}

bool KVulkanRenderDevice::CreateTex()
{
	CreateTexture(m_Texture);
	m_Texture->InitMemory("texture.jpg", true);
	m_Texture->InitDevice();

	CreateSampler(m_Sampler);
	//m_Sampler->SetAnisotropic(true);
	//m_Sampler->SetAnisotropicCount(16);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->SetMipmapLod(0, m_Texture->GetMipmaps());
	m_Sampler->Init();

	return true;
}

VkBool32 KVulkanRenderDevice::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
	)
{
	if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	{
		printf("[Vulkan Validation Layer Debug] %s\n", pCallbackData->pMessage);
	}
	else
	{
		printf("[Vulkan Validation Layer Error] %s\n", pCallbackData->pMessage);
		assert(false);
	}
	return VK_FALSE;
}

bool KVulkanRenderDevice::SetupDebugMessenger()
{
	if(m_EnableValidationLayer)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		PopulateDebugMessengerCreateInfo(createInfo, DebugCallback);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) == VK_SUCCESS)
		{
			return true;
		}
		return false;
	}
	return true;
}

bool KVulkanRenderDevice::UnsetDebugMessenger()
{
	if(m_EnableValidationLayer)
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}
	return true;
}

bool KVulkanRenderDevice::Init(IKRenderWindowPtr window)
{
	if(!window)
		return false;

	KVulkanRenderWindow* renderWindow = (KVulkanRenderWindow*)window.get();
	if(renderWindow == nullptr || renderWindow->GetGLFWwindow() == nullptr)
		return false;
	m_pWindow = renderWindow;

	VkApplicationInfo appInfo = {};

	// 描述实例
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "KVulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "KVulkanEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	// 挂接验证层
	if(m_EnableValidationLayer)
	{
		bool bCheckResult = CheckValidationLayerAvailable();
		assert(bCheckResult);
		createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
		createInfo.ppEnabledLayerNames = VALIDATION_LAYER;

		// 这是为了检查vkCreateInstance与SetupDebugMessenger之间的错误
		PopulateDebugMessengerCreateInfo(debugCreateInfo, DebugCallback);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// 挂接扩展
	auto extensions = PopulateExtensions(m_EnableValidationLayer);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) == VK_SUCCESS)
	{
		if(!SetupDebugMessenger())
			return false;
		if(!CreateSurface())
			return false;
		if(!PickPhysicsDevice())
			return false;
		if(!CreateLogicalDevice())
			return false;
		if(!CreateCommandPool())
			return false;
		// 这里已经实际完成了设备初始化
		PostInit();

		if(!KVulkanHeapAllocator::Init())
			return false;

		if(!CreateSwapChain())
			return false;
		if(!CreateImageViews())
			return false;
		if(!CreateSyncObjects())
			return false;

		// Temporarily for demo use
		if(!CreateVertexInput())
			return false;
		if(!CreateUniform())
			return false;
		if(!CreateTex())
			return false;
		if(!CreatePipelines())
			return false;
		m_ThreadPool.PushWorkerThreads(std::thread::hardware_concurrency());
		if(!CreateCommandBuffers())
			return false;

		m_pWindow->SetVulkanDevice(this);
		return true;
	}
	else
	{
		memset(&m_Instance, 0, sizeof(m_Instance));
	}
	return false;
}

/*
总共有三个入口可以侦查并促发到交换链重建
1.glfw窗口大小改变
2.vkAcquireNextImageKHR
3.vkQueuePresentKHR
技术上只有要一个入口成功合理的创建了交换链之后
vkAcquireNextImageKHR或者vkQueuePresentKHR不会侦查到交换链需要重新创建
*/
bool KVulkanRenderDevice::RecreateSwapChain()
{
	m_pWindow->IdleUntilForeground();
	vkDeviceWaitIdle(m_Device);
	// 记得要重新更新SwapChainDetail
	SwapChainSupportDetails detail = QuerySwapChainSupport(m_PhysicalDevice.device);
	m_PhysicalDevice.swapChainSupportDetails = detail;

	CleanupSwapChain();
	/*
	设计上FramesInFight的数量与交换链数量是耦合的
	所以这里重新构建信号量和栏栅
	*/
	DestroySyncObjects();

	CreateSwapChain();
	CreateImageViews();
	CreateUniform();
	CreatePipelines();
	CreateCommandBuffers();

	CreateSyncObjects();

	return true;
}

bool KVulkanRenderDevice::CleanupSwapChain()
{
	for (IKRenderTargetPtr target :  m_SwapChainRenderTargets)
	{
		target->UnInit();
	}
	m_SwapChainRenderTargets.clear();

	if(m_ObjectBuffer)
	{
		m_ObjectBuffer->UnInit();
		m_ObjectBuffer = nullptr;
	}
	if(m_CameraBuffer)
	{
		m_CameraBuffer->UnInit();
		m_CameraBuffer = nullptr;
	}

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		for (ThreadData& thread : m_CommandBuffers[i].threadDatas)
		{
			vkDestroyCommandPool(m_Device, thread.commandPool, nullptr);
		}
	}
	m_CommandBuffers.clear();

	for (IKPipelinePtr pipeline :  m_SwapChainPipelines)
	{
		pipeline->UnInit();
	}
	m_SwapChainPipelines.clear();


	m_SwapChainImages.clear();

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	return true;
}

bool KVulkanRenderDevice::UnInit()
{
	//m_RenderThreadPool.Wait();
	m_ThreadPool.WaitAllAsyncTaskDone();
	
	m_pWindow = nullptr;

	if(m_VertexBuffer)
	{
		m_VertexBuffer->UnInit();
		m_VertexBuffer = nullptr;
	}
	if(m_IndexBuffer)
	{
		m_IndexBuffer->UnInit();
		m_VertexBuffer = nullptr;
	}
	if(m_Texture)
	{
		m_Texture->UnInit();
		m_Texture = nullptr;
	}
	if(m_Sampler)
	{
		m_Sampler->UnInit();
		m_Sampler = nullptr;
	}

	CleanupSwapChain();

	DestroySyncObjects();
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	KVulkanHeapAllocator::UnInit();

	UnsetDebugMessenger();
	vkDestroyDevice(m_Device, nullptr);
	vkDestroyInstance(m_Instance, nullptr);

	PostUnInit();
	return true;
}

KVulkanRenderDevice::SwapChainSupportDetails KVulkanRenderDevice::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool KVulkanRenderDevice::CheckExtentionsSupported(VkPhysicalDevice vkDevice)
{
	uint32_t extensionCount = 0;
	if(vkEnumerateDeviceExtensionProperties(vkDevice, nullptr, &extensionCount, nullptr) == VK_SUCCESS)
	{
		std::vector<VkExtensionProperties> extensions(extensionCount);
		if(vkEnumerateDeviceExtensionProperties(vkDevice, nullptr, &extensionCount, extensions.data()) == VK_SUCCESS)
		{
			// 确保Vulkan具有我们需要的扩展
			for(const char* requiredExt : DEVICE_EXTENSIONS)
			{
				if(std::find_if(extensions.begin(),
					extensions.end(),
					[&](VkExtensionProperties& prop)->bool
				{
					if(strcmp(prop.extensionName, requiredExt) == 0)
						return true;
					return false;
				}) == extensions.end())
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool KVulkanRenderDevice::PostInit()
{
	KVulkanGlobal::device = m_Device;
	KVulkanGlobal::physicalDevice = m_PhysicalDevice.device;
	KVulkanGlobal::graphicsCommandPool = m_CommandPool;
	KVulkanGlobal::graphicsQueue = m_GraphicsQueue;

	KVulkanGlobal::deviceReady = true;
	return true;
}

bool KVulkanRenderDevice::PostUnInit()
{
	KVulkanGlobal::deviceReady = false;
	return true;
}

bool KVulkanRenderDevice::CreateShader(IKShaderPtr& shader)
{
	shader = IKShaderPtr(new KVulkanShader());
	return true; 
}

bool KVulkanRenderDevice::CreateProgram(IKProgramPtr& program)
{
	program = IKProgramPtr(new KVulkanProgram());
	return true;
}

bool KVulkanRenderDevice::CreateVertexBuffer(IKVertexBufferPtr& buffer)
{
	buffer = IKVertexBufferPtr(static_cast<IKVertexBuffer*>(new KVulkanVertexBuffer()));
	return true;
}

bool KVulkanRenderDevice::CreateIndexBuffer(IKIndexBufferPtr& buffer)
{
	buffer = IKIndexBufferPtr(static_cast<IKIndexBuffer*>(new KVulkanIndexBuffer()));
	return true;
}

bool KVulkanRenderDevice::CreateUniformBuffer(IKUniformBufferPtr& buffer)
{
	buffer = IKUniformBufferPtr(static_cast<IKUniformBuffer*>(new KVulkanUniformBuffer()));
	return true;
}

bool KVulkanRenderDevice::CreateTexture(IKTexturePtr& texture)
{
	texture = IKTexturePtr(static_cast<IKTexture*>(new KVulkanTexture()));
	return true;
}

bool KVulkanRenderDevice::CreateSampler(IKSamplerPtr& sampler)
{
	sampler = IKSamplerPtr(static_cast<IKSampler*>(new KVulkanSampler()));
	return true;	
}

bool KVulkanRenderDevice::CreateRenderTarget(IKRenderTargetPtr& target)
{
	target = IKRenderTargetPtr(static_cast<IKRenderTarget*>(new KVulkanRenderTarget()));
	return true;
}

bool KVulkanRenderDevice::CreatePipeline(IKPipelinePtr& pipeline)
{
	pipeline = IKPipelinePtr(static_cast<IKPipeline*>(new KVulkanPipeline()));
	return true;
}

bool KVulkanRenderDevice::UpdateCamera()
{
	glm::mat4 view = glm::lookAt(glm::vec3(0, 400.0f, 400.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_SwapChainExtent.width / (float) m_SwapChainExtent.height, 0.1f, 3000.0f);
	proj[1][1] *= -1;

	void* pWritePos = nullptr;
	void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CAMERA);	
	const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CAMERA);
	for(KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
	{
		if(detail.semantic == CS_VIEW)
		{
			pWritePos = POINTER_OFFSET(pData, detail.offset);
			assert(sizeof(view) == detail.size);
			memcpy(pWritePos, &view, sizeof(view));
		}
		else if(detail.semantic == CS_PROJ)
		{
			pWritePos = POINTER_OFFSET(pData, detail.offset);
			assert(sizeof(proj) == detail.size);
			memcpy(pWritePos, &proj, sizeof(proj));
		}
	}
	m_CameraBuffer->Write(pData);

	return true;
}

bool KVulkanRenderDevice::UpdateObjectTransform()
{
	static auto lastTime = std::chrono::high_resolution_clock::now();
	static auto firstTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();

	float fLastTime = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>>(lastTime - firstTime).count();
	float fCurrentTime = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>>(currentTime - firstTime).count();

	glm::mat4 deltaRotate = glm::rotate(glm::mat4(1.0f), (fCurrentTime - fLastTime) * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	const float speed = 1.0f;
	const float maxRange = 5.0f;
	float translate = maxRange * ((sinf(fCurrentTime * speed) - sinf(fLastTime * speed)) / glm::pi<float>());

	glm::mat4 deltaTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, translate));

	for(size_t i = 0; i < m_ObjectFinalTransforms.size(); ++i)
	{
		ObjectInitTransform& transform = m_ObjectTransforms[i];

		transform.translate = deltaTranslate * transform.translate;
		transform.rotate = deltaRotate * transform.rotate;

		glm::mat4& model = m_ObjectFinalTransforms[i];
		model = transform.translate * transform.rotate;
	}

	lastTime = currentTime;
	return true;
}

#define MULTITHREAD_SUBMIT_COMMAND

void KVulkanRenderDevice::ThreadRenderObject(uint32_t threadIndex, uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	ThreadData& threadData = m_CommandBuffers[imageIndex].threadDatas[threadIndex];
	
	// 命令开始时候创建需要一个命令开始信息
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // Optional
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	VkCommandBuffer commandBuffer = threadData.commandBuffer;
	VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));	

	KVulkanRenderTarget* target = (KVulkanRenderTarget*)m_SwapChainRenderTargets[imageIndex].get();
	{
		KVulkanPipeline* swapchainPipeline = (KVulkanPipeline*)m_SwapChainPipelines[imageIndex].get();

		VkPipeline pipeline = swapchainPipeline->GetVkPipeline();
		VkPipelineLayout pipelineLayout = swapchainPipeline->GetVkPipelineLayout();
		VkDescriptorSet descriptorSet = swapchainPipeline->GetVkDescriptorSet();

		// 绑定管线
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// 绑定管线布局
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// 绑定顶点缓冲
		KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_VertexBuffer.get();
		VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		// 绑定索引缓冲
		KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_IndexBuffer.get();
		vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

		size_t numThread = m_ThreadPool.GetWorkerThreadNum();
		size_t numPerThread = m_ObjectTransforms.size() / numThread;
		size_t numRemain = m_ObjectTransforms.size() % numThread;
		size_t numThisThread = numPerThread + ((threadIndex + 1) == numThread ? numRemain : 0);

		for(size_t i = 0; i < numThisThread; ++i)
		{
			glm::mat4& model = m_ObjectFinalTransforms[i + numPerThread * threadIndex];
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)m_ObjectBuffer->GetBufferSize(), &model);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
		}
	}
	VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));
}

bool KVulkanRenderDevice::SubmitCommandBufferSingleThread(unsigned int imageIndex)
{
	assert(imageIndex < m_CommandBuffers.size());

	VkCommandBuffer commandBuffer = m_CommandBuffers[imageIndex].primaryCommandBuffer;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// 开始渲染过程
	VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));	
	{
		KVulkanRenderTarget* target = (KVulkanRenderTarget*)m_SwapChainRenderTargets[imageIndex].get();

		// 创建开始渲染过程描述
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		// 指定渲染通道
		renderPassInfo.renderPass = target->GetRenderPass();
		// 指定帧缓冲
		renderPassInfo.framebuffer = target->GetFrameBuffer();

		renderPassInfo.renderArea.offset.x = 0;
		renderPassInfo.renderArea.offset.y = 0;
		renderPassInfo.renderArea.extent = target->GetExtend();

		// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致
		auto clearValuesPair = target->GetVkClearValues();
		renderPassInfo.pClearValues = clearValuesPair.first;
		renderPassInfo.clearValueCount = clearValuesPair.second;

		// 开始渲染过程
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			KVulkanPipeline* swapchainPipeline = (KVulkanPipeline*)m_SwapChainPipelines[imageIndex].get();

			VkPipeline pipeline = swapchainPipeline->GetVkPipeline();
			VkPipelineLayout pipelineLayout = swapchainPipeline->GetVkPipelineLayout();
			VkDescriptorSet descriptorSet = swapchainPipeline->GetVkDescriptorSet();

			// 绑定管线
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			// 绑定管线布局
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			// 绑定顶点缓冲
			KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_VertexBuffer.get();
			VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			// 绑定索引缓冲
			KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_IndexBuffer.get();
			vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

			//vkCmdDraw(m_CommandBuffers[i], (uint32_t)vulkanVertexBuffer->GetVertexCount(), 1, 0, 0);
			// 绘制调用
			for(glm::mat4& model : m_ObjectFinalTransforms)
			{
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)m_ObjectBuffer->GetBufferSize(), &model);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
			}
		}
		// 结束渲染过程
		vkCmdEndRenderPass(commandBuffer);
	}
	VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));

	return true;
}

bool KVulkanRenderDevice::SubmitCommandBufferMuitiThread(unsigned int imageIndex)
{
	assert(imageIndex < m_CommandBuffers.size());
	KVulkanRenderTarget* target = (KVulkanRenderTarget*)m_SwapChainRenderTargets[imageIndex].get();

	// 创建开始渲染过程描述
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	// 指定渲染通道
	renderPassInfo.renderPass = target->GetRenderPass();
	// 指定帧缓冲
	renderPassInfo.framebuffer = target->GetFrameBuffer();

	renderPassInfo.renderArea.offset.x = 0;
	renderPassInfo.renderArea.offset.y = 0;
	renderPassInfo.renderArea.extent = target->GetExtend();

	// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致
	auto clearValuesPair = target->GetVkClearValues();
	renderPassInfo.pClearValues = clearValuesPair.first;
	renderPassInfo.clearValueCount = clearValuesPair.second;

	VkCommandBuffer primaryCommandBuffer = m_CommandBuffers[imageIndex].primaryCommandBuffer;
	// 开始渲染过程
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_ASSERT_RESULT(vkBeginCommandBuffer(primaryCommandBuffer, &cmdBufferBeginInfo));
	{
		vkCmdBeginRenderPass(primaryCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		{
			VkCommandBufferInheritanceInfo inheritanceInfo = {};
			inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.renderPass = ((KVulkanRenderTarget*)m_SwapChainRenderTargets[imageIndex].get())->GetRenderPass();
			inheritanceInfo.framebuffer = ((KVulkanRenderTarget*)m_SwapChainRenderTargets[imageIndex].get())->GetFrameBuffer();

			for(size_t i = 0; i < m_ThreadPool.GetWorkerThreadNum(); ++i)
			{
				m_ThreadPool.SubmitTask([=]()
				{
					ThreadRenderObject((uint32_t)i, imageIndex, inheritanceInfo);
				});
			}

			m_ThreadPool.WaitAllAsyncTaskDone();

			std::vector<VkCommandBuffer> commandBuffers;
			size_t numThread = m_ThreadPool.GetWorkerThreadNum();
			for(size_t threadIndex = 0; threadIndex < numThread; ++threadIndex)
			{
				ThreadData& threadData = m_CommandBuffers[imageIndex].threadDatas[threadIndex];
				if(threadIndex == 0 || numThread <= m_ObjectTransforms.size())
				{
					commandBuffers.push_back(threadData.commandBuffer);
				}
			}

			vkCmdExecuteCommands(primaryCommandBuffer, (uint32_t)commandBuffers.size(), commandBuffers.data());
		}
		vkCmdEndRenderPass(primaryCommandBuffer);
	}
	VK_ASSERT_RESULT(vkEndCommandBuffer(primaryCommandBuffer));

	return true;
}

bool KVulkanRenderDevice::CreateCommandBuffers()
{
	// 交换链上的每个帧缓冲都需要提交命令
	m_CommandBuffers.resize(m_SwapChainRenderTargets.size());

	size_t numThread = m_ThreadPool.GetWorkerThreadNum();

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffers[i].primaryCommandBuffer);

		m_CommandBuffers[i].threadDatas.resize(numThread);

		size_t numThread = m_ThreadPool.GetWorkerThreadNum();
		size_t numPerThread = m_ObjectTransforms.size() / numThread;
		size_t numRemain = m_ObjectTransforms.size() % numThread;

		for (size_t threadIdx = 0; threadIdx < numThread; ++threadIdx)
		{
			ThreadData& threadData = m_CommandBuffers[i].threadDatas[threadIdx];

			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VK_ASSERT_RESULT(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &threadData.commandPool));

			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = threadData.commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = 1;

			vkAllocateCommandBuffers(m_Device, &allocInfo, &threadData.commandBuffer);
		}

	}
	return true;
}


bool KVulkanRenderDevice::Present()
{
	vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFlightIndex], VK_TRUE, UINT64_MAX);	

	uint32_t imageIndex = UINT32_MAX;
	VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFlightIndex], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return true;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		return false;
	}

	UpdateCamera();
	UpdateObjectTransform();

	VkCommandBuffer primaryCommandBuffer = m_CommandBuffers[imageIndex].primaryCommandBuffer;
#ifdef MULTITHREAD_SUBMIT_COMMAND
	SubmitCommandBufferMuitiThread(imageIndex);
#else
	SubmitCommandBufferSingleThread(imageIndex);
#endif
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// 交换链可以用于绘制时将促发此信号量
	VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFlightIndex]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// 指定命令缓冲
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &primaryCommandBuffer;

	// 交换链绘制完成时将促发此信号量
	VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFlightIndex]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// vkResetFences放置到vkQueueSubmit之前 保证调用vkWaitForFences都不会无限死锁
	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFlightIndex]);
	// 提交该绘制命令
	if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFlightIndex]) != VK_SUCCESS)
	{
		return false;
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {m_SwapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
	}
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
	if (result != VK_SUCCESS)
	{
		return false;
	}
	UpdateFrameTime();
	return true;
}

bool KVulkanRenderDevice::Wait()
{
	vkDeviceWaitIdle(m_Device);
	return true;
}