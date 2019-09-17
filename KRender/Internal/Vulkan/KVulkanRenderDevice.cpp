#include "KVulkanRenderDevice.h"
#include "KVulkanRenderWindow.h"
#include "KVulkanShader.h"
#include "KVulkanProgram.h"
#include "KVulkanBuffer.h"

#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"

#include "Internal/KConstantGlobal.h"

#include <algorithm>
#include <set>
#include <functional>
#include <assert.h>

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
	m_EnableValidationLayer(true),
	m_MaxFramesInFight(0),
	m_CurrentFlightIndex(0),
	m_VertexBuffer(nullptr),
	m_IndexBuffer(nullptr)
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
		// ����豸����
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			familyIndices.graphicsFamily.first = idx;
			familyIndices.graphicsFamily.second = true;
		}
		// ����������
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

	SwapChainSupportDetails detail = QuerySwapChainSupport(device.device);
	device.swapChainSupportDetails = detail;

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

	// ��ʵ��ʱӦ�ö�format�������� ����ѵ�һ����Ϊ���ѡ��
	return m_PhysicalDevice.swapChainSupportDetails.formats[0];
}

VkPresentModeKHR KVulkanRenderDevice::ChooseSwapPresentMode()
{
	VkPresentModeKHR ret = VK_PRESENT_MODE_MAX_ENUM_KHR;

	const auto& presentModes = m_PhysicalDevice.swapChainSupportDetails.presentModes;
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		// �����ػ����ʹ�����ػ���
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
		// ˫�ػ���
		if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
		{
			ret = presentMode;
		}
	}

	// ��ʵVulkan��֤������˫�ػ������ʹ��
	if(ret == VK_PRESENT_MODE_MAX_ENUM_KHR)
	{
		ret = m_PhysicalDevice.swapChainSupportDetails.presentModes[0];
	}

	return ret;
}

VkExtent2D KVulkanRenderDevice::ChooseSwapExtent()
{
	const VkSurfaceCapabilitiesKHR& capabilities = m_PhysicalDevice.swapChainSupportDetails.capabilities;
	// ���Vulkan������currentExtent ��ô��������extent�ͱ�����֮һ��
	if(capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	// �������ѡ���봰�ڴ�С�����ƥ��
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
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const QueueFamilyIndices &indices = m_PhysicalDevice.queueFamilyIndices;
	assert(indices.IsComplete());
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.first, indices.presentFamily.first};

	// ���ͼ����м�������ֶ��м��岻һ�� ��Ҫ����ģʽ֧��
	if (indices.graphicsFamily != indices.presentFamily)
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

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	// �����һ�δ��������� ����Ϊ�ռ���
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
	m_SwapChainImageViews.resize(m_SwapChainImages.size());
	for(size_t i = 0; i < m_SwapChainImages.size(); ++i)
	{
		VkImage& image = m_SwapChainImages[i];
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		// ����image
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		// format�뽻����formatͬ��
		createInfo.format = m_SwapChainImageFormat;
		// ����Ĭ��rgbaӳ����Ϊ
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		// ָ��View���ʷ�Χ
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
		{
			for(size_t j = 0; j < i; ++j)
			{
				vkDestroyImageView(m_Device, m_SwapChainImageViews[j], nullptr);
			}
			m_SwapChainImageViews.clear();
			return false;
		}
	}
	return true;
}

bool KVulkanRenderDevice::CreateFramebuffers()
{
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());
	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &m_SwapChainImageViews[i];
		framebufferInfo.width = m_SwapChainExtent.width;
		framebufferInfo.height = m_SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			for(size_t j = 0; j < i; ++j)
			{
				vkDestroyFramebuffer(m_Device, m_SwapChainFramebuffers[j], nullptr);
			}
			m_SwapChainFramebuffers.clear();
			return false;
		}
	}
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

			// ���VkDeviceQueueCreateInfo
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = index.first;
			queueCreateInfo.queueCount = 1;

			float queuePriority = 1.0f;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			DeviceQueueCreateInfos.push_back(queueCreateInfo);
		}

		// ���VkDeviceCreateInfo		
		VkDeviceCreateInfo createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = DeviceQueueCreateInfos.data();
		createInfo.queueCreateInfoCount = (uint32_t)DeviceQueueCreateInfos.size();

		createInfo.enabledExtensionCount = DEVICE_EXTENSIONS_COUNT;
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS;

		VkPhysicalDeviceFeatures deviceFeatures = {};
		createInfo.pEnabledFeatures = &deviceFeatures;

		// ��������Vulkanʵ����֤�����豸��֤���Ѿ�ͳһ
		// ������ñ������������
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

bool KVulkanRenderDevice::CreateRenderPass()
{
	// ����Attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_SwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// ����Attachment���ýṹ
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// ������ͨ��
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	// ������Ⱦͨ��
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	// ��������
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS)
	{
		return true;
	}
	return false;
}

struct ProgramHolder
{
	IKShaderPtr VSShader;
	IKShaderPtr FGShader;
	IKProgramPtr Program;

	ProgramHolder()
	{
		VSShader = nullptr;
		FGShader = nullptr;
		Program = nullptr;
	}

	~ProgramHolder()
	{
		if(VSShader)
		{
			VSShader->UnInit();
			VSShader = nullptr;
		}
		if(FGShader)
		{
			FGShader->UnInit();
			FGShader = nullptr;
		}
		if(Program)
		{
			Program->UnInit();
			Program = nullptr;
		}
	}
};

bool KVulkanRenderDevice::CreateGraphicsPipeline()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	for(KVulkanHelper::VulkanBindingDetail& detail : m_VertexBindDetailList)
	{
		bindingDescriptions.push_back(detail.bindingDescription);
		attributeDescriptions.insert(
			attributeDescriptions.end(),
			detail.attributeDescriptions.begin(), detail.attributeDescriptions.end()
			);
	}

	// ���ö���������Ϣ
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;	

	vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// ���ö�����װ��Ϣ
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// �����ӿڲü�
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) m_SwapChainExtent.width;
	viewport.height = (float) m_SwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = 
	{
		{0, 0},
		m_SwapChainExtent
	};

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// ���ù�դ����Ϣ
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// ���ö��ز�����Ϣ
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// ����Alpha�����Ϣ
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// ���ö�̬״̬
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// �������߲���
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
	{
		return false;
	}

	ProgramHolder holder;

	CreateShader(holder.VSShader);
	CreateShader(holder.FGShader);

	if(!(holder.VSShader->InitFromFile("shader.vert") && holder.FGShader->InitFromFile("shader.frag")))
	{
		return false;
	}

	CreateProgram(holder.Program);
	holder.Program->AttachShader(ST_VERTEX, holder.VSShader);
	holder.Program->AttachShader(ST_FRAGMENT, holder.FGShader);
	holder.Program->Init();
	const std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo = ((KVulkanProgram*)holder.Program.get())->GetShaderStageInfo();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// ָ��������ʹ�õ�Shader
	pipelineInfo.stageCount = (uint32_t)shaderStageCreateInfo.size();
	pipelineInfo.pStages = shaderStageCreateInfo.data();
	// ָ�����߶���������Ϣ
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	// ָ�������ӿ�
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	// ָ�����ز�����ʽ
	pipelineInfo.pMultisampleState = &multisampling;
	// ָ�����ģʽ
	pipelineInfo.pColorBlendState = &colorBlending;
	// ָ�����߲���
	pipelineInfo.layout = m_PipelineLayout;
	// ָ����Ⱦͨ��
	pipelineInfo.renderPass = m_RenderPass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

bool KVulkanRenderDevice::CreateCommandPool()
{
	assert(m_PhysicalDevice.queueFamilyIndices.IsComplete());

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// ָ��������������Ķ��м���
	poolInfo.queueFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) == VK_SUCCESS)
	{
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::CreateSyncObjects()
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

bool KVulkanRenderDevice::CreateCommandBuffers()
{
	// �������ϵ�ÿ��֡���嶼��Ҫ�ύ����
	m_CommandBuffers.resize(m_SwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};	
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
	{
		m_CommandBuffers.clear();
		return false;
	}

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		// ���ʼʱ�򴴽���Ҫһ�����ʼ��Ϣ
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			m_CommandBuffers.clear();
			return false;
		}
		{
			// ������ʼ��Ⱦ��������
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			// ָ����Ⱦͨ��
			renderPassInfo.renderPass = m_RenderPass;
			// ָ��֡����
			renderPassInfo.framebuffer = m_SwapChainFramebuffers[i];

			renderPassInfo.renderArea.offset.x = 0;
			renderPassInfo.renderArea.offset.y = 0;
			renderPassInfo.renderArea.extent = m_SwapChainExtent;

			VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			// ��ʼ��Ⱦ����
			vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

				KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_VertexBuffer.get();
				VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

				KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_IndexBuffer.get();
				vkCmdBindIndexBuffer(m_CommandBuffers[i], vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

				//vkCmdDraw(m_CommandBuffers[i], (uint32_t)vulkanVertexBuffer->GetVertexCount(), 1, 0, 0);
				vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
			}
			// ������Ⱦ����
			vkCmdEndRenderPass(m_CommandBuffers[i]);
		}

		if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS)
		{
			m_CommandBuffers.clear();
			return false;
		}
	}
	return true;
}

bool KVulkanRenderDevice::CreateVertexInput()
{
	using namespace KVertexDefinition;

	const POS_3F_NORM_3F_UV_2F vertices[] =
	{
		{glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
		{glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
		{glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
		{glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)}
	};

	CreateVertexBuffer(m_VertexBuffer);
	m_VertexBuffer->InitMemory(sizeof(vertices) / sizeof(vertices[0]), sizeof(vertices[0]), vertices);
	m_VertexBuffer->InitDevice();

	const uint16_t indices[] = {0, 1, 2, 2, 3, 0};
	CreateIndexBuffer(m_IndexBuffer);
	m_IndexBuffer->InitMemory(IT_16, sizeof(indices) / sizeof(indices[0]), indices);
	m_IndexBuffer->InitDevice();

	VertexBindingDetail bindingDetail;
	bindingDetail.vertexBuffer = m_VertexBuffer;
	bindingDetail.formats.push_back(VF_POINT_NORMAL_UV);
	KVulkanHelper::PopulateInputBindingDescription(&bindingDetail, 1, m_VertexBindDetailList);

	return true;
}

bool KVulkanRenderDevice::CreateUniform()
{
	KConstantDefinition::ConstantBufferDetail detail = KConstantDefinition::GetConstantBufferDetail(CBT_TRANSFORM);

	m_UniformBuffers.clear();
	for(size_t i = 0; i < m_SwapChainImages.size(); ++i)
	{
		IKUniformBufferPtr buffer = nullptr;
		CreateUniformBuffer(buffer);
		m_UniformBuffers.push_back(buffer);

		buffer->InitMemory(detail.bufferSize, &KConstantGlobal::Transform);
		buffer->InitDevice();
	}
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

	// ����ʵ��
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
	// �ҽ���֤��
	if(m_EnableValidationLayer)
	{
		bool bCheckResult = CheckValidationLayerAvailable();
		assert(bCheckResult);
		createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
		createInfo.ppEnabledLayerNames = VALIDATION_LAYER;

		// ����Ϊ�˼��vkCreateInstance��SetupDebugMessenger֮��Ĵ���
		PopulateDebugMessengerCreateInfo(debugCreateInfo, DebugCallback);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// �ҽ���չ
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
		if(!CreateSwapChain())
			return false;
		if(!CreateImageViews())
			return false;
		if(!CreateCommandPool())
			return false;
		if(!CreateSyncObjects())
			return false;
		m_pWindow->SetVulkanDevice(this);
		// �����Ѿ�ʵ��������豸��ʼ��
		PostInit();

		// Temporarily for demo use
		if(!CreateVertexInput())
			return false;
		if(!CreateUniform())
			return false;
		if(!CreateRenderPass())
			return false;
		if(!CreateGraphicsPipeline())
			return false;
		if(!CreateFramebuffers())
			return false;
		if(!CreateCommandBuffers())
			return false;

		return true;
	}
	else
	{
		memset(&m_Instance, 0, sizeof(m_Instance));
	}
	return false;
}

/*
�ܹ���������ڿ�����鲢�ٷ����������ؽ�
1.glfw���ڴ�С�ı�
2.vkAcquireNextImageKHR
3.vkQueuePresentKHR
������ֻ��Ҫһ����ڳɹ�����Ĵ����˽�����֮��
vkAcquireNextImageKHR����vkQueuePresentKHR������鵽��������Ҫ���´���
*/
bool KVulkanRenderDevice::RecreateSwapChain()
{
	m_pWindow->IdleUntilForeground();
	vkDeviceWaitIdle(m_Device);
	// �ǵ�Ҫ���¸���SwapChainDetail
	SwapChainSupportDetails detail = QuerySwapChainSupport(m_PhysicalDevice.device);
	m_PhysicalDevice.swapChainSupportDetails = detail;

	CleanupSwapChain();
	/*
	�����FramesInFight�������뽻������������ϵ�
	�����������¹����ź�������դ
	*/
	DestroySyncObjects();

	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandBuffers();

	CreateSyncObjects();

	return true;
}

bool KVulkanRenderDevice::CleanupSwapChain()
{
	for (VkFramebuffer framebuffer : m_SwapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
	}
	m_SwapChainFramebuffers.clear();

	for (VkImageView imageView : m_SwapChainImageViews)
	{
		vkDestroyImageView(m_Device, imageView, nullptr);
	}
	m_SwapChainImageViews.clear();

	m_SwapChainImages.clear();

	vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	return true;
}

bool KVulkanRenderDevice::UnInit()
{
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
	for(IKUniformBufferPtr uniformBuffer : m_UniformBuffers)
	{
		uniformBuffer->UnInit();
	}
	m_UniformBuffers.clear();
	CleanupSwapChain();

	m_CommandBuffers.clear();

	DestroySyncObjects();

	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

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
			// ȷ��Vulkan����������Ҫ����չ
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

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// �������������ڻ���ʱ���ٷ����ź���
	VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFlightIndex]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// ָ�������
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

	// �������������ʱ���ٷ����ź���
	VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFlightIndex]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// vkResetFences���õ�vkQueueSubmit֮ǰ ��֤����vkWaitForFences��������������
	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFlightIndex]);
	// �ύ�û�������
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
	if (result != VK_SUCCESS)
	{
		return false;
	}
	return true;
}

bool KVulkanRenderDevice::Wait()
{
	vkDeviceWaitIdle(m_Device);
	return true;
}