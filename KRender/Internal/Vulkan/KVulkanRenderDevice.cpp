#include "KVulkanRenderDevice.h"

#include "KVulkanShader.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"
#include "KVulkanCommandBuffer.h"

#include "KVulkanUIOverlay.h"

#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"

#include "KVulkanHeapAllocator.h"

#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

#include "Internal/ECS/KECSGlobal.h"

#include "KBase/Interface/IKLog.h"

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

//-------------------- Validation Layer --------------------//
const char* KHRONOS_VALIDATION_LAYERS[] =
{
	"VK_LAYER_KHRONOS_validation",
};

const char* LUNARG_GOOGLE_VALIDATION_LAYERS[] =
{
	"VK_LAYER_LUNARG_core_validation",
	"VK_LAYER_LUNARG_object_tracker",
	"VK_LAYER_LUNARG_parameter_validation",
	"VK_LAYER_GOOGLE_threading",
	"VK_LAYER_GOOGLE_unique_objects"
};

struct ValidationLayerCandidate
{
	const char** layers;
	uint32_t arraySize;
};

ValidationLayerCandidate VALIDATION_LAYER_CANDIDATE[] =
{
	{KHRONOS_VALIDATION_LAYERS, ARRAY_SIZE(KHRONOS_VALIDATION_LAYERS)},
	{LUNARG_GOOGLE_VALIDATION_LAYERS, ARRAY_SIZE(LUNARG_GOOGLE_VALIDATION_LAYERS)},
};

static void PopulateDebugUtilsMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, PFN_vkDebugUtilsMessengerCallbackEXT pCallBack)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = pCallBack;
}

static VkResult CreateDebugUtilsMessengerEXT(
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

static void DestroyDebugUtilsMessengerEXT(
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

static void PopulateDebugReportCallbackCreateInfo(VkDebugReportCallbackCreateInfoEXT& createInfo, PFN_vkDebugReportCallbackEXT pCallBack)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	createInfo.flags =  VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	createInfo.pfnCallback = pCallBack;
}

static VkResult CreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* debugReport)
{

	PFN_vkCreateDebugReportCallbackEXT func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, debugReport);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void DestroyDebugReportCallbackEXT(
	VkInstance instance,
	VkDebugReportCallbackEXT debugReport,
	const VkAllocationCallbacks* pAllocator)
{
	PFN_vkDestroyDebugReportCallbackEXT func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, debugReport, pAllocator);
	}
}

/* TODO LIST
材质文件
场景文件
深度排序
场景渲染框架
*/

/* BUG LIST
PC 阴影闪烁
Android 镜头翻转
*/

//-------------------- KVulkanRenderDevice --------------------//
KVulkanRenderDevice::KVulkanRenderDevice()
	: m_pWindow(nullptr),
	m_EnableValidationLayer(
#if defined(_WIN32) && defined(_DEBUG)
	true
#elif defined(__ANDROID__)
	false
#else
	false
#endif
	),
	m_ValidationLayerIdx(-1),
	m_FrameInFlight(2)
{
}

KVulkanRenderDevice::~KVulkanRenderDevice()
{

}

bool KVulkanRenderDevice::CheckValidationLayerAvailable(int32_t& candidateIdx)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	candidateIdx = -1;

	for(uint32_t i = 0; i < ARRAY_SIZE(VALIDATION_LAYER_CANDIDATE); ++i)
	{
		const ValidationLayerCandidate& candidate = VALIDATION_LAYER_CANDIDATE[i];
		uint32_t j = 0;
		for(; j < candidate.arraySize; ++j)
		{
			const char* layerName = candidate.layers[j];

			if(std::find_if(
				availableLayers.begin(),
				availableLayers.end(),
				[layerName](const VkLayerProperties& prop) { return strcmp(layerName, prop.layerName) == 0; }) == availableLayers.end())
			{
				break;
			}
		}

		if(j == candidate.arraySize)
		{
			candidateIdx = i;
			return true;
		}
	}
	return false;
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

	uint32_t idx = -1;
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

	if(!CheckExtentionsSupported(device))
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

	// Get list of supported extensions
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(vkDevice, nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateDeviceExtensionProperties(vkDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
		{
			for (auto ext : extensions)
			{
				device.supportedExtensions.push_back(ext.extensionName);
			}
		}
	}

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

bool KVulkanRenderDevice::CreateSwapChain()
{
	ASSERT_RESULT(m_PhysicalDevice.queueFamilyIndices.IsComplete());
	ASSERT_RESULT(m_pWindow != nullptr);

	size_t windowWidth = 0, windowHeight= 0;
	ASSERT_RESULT(m_pWindow->GetSize(windowWidth, windowHeight));

	ASSERT_RESULT(m_SwapChain == nullptr);
	CreateSwapChain(m_SwapChain);

	ASSERT_RESULT(m_SwapChain->Init((uint32_t)windowWidth, (uint32_t)windowHeight, m_FrameInFlight));
	return true;
}

bool KVulkanRenderDevice::CreateUI()
{
	CreateUIOverlay(m_UIOverlay);
	m_UIOverlay->Init(this, m_FrameInFlight);
	m_UIOverlay->Resize(m_SwapChain->GetWidth(), m_SwapChain->GetHeight());
	return true;
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

		// TODO 检测
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		createInfo.pEnabledFeatures = &deviceFeatures;

		// 尽管最新Vulkan实例验证层与设备验证层已经统一
		// 但是最好保留代码兼容性
		if (m_EnableValidationLayer && m_ValidationLayerIdx >= 0)
		{
			createInfo.enabledLayerCount = VALIDATION_LAYER_CANDIDATE[m_ValidationLayerIdx].arraySize;
			createInfo.ppEnabledLayerNames = VALIDATION_LAYER_CANDIDATE[m_ValidationLayerIdx].layers;
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

bool KVulkanRenderDevice::CreatePipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_ASSERT_RESULT(vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	return true;
}

bool KVulkanRenderDevice::CreateCommandPool()
{
	assert(m_PhysicalDevice.queueFamilyIndices.IsComplete());

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// 指定该命令池所属的队列家族
	poolInfo.queueFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;
	// 为Transient所用
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_GraphicCommandPool) == VK_SUCCESS)
	{
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::CreateMesh()
{
	const char* szPaths[] =
	{
		"Model/OBJ/spider.obj",
		"../Sponza/sponza.obj"
	};

	const char* destPaths[] =
	{
		"../Dependency/assimp-3.3.1/test/models/OBJ/spider.mesh",
		"../Sponza/sponza.mesh"
	};

#if 1
#ifdef _DEBUG
	int width = 1, height = 1;
#else
	int width = 10, height = 10;
#endif
	int widthExtend = width * 8, heightExtend = height * 8;
	for(int i = 0; i < width; ++i)
	{
		for(int j = 0; j < height; ++j)
		{
			KEntityPtr entity = KECSGlobal::EntityManager.CreateEntity();

			KComponentBase* component = nullptr;
			if (entity->RegisterComponent(CT_RENDER, &component))
			{
				((KRenderComponent*)component)->InitFromAsset(szPaths[0]);
			}

			if (entity->RegisterComponent(CT_TRANSFORM, &component))
			{
				glm::vec3 pos = ((KTransformComponent*)component)->GetPosition();
				pos.x = (float)(i * 2 - width) / width * widthExtend;
				pos.z = (float)(j * 2 - height) / height * heightExtend;
				pos.y = 0;
				((KTransformComponent*)component)->SetPosition(pos);

				glm::vec3 scale = ((KTransformComponent*)component)->GetScale();
				scale = glm::vec3(0.1f, 0.1f, 0.1f);
				((KTransformComponent*)component)->SetScale(scale);
			}

			KRenderGlobal::Scene.Add(entity);
		}
	}
#endif
#if 1
	const uint32_t IDX = 1;

	enum InitMode
	{
		IM_EXPORT,
		IM_IMPORT,
		IM_IMPORT_ZIP,
	}mode = IM_IMPORT_ZIP;

	for(size_t i = 0; i < 1; ++i)
	{
		KEntityPtr entity = KECSGlobal::EntityManager.CreateEntity();

		KComponentBase* component = nullptr;
		if(entity->RegisterComponent(CT_RENDER, &component))
		{
#if 1
			if(mode == IM_EXPORT)
			{
				((KRenderComponent*)component)->InitFromAsset(szPaths[IDX]);

				KMeshPtr mesh = ((KRenderComponent*)component)->GetMesh();
				if(mesh && mesh->SaveAsFile(destPaths[IDX]))
				{
					((KRenderComponent*)component)->Init(destPaths[IDX]);
				}
			}
			else if(mode == IM_IMPORT)
			{
				((KRenderComponent*)component)->Init(destPaths[IDX]);
			}
			else if(mode == IM_IMPORT_ZIP)
			{
				((KRenderComponent*)component)->Init("Sponza/sponza.mesh");
			}
#else
			((KRenderComponent*)component)->InitFromAsset(szPaths[IDX]);
			//((KRenderComponent*)component)->Init(destPaths[IDX]);
#endif
		}
		entity->RegisterComponent(CT_TRANSFORM);

		KRenderGlobal::Scene.Add(entity);
	}
#endif
	return true;
}

VkBool32 KVulkanRenderDevice::DebugUtilsMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
	)
{
	if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	{
		KG_LOG(LM_RENDER, "[Vulkan Validation Layer Debug] %s\n", pCallbackData->pMessage);
	}
	if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
	{
		KG_LOGW(LM_RENDER, "[Vulkan Validation Layer Performance] %s\n", pCallbackData->pMessage);
	}
	else if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{
		KG_LOGE_ASSERT(LM_RENDER, "[Vulkan Validation Layer Error] %s\n", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

VkBool32 KVulkanRenderDevice::DebugReportCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char *pLayerPrefix,
	const char *pMessage,
	void *pUserData
	)
{
	if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		KG_LOG(LM_RENDER, "[Vulkan Validation Layer Info] %s\n", pMessage);
	}
	if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		KG_LOGD(LM_RENDER, "[Vulkan Validation Layer Debug] %s\n", pMessage);
	}
	if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		KG_LOGW(LM_RENDER, "[Vulkan Validation Layer Performance] %s\n", pMessage);
	}
	if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		KG_LOGW(LM_RENDER, "[Vulkan Validation Layer Warning] %s\n", pMessage);
	}
	if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		KG_LOGE_ASSERT(LM_RENDER, "[Vulkan Validation Layer Error] %s\n", pMessage);
	}

	return VK_FALSE;
}

bool KVulkanRenderDevice::SetupDebugMessenger()
{
	if(m_EnableValidationLayer)
	{
#ifndef __ANDROID__
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		PopulateDebugUtilsMessengerCreateInfo(createInfo, DebugUtilsMessengerCallback);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugUtilsMessenger) == VK_SUCCESS)
		{
			return true;
		}
#else
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		PopulateDebugReportCallbackCreateInfo(createInfo, DebugReportCallback);

		if (CreateDebugReportCallbackEXT(m_Instance, &createInfo, nullptr, &m_DebugReportCallback) == VK_SUCCESS)
		{
			return true;
		}
#endif
		return false;
	}
	return true;
}

bool KVulkanRenderDevice::UnsetDebugMessenger()
{
	if(m_EnableValidationLayer)
	{
#ifndef __ANDROID__
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugUtilsMessenger, nullptr);
#else
		DestroyDebugReportCallbackEXT(m_Instance, m_DebugReportCallback, nullptr);
#endif
	}
	return true;
}

bool KVulkanRenderDevice::InitHeapAllocator()
{
	KVulkanHeapAllocator::Init();
	return true;
}

bool KVulkanRenderDevice::UnInitHeapAllocator()
{
	KVulkanHeapAllocator::UnInit();
	return true;
}

bool KVulkanRenderDevice::Init(IKRenderWindow* window)
{
	if(window == nullptr
#if defined(_WIN32)
		|| window->GetHWND() == nullptr
#elif defined(__ANDROID__)
		|| window->GetAndroidApp() == nullptr
#endif
		)
	{
		return false;
	}

	m_pWindow = window;

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
	m_ValidationLayerIdx = -1;
	// 挂接验证层
	if(m_EnableValidationLayer)
	{
		bool bCheckResult = CheckValidationLayerAvailable(m_ValidationLayerIdx);
		assert(bCheckResult && "try to find validation layer but fail");
		if(m_ValidationLayerIdx >= 0)
		{
			createInfo.enabledLayerCount = VALIDATION_LAYER_CANDIDATE[m_ValidationLayerIdx].arraySize;
			createInfo.ppEnabledLayerNames = VALIDATION_LAYER_CANDIDATE[m_ValidationLayerIdx].layers;
			// 这是为了检查vkCreateInstance与SetupDebugMessenger之间的错误
#ifndef __ANDROID__	
			PopulateDebugUtilsMessengerCreateInfo(debugCreateInfo, DebugUtilsMessengerCallback);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else

#endif
			for(uint32_t i = 0; i < createInfo.enabledLayerCount; ++i)
			{
				KG_LOG(LM_RENDER, "Vulkan validation layer picked [%s]\n", createInfo.ppEnabledLayerNames[i]);
			}
		}
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// 挂接扩展
	std::vector<const char*> extensions;
	ASSERT_RESULT(PopulateInstanceExtensions(extensions));
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
		if(!CreatePipelineCache())
			return false;
		if(!CreateCommandPool())
			return false;

		// 实际完成了设备初始化
		if(!InitDeviceGlobal())
			return false;

		if(!InitHeapAllocator())
			return false;
		if(!CreateSwapChain())
			return false;
		if(!CreateUI())
			return false;

		for (KDeviceInitCallback* callback : m_InitCallback)
		{
			(*callback)();
		}

		// Temporarily for demo use
		KRenderGlobal::Scene.Init(SCENE_MANGER_TYPE_OCTREE, 2000.0f, glm::vec3(0.0f));
		if(!CreateMesh())
			return false;

		m_pWindow->SetRenderDevice(this);
		return true;
	}
	else
	{
		memset(&m_Instance, 0, sizeof(m_Instance));
	}
	return false;
}

bool KVulkanRenderDevice::CleanupSwapChain()
{
	if(m_UIOverlay)
	{
		m_UIOverlay->UnInit();
		m_UIOverlay = nullptr;
	}

	// clear swapchain
	if (m_SwapChain)
	{
		m_SwapChain->UnInit();
		m_SwapChain = nullptr;
	}

	return true;
}

bool KVulkanRenderDevice::UnInit()
{
	Wait();

	KRenderGlobal::Scene.UnInit();

	KECSGlobal::EntityManager.ViewAllEntity([](KEntityPtr entity)
	{
		KRenderComponent* component = nullptr;
		if(entity->GetComponent(CT_RENDER, (KComponentBase**)&component))
		{
			KMeshPtr mesh = component->GetMesh();
			if(mesh)
			{
				component->UnInit();
			}
		}
	});

	CleanupSwapChain();

	if (m_PipelineCache != VK_NULL_HANDLE)
	{
		vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);
		m_PipelineCache = VK_NULL_HANDLE;
	}

	if (m_GraphicCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_GraphicCommandPool, nullptr);
		m_GraphicCommandPool = VK_NULL_HANDLE;
	}

	if (m_Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
	}

	for (KDeviceUnInitCallback* callback : m_UnInitCallback)
	{
		(*callback)();
	}

	UnInitHeapAllocator();

	UnsetDebugMessenger();

	m_pWindow = nullptr;

	if (m_Device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	if (m_Instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;
	}

	UnInitDeviceGlobal();
	return true;
}

bool KVulkanRenderDevice::CheckExtentionsSupported(PhysicalDevice& device)
{
	uint32_t extensionCount = 0;
	// 确保Vulkan具有我们需要的扩展
	for (const char *requiredExt : DEVICE_EXTENSIONS)
	{
		if(std::find(device.supportedExtensions.begin(), device.supportedExtensions.end(), requiredExt) == device.supportedExtensions.end())
		{
			return false;
		}
	}
	return true;
}

bool KVulkanRenderDevice::InitDeviceGlobal()
{
	KVulkanGlobal::device = m_Device;
	KVulkanGlobal::physicalDevice = m_PhysicalDevice.device;
	KVulkanGlobal::surface = m_Surface;
	KVulkanGlobal::graphicsCommandPool = m_GraphicCommandPool;
	KVulkanGlobal::graphicsQueue = m_GraphicsQueue;
	KVulkanGlobal::pipelineCache = m_PipelineCache;

	assert(m_PhysicalDevice.queueFamilyIndices.IsComplete());

	KVulkanGlobal::graphicsFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;
	KVulkanGlobal::presentFamilyIndex = m_PhysicalDevice.queueFamilyIndices.presentFamily.first;

	KVulkanGlobal::deviceReady = true;

	return true;
}

bool KVulkanRenderDevice::UnInitDeviceGlobal()
{
	KVulkanGlobal::deviceReady = false;

	KVulkanGlobal::device = VK_NULL_HANDLE;
	KVulkanGlobal::physicalDevice = VK_NULL_HANDLE;
	KVulkanGlobal::surface = VK_NULL_HANDLE;
	KVulkanGlobal::graphicsCommandPool = VK_NULL_HANDLE;
	KVulkanGlobal::graphicsQueue = VK_NULL_HANDLE;
	KVulkanGlobal::pipelineCache = VK_NULL_HANDLE;

	KVulkanGlobal::graphicsFamilyIndex = 0;
	KVulkanGlobal::presentFamilyIndex = 0;

	return true;
}

bool KVulkanRenderDevice::CreateShader(IKShaderPtr& shader)
{
	shader = IKShaderPtr(new KVulkanShader());
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

bool KVulkanRenderDevice::CreateSwapChain(IKSwapChainPtr& swapChain)
{
	swapChain = IKSwapChainPtr(static_cast<IKSwapChain*>(new KVulkanSwapChain()));
	return true;
}

bool KVulkanRenderDevice::CreateRenderTarget(IKRenderTargetPtr& target)
{
	target = KVulkanRenderTarget::CreateRenderTarget();
	return true;
}

bool KVulkanRenderDevice::CreatePipeline(IKPipelinePtr& pipeline)
{
	pipeline = KVulkanPipeline::CreatePipeline();
	return true;
}

bool KVulkanRenderDevice::CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle)
{
	pipelineHandle = IKPipelineHandlePtr(static_cast<IKPipelineHandle*>(new KVulkanPipelineHandle()));
	return true;
}

bool KVulkanRenderDevice::CreateUIOverlay(IKUIOverlayPtr& ui)
{
	ui = IKUIOverlayPtr(static_cast<IKUIOverlay*>(new KVulkanUIOverlay()));
	return true;
}

bool KVulkanRenderDevice::Present()
{
	VkResult vkResult;

	uint32_t frameIndex = 0;
	vkResult = ((KVulkanSwapChain*)m_SwapChain.get())->WaitForInfightFrame(frameIndex);
	VK_ASSERT_RESULT(vkResult);

	uint32_t chainImageIndex = 0;
	vkResult = ((KVulkanSwapChain*)m_SwapChain.get())->AcquireNextImage(chainImageIndex);

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return true;
	}
	else if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
	{
		return false;
	}

	for (KDevicePresentCallback* callback : m_PresentCallback)
	{
		(*callback)(chainImageIndex, frameIndex);
	}

	VkCommandBuffer primaryCommandBuffer = ((KVulkanCommandBuffer*)KRenderGlobal::RenderDispatcher.GetPrimaryCommandBuffer(frameIndex).get())->GetVkHandle();
	vkResult = ((KVulkanSwapChain*)m_SwapChain.get())->PresentQueue(m_GraphicsQueue, m_PresentQueue, chainImageIndex, primaryCommandBuffer);
	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
	}
	return true;
}

bool KVulkanRenderDevice::CreateCommandPool(IKCommandPoolPtr& pool)
{
	pool = IKCommandPoolPtr(new KVulkanCommandPool());
	return true;
}

bool KVulkanRenderDevice::CreateCommandBuffer(IKCommandBufferPtr& buffer)
{
	buffer = IKCommandBufferPtr(new KVulkanCommandBuffer());
	return true;
}

bool KVulkanRenderDevice::Wait()
{
	if(m_Device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(m_Device);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::RegisterPresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		m_PresentCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::UnRegisterPresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		auto it = m_PresentCallback.find(callback);
		if (it != m_PresentCallback.end())
		{
			m_PresentCallback.erase(it);
			return true;
		}
	}
	return false;
}

bool KVulkanRenderDevice::RegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback)
{
	if (callback)
	{
		m_SwapChainCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::UnRegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback)
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

bool KVulkanRenderDevice::RegisterDeviceInitCallback(KDeviceInitCallback* callback)
{
	if (callback)
	{
		m_InitCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::UnRegisterDeviceInitCallback(KDeviceInitCallback* callback)
{
	if (callback)
	{
		auto it = m_InitCallback.find(callback);
		if (it != m_InitCallback.end())
		{
			m_InitCallback.erase(it);
			return true;
		}
	}
	return false;
}

bool KVulkanRenderDevice::RegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback)
{
	if (callback)
	{
		m_UnInitCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::UnRegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback)
{
	if (callback)
	{
		auto it = m_UnInitCallback.find(callback);
		if (it != m_UnInitCallback.end())
		{
			m_UnInitCallback.erase(it);
			return true;
		}
	}
	return false;
}

IKSwapChainPtr KVulkanRenderDevice::GetSwapChain()
{
	return m_SwapChain;
}

IKUIOverlayPtr KVulkanRenderDevice::GetUIOverlay()
{
	return m_UIOverlay;
}

uint32_t KVulkanRenderDevice::GetNumFramesInFlight()
{
	return m_FrameInFlight;
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

	size_t width = 0, height = 0;
	m_pWindow->GetSize(width, height);

	m_SwapChain->UnInit();
	m_SwapChain->Init((uint32_t)width, (uint32_t)height, m_FrameInFlight);

	m_UIOverlay->UnInit();
	m_UIOverlay->Init(this, m_FrameInFlight);

	m_UIOverlay->Resize(m_SwapChain->GetWidth(), m_SwapChain->GetHeight());

	for (KSwapChainRecreateCallback* callback : m_SwapChainCallback)
	{
		(*callback)((uint32_t)width, (uint32_t)height);
	}

	return true;
}

// 平台相关的脏东西放到最下面
#if defined(_WIN32)
#	pragma warning (disable : 4005)
#	include <Windows.h>
#	include "vulkan/vulkan_win32.h"
#elif defined(__ANDROID__)
#	include "vulkan/vulkan_android.h"
#	include "android_native_app_glue.h"
#endif

bool KVulkanRenderDevice::PopulateInstanceExtensions(std::vector<const char*>& extensions)
{
	extensions.clear();
#if defined(_WIN32)
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	if (m_EnableValidationLayer)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return true;
#elif defined(__ANDROID__)
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
	if (m_EnableValidationLayer)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return true;
#else
	return false;
#endif	
}

bool KVulkanRenderDevice::CreateSurface()
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
		if (vkCreateWin32SurfaceKHR(m_Instance, &surfaceCreateInfo, nullptr, &m_Surface) == VK_SUCCESS)
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