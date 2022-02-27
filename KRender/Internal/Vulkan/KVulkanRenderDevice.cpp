#include "KVulkanRenderDevice.h"

#include "KVulkanShader.h"
#include "KVulkanBuffer.h"
#include "KVulkanAccelerationStructure.h"
#include "KVulkanRayTracePipeline.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanQuery.h"
#include "KVulkanRenderPass.h"
#include "KVulkanComputePipeline.h"

#include "KVulkanUIOverlay.h"
#include "KVulkanGlobal.h"
#include "KVulkanHeapAllocator.h"

#include "KBase/Interface/IKLog.h"

#if defined(_WIN32)
#include "GFSDK_Aftermath_GpuCrashDump.h"
#endif

#include <algorithm>
#include <set>
#include <functional>
#include <assert.h>

//-------------------- Extensions --------------------//
const char* DEVICE_DEFAULT_EXTENSIONS[] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const char* DEVICE_ADDRESS_EXTENSIONS[] =
{
	VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};

const char* DEVICE_NV_EXTENSIONS[] =
{
	// Required by Nvidia Aftermatch
	VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
	VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME
};

#ifndef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
const char* DEBUG_MARKER_EXTENSIONS[] =
{
	VK_EXT_DEBUG_MARKER_EXTENSION_NAME
};
#endif

const char* DEVICE_RAYTRACE_EXTENSIONS[] =
{
	// Required by VK_KHR_acceleration_structure
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	// Required by Ray Tracing
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_RAY_QUERY_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	// Required by VK_KHR_ray_tracing_pipeline
	VK_KHR_SPIRV_1_4_EXTENSION_NAME,
	// Required by relaxed block layout
	VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME,
	// Required by VK_KHR_spirv_1_4
	VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};

const char* DEVICE_MESHSHADER_EXTENSIONS[] =
{
	// VK_NV_GLSL_SHADER_EXTENSION_NAME,
	VK_NV_MESH_SHADER_EXTENSION_NAME,

	VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME
};

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
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = pCallBack;
	createInfo.pUserData = nullptr;
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
#if 0
		// 检查表现索引
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vkDevice, idx, m_Surface, &presentSupport);
		if(presentSupport)
		{
			familyIndices.presentFamily.first = idx;
			familyIndices.presentFamily.second = true;
		}
#else
		{
			familyIndices.presentFamily.first = idx;
			familyIndices.presentFamily.second = true;
		}
#endif
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
			for (const auto& ext : extensions)
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
			device.score += 5000;
		}

		// Support raytracing
		if (device.supportRaytraceExtension)
		{
			device.score += 500;
		}

		// Support meshshader
		if (device.supportMeshShaderExtension)
		{
			device.score += 50;
		}
	}
	else
	{
		device.score = -100;
	}

	return device;
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

	KG_LOGE(LM_RENDER, "Could not find available physics device for vulkan");
	return false;
}

void* KVulkanRenderDevice::GetEnabledFeatures()
{
	void* pNext = nullptr;

	// Enable features required for ray tracing using feature chaining via pNext
	static VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures = {};
	enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
	enabledBufferDeviceAddresFeatures.pNext = pNext;
	pNext = &enabledBufferDeviceAddresFeatures;

	static VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};
	physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
	physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
	physicalDeviceDescriptorIndexingFeatures.pNext = pNext;
	pNext = &physicalDeviceDescriptorIndexingFeatures;

	if (m_PhysicalDevice.supportRaytraceExtension)
	{
		static VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures = {};
		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		enabledRayTracingPipelineFeatures.pNext = pNext;
		pNext = &enabledRayTracingPipelineFeatures;

		static VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures = {};
		enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledAccelerationStructureFeatures.pNext = pNext;
		pNext = &enabledAccelerationStructureFeatures;

		static VkPhysicalDeviceRayQueryFeaturesKHR enabledRayqueryFeatures = {};
		enabledRayqueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		enabledRayqueryFeatures.rayQuery = VK_TRUE;
		enabledRayqueryFeatures.pNext = pNext;
		pNext = &enabledRayqueryFeatures;
	}

	if (m_PhysicalDevice.supportMeshShaderExtension)
	{
		static VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = {};
		meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
		meshFeatures.taskShader = VK_TRUE;
		meshFeatures.meshShader = VK_TRUE;
		meshFeatures.pNext = pNext;
		pNext = &meshFeatures;

		static VkPhysicalDeviceFloat16Int8FeaturesKHR float16int8Features = {};
		float16int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
		float16int8Features.shaderFloat16 = VK_TRUE;
		float16int8Features.shaderInt8 = VK_TRUE;
		float16int8Features.pNext = pNext;
		pNext = &float16int8Features;
	}

	return pNext;
}

bool KVulkanRenderDevice::CreateLogicalDevice()
{
	const QueueFamilyIndices& indices = m_PhysicalDevice.queueFamilyIndices;
	// TODO 用所有索引去创建队列 一定能够保证交换链获得需要的队列
	if(indices.IsComplete())
	{
		std::set<QueueFamilyIndices::QueueFamilyIndex> uniqueIndices;
		uniqueIndices.insert(indices.graphicsFamily);
		uniqueIndices.insert(indices.presentFamily);

		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;

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

			deviceQueueCreateInfos.push_back(queueCreateInfo);
		}
		VkResult RES = VK_TIMEOUT;

		VkPhysicalDeviceFeatures deviceFeatures = {};

		// 查询有哪些Features可以被打开
		// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceFeatures.html

		// Voxel Shader
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
		// RayTrace Shader引用DeviceAddress需要Int64
		deviceFeatures.shaderInt64 = VK_TRUE;
		// 即使没有使用MSAA的StorageImage 验证层还是会报错
		deviceFeatures.shaderStorageImageMultisample = VK_TRUE;
		// 查询各项异性支持
		if (m_PhysicalDevice.deviceProperties.limits.maxSamplerAnisotropy > 0.0f)
		{
			m_Properties.anisotropySupport = true;
			deviceFeatures.samplerAnisotropy = VK_TRUE;
		}
		else
		{
			m_Properties.anisotropySupport = false;
			deviceFeatures.samplerAnisotropy = VK_FALSE;
		}

		// 查询DDS支持
		{
			VkFormat bcFormats[] = 
			{
				VK_FORMAT_BC1_RGB_UNORM_BLOCK,
				VK_FORMAT_BC1_RGB_SRGB_BLOCK,
				VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
				VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
				VK_FORMAT_BC2_UNORM_BLOCK,
				VK_FORMAT_BC2_SRGB_BLOCK,
				VK_FORMAT_BC3_UNORM_BLOCK,
				VK_FORMAT_BC3_SRGB_BLOCK,
				VK_FORMAT_BC4_UNORM_BLOCK,
				VK_FORMAT_BC4_SNORM_BLOCK,
				VK_FORMAT_BC5_UNORM_BLOCK,
				VK_FORMAT_BC5_SNORM_BLOCK,
				VK_FORMAT_BC6H_UFLOAT_BLOCK,
				VK_FORMAT_BC6H_SFLOAT_BLOCK,
				VK_FORMAT_BC7_UNORM_BLOCK,
				VK_FORMAT_BC7_SRGB_BLOCK
			};

			bool ddsSupport = true;
			for (VkFormat format : bcFormats)
			{
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice.device, format, &props);
				if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
					!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
					!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
				{
					ddsSupport = false;
					break;
				}
			}

			if (ddsSupport)
			{
				m_Properties.bcSupport = true;
				deviceFeatures.textureCompressionBC = VK_TRUE;
			}
			else
			{
				m_Properties.bcSupport = false;
			}
		}

		// Query 查询ETC2支持
		{
			VkFormat etc2Formats[] =
			{
				VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
				VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,				
				VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
				VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
				VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
				VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
				VK_FORMAT_EAC_R11_UNORM_BLOCK,
				VK_FORMAT_EAC_R11_SNORM_BLOCK,
				VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
				VK_FORMAT_EAC_R11G11_SNORM_BLOCK
			};

			bool etc2Support = true;
			for (VkFormat format : etc2Formats)
			{
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice.device, format, &props);
				if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
					!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
					!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
				{
					etc2Support = false;
					break;
				}
			}

			if (etc2Support)
			{
				m_Properties.etc2Support = true;
				deviceFeatures.textureCompressionETC2 = VK_TRUE;
			}
			else
			{
				m_Properties.etc2Support = false;
			}
		}

		// 查询ASTC支持
		{
			VkFormat astcFormats[] =
			{
				VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
				VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
				VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
				VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
				VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
				VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
				VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
				VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
				VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
				VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
				VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
				VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
				VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
				VK_FORMAT_ASTC_12x12_SRGB_BLOCK
			};

			bool astcSupport = true;
			for (VkFormat format : astcFormats)
			{
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice.device, format, &props);
				if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
					!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
					!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
				{
					astcSupport = false;
					break;
				}
			}

			if (astcSupport)
			{
				m_Properties.astcSupport = true;
				deviceFeatures.textureCompressionASTC_LDR = VK_TRUE;
			}
			else
			{
				m_Properties.astcSupport = false;
			}
		}

		m_Properties.etc1Support = true;

		{
			KCodec::ETC1HardwareCodec = m_Properties.etc1Support;
			KCodec::ETC2HardwareCodec = m_Properties.etc2Support;
			KCodec::ASTCHardwareCodec = m_Properties.astcSupport;
			KCodec::BCHardwareCodec = m_Properties.bcSupport;
		}

		// 不用查询的东西
		{
			deviceFeatures.fillModeNonSolid = VK_TRUE;
			deviceFeatures.inheritedQueries = VK_TRUE;
		}

		// 记录其它设备属性
		{
			m_Properties.uniformBufferMaxRange = (size_t)m_PhysicalDevice.deviceProperties.limits.maxUniformBufferRange;
			m_Properties.uniformBufferOffsetAlignment = (size_t)m_PhysicalDevice.deviceProperties.limits.minUniformBufferOffsetAlignment;
			m_Properties.storageBufferOffsetAlignment = (size_t)m_PhysicalDevice.deviceProperties.limits.minStorageBufferOffsetAlignment;
		}

		// 组装支持扩展
		std::vector<const char*> extensionNames(DEVICE_DEFAULT_EXTENSIONS, DEVICE_DEFAULT_EXTENSIONS + ARRAY_SIZE(DEVICE_DEFAULT_EXTENSIONS));
		if (m_PhysicalDevice.supportNvExtension)
		{
			extensionNames.insert(extensionNames.end(), DEVICE_NV_EXTENSIONS, DEVICE_NV_EXTENSIONS + ARRAY_SIZE(DEVICE_NV_EXTENSIONS));
		}

		if (m_PhysicalDevice.supportRaytraceExtension)
		{
			extensionNames.insert(extensionNames.end(), DEVICE_RAYTRACE_EXTENSIONS, DEVICE_RAYTRACE_EXTENSIONS + ARRAY_SIZE(DEVICE_RAYTRACE_EXTENSIONS));
			m_Properties.raytraceSupport = true;
		}
		else
		{
			m_Properties.raytraceSupport = false;
		}

		if (m_PhysicalDevice.supportMeshShaderExtension)
		{
			extensionNames.insert(extensionNames.end(), DEVICE_MESHSHADER_EXTENSIONS, DEVICE_MESHSHADER_EXTENSIONS + ARRAY_SIZE(DEVICE_MESHSHADER_EXTENSIONS));
			m_Properties.meshShaderSupport = true;
		}
		else
		{
			m_Properties.meshShaderSupport = false;
		}

		if (m_PhysicalDevice.supportDebugMarker)
		{
#ifndef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
			extensionNames.insert(extensionNames.end(), DEBUG_MARKER_EXTENSIONS, DEBUG_MARKER_EXTENSIONS + ARRAY_SIZE(DEBUG_MARKER_EXTENSIONS));
#endif
			m_Properties.debugMarkerSupport = true;
		}
		else
		{
			m_Properties.debugMarkerSupport = false;
		}

		// 填充VkDeviceCreateInfo
		VkDeviceCreateInfo createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
		createInfo.queueCreateInfoCount = (uint32_t)deviceQueueCreateInfos.size();

		createInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
		createInfo.ppEnabledExtensionNames = extensionNames.data();
		createInfo.pEnabledFeatures = &deviceFeatures;

		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};

		void* pNextChain = GetEnabledFeatures();
		// If a pNext(Chain) has been passed, we need to add it to the device creation info
		if (pNextChain)
		{
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = deviceFeatures;
			physicalDeviceFeatures2.pNext = pNextChain;
			createInfo.pEnabledFeatures = nullptr;
			createInfo.pNext = &physicalDeviceFeatures2;
		}

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

	KG_LOGE(LM_RENDER, "Could not create logic device for vulkan");
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
		// KG_LOGW(LM_RENDER, "[Vulkan Validation Layer Performance] %s\n", pCallbackData->pMessage);
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

		KG_LOGE(LM_RENDER, "Could not setup debug messenger for vulkan");
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

#if defined(_WIN32)
static void OnAftermath_GpuCrashDumpCb(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
{
	FILE* fp = NULL;
	fopen_s(&fp, "crash.nv-gpudmp", "wb");
	if (fp)
	{
		fwrite(pGpuCrashDump, 1, gpuCrashDumpSize, fp);
		fclose(fp);
	}
}
#endif

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

#if defined(_WIN32)
	GFSDK_Aftermath_EnableGpuCrashDumps(GFSDK_Aftermath_Version_API,
		GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
		GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default,
		&OnAftermath_GpuCrashDumpCb, NULL, NULL, nullptr);
#endif

	VkApplicationInfo appInfo = {};

	// 描述实例
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "KVulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
	appInfo.pEngineName = "KVulkanEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

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

		CreateSwapChain(m_SwapChain);
		CreateUIOverlay(m_UIOverlay);

		for (KDeviceInitCallback* callback : m_InitCallback)
		{
			(*callback)();
		}

		if(!InitSwapChain())
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

bool KVulkanRenderDevice::InitSwapChain()
{
	ASSERT_RESULT(m_PhysicalDevice.queueFamilyIndices.IsComplete());
	ASSERT_RESULT(m_pWindow != nullptr);

	ASSERT_RESULT(m_SwapChain);
	ASSERT_RESULT(m_SwapChain->Init(m_pWindow, m_FrameInFlight));

	ASSERT_RESULT(m_UIOverlay);
	m_UIOverlay->Init(this, m_FrameInFlight);
	m_UIOverlay->Resize(m_SwapChain->GetWidth(), m_SwapChain->GetHeight());

	return true;
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

	for (KDeviceUnInitCallback* callback : m_UnInitCallback)
	{
		(*callback)();
	}

	for (IKSwapChain* swapChain : m_SecordarySwapChains)
	{
		SAFE_UNINIT(swapChain);
	}
	m_SecordarySwapChains.clear();

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
	// 确保Vulkan具有我们需要的必要扩展
	device.suitable = true;
	for (const char *requiredExt : DEVICE_DEFAULT_EXTENSIONS)
	{
		if(std::find(device.supportedExtensions.begin(), device.supportedExtensions.end(), requiredExt) == device.supportedExtensions.end())
		{
			device.suitable = false;
			break;
		}
	}

	device.supportNvExtension = true;
	for (const char* requiredExt : DEVICE_NV_EXTENSIONS)
	{
		if (std::find(device.supportedExtensions.begin(), device.supportedExtensions.end(), requiredExt) == device.supportedExtensions.end())
		{
			device.supportNvExtension = false;
			break;
		}
	}

	device.supportRaytraceExtension = true;
	for (const char* requiredExt : DEVICE_RAYTRACE_EXTENSIONS)
	{
		if (std::find(device.supportedExtensions.begin(), device.supportedExtensions.end(), requiredExt) == device.supportedExtensions.end())
		{
			device.supportRaytraceExtension = false;
			break;
		}
	}

	device.supportMeshShaderExtension = true;
	for (const char* requiredExt : DEVICE_MESHSHADER_EXTENSIONS)
	{
		if (std::find(device.supportedExtensions.begin(), device.supportedExtensions.end(), requiredExt) == device.supportedExtensions.end())
		{
			device.supportMeshShaderExtension = false;
			break;
		}
	}

	device.supportDebugMarker = true;
#ifndef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
	for (const char* requiredExt : DEBUG_MARKER_EXTENSIONS)
	{
		if (std::find(device.supportedExtensions.begin(), device.supportedExtensions.end(), requiredExt) == device.supportedExtensions.end())
		{
			device.supportDebugMarker = false;
			break;
		}
	}
#endif

	return true;
}

bool KVulkanRenderDevice::InitDeviceGlobal()
{
	KVulkanGlobal::device = m_Device;
	KVulkanGlobal::supportRaytrace = m_PhysicalDevice.supportRaytraceExtension;
	KVulkanGlobal::supportMeshShader = m_PhysicalDevice.supportMeshShaderExtension;
	KVulkanGlobal::supportDebugMarker = m_PhysicalDevice.supportDebugMarker;
	KVulkanGlobal::instance = m_Instance;
	KVulkanGlobal::physicalDevice = m_PhysicalDevice.device;
	KVulkanGlobal::deviceProperties = m_PhysicalDevice.deviceProperties;
	KVulkanGlobal::deviceFeatures = m_PhysicalDevice.deviceFeatures;

	KVulkanGlobal::graphicsCommandPool = m_GraphicCommandPool;
	KVulkanGlobal::graphicsQueue = m_GraphicsQueue;
	KVulkanGlobal::pipelineCache = m_PipelineCache;

	assert(m_PhysicalDevice.queueFamilyIndices.IsComplete());

	KVulkanGlobal::graphicsFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;

	// Get properties and features
	VkPhysicalDeviceProperties2 deviceProperties2 = {};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	KVulkanGlobal::rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	deviceProperties2.pNext = &KVulkanGlobal::rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(m_PhysicalDevice.device, &deviceProperties2);

	KVulkanGlobal::accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	deviceFeatures2.pNext = &KVulkanGlobal::accelerationStructureFeatures;
	vkGetPhysicalDeviceFeatures2(m_PhysicalDevice.device, &deviceFeatures2);

	KVulkanGlobal::meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
	deviceProperties2.pNext = &KVulkanGlobal::meshShaderFeatures;
	vkGetPhysicalDeviceProperties2(m_PhysicalDevice.device, &deviceProperties2);

	// Function pointers for ray tracing
	KVulkanGlobal::vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(m_Device, "vkGetBufferDeviceAddressKHR"));
	KVulkanGlobal::vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdBuildAccelerationStructuresKHR"));
	KVulkanGlobal::vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_Device, "vkBuildAccelerationStructuresKHR"));
	KVulkanGlobal::vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_Device, "vkCreateAccelerationStructureKHR"));
	KVulkanGlobal::vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_Device, "vkDestroyAccelerationStructureKHR"));
	KVulkanGlobal::vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_Device, "vkGetAccelerationStructureBuildSizesKHR"));
	KVulkanGlobal::vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_Device, "vkGetAccelerationStructureDeviceAddressKHR"));
	KVulkanGlobal::vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdTraceRaysKHR"));
	KVulkanGlobal::vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_Device, "vkGetRayTracingShaderGroupHandlesKHR"));
	KVulkanGlobal::vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_Device, "vkCreateRayTracingPipelinesKHR"));

	// Function pointers for mesh shader
	KVulkanGlobal::vkCmdDrawMeshTasksNV = reinterpret_cast<PFN_vkCmdDrawMeshTasksNV>(vkGetDeviceProcAddr(m_Device, "vkCmdDrawMeshTasksNV"));

	// Function pointers for debug marker
	KVulkanGlobal::vkDebugMarkerSetObjectTag = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(vkGetDeviceProcAddr(m_Device, "vkDebugMarkerSetObjectTagEXT"));
	KVulkanGlobal::vkDebugMarkerSetObjectName = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(vkGetDeviceProcAddr(m_Device, "vkDebugMarkerSetObjectNameEXT"));
	KVulkanGlobal::vkCmdDebugMarkerBegin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdDebugMarkerBeginEXT"));
	KVulkanGlobal::vkCmdDebugMarkerEnd = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdDebugMarkerEndEXT"));
	KVulkanGlobal::vkCmdDebugMarkerInsert = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdDebugMarkerInsertEXT"));

	// Function pointers for debug util
	KVulkanGlobal::vkSetDebugUtilsObjectTag = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(vkGetDeviceProcAddr(m_Device, "vkSetDebugUtilsObjectTagEXT"));
	KVulkanGlobal::vkSetDebugUtilsObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(m_Device, "vkSetDebugUtilsObjectNameEXT"));
	KVulkanGlobal::vkCmdBeginDebugUtilsLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdBeginDebugUtilsLabelEXT"));
	KVulkanGlobal::vkCmdEndDebugUtilsLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdEndDebugUtilsLabelEXT"));
	KVulkanGlobal::vkCmdInsertDebugUtilsLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdInsertDebugUtilsLabelEXT"));

	KVulkanGlobal::deviceReady = true;

	return true;
}

bool KVulkanRenderDevice::UnInitDeviceGlobal()
{
	KVulkanGlobal::deviceReady = false;
	KVulkanGlobal::supportRaytrace = false;
	KVulkanGlobal::supportMeshShader = false;
	KVulkanGlobal::supportDebugMarker = false;
	KVulkanGlobal::instance = VK_NULL_HANDLE;
	KVulkanGlobal::device = VK_NULL_HANDLE;
	KVulkanGlobal::physicalDevice = VK_NULL_HANDLE;
	KVulkanGlobal::graphicsCommandPool = VK_NULL_HANDLE;
	KVulkanGlobal::graphicsQueue = VK_NULL_HANDLE;
	KVulkanGlobal::pipelineCache = VK_NULL_HANDLE;

	KVulkanGlobal::vkGetBufferDeviceAddressKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkCreateAccelerationStructureKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkDestroyAccelerationStructureKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkGetAccelerationStructureBuildSizesKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkGetAccelerationStructureDeviceAddressKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkBuildAccelerationStructuresKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkCmdBuildAccelerationStructuresKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkCmdTraceRaysKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkGetRayTracingShaderGroupHandlesKHR = VK_NULL_HANDLE;
	KVulkanGlobal::vkCreateRayTracingPipelinesKHR = VK_NULL_HANDLE;

	KVulkanGlobal::graphicsFamilyIndex = 0;

	return true;
}

bool KVulkanRenderDevice::CreateShader(IKShaderPtr& shader)
{
	shader = IKShaderPtr(KNEW KVulkanShader());
	return true;
}

bool KVulkanRenderDevice::CreateVertexBuffer(IKVertexBufferPtr& buffer)
{
	buffer = IKVertexBufferPtr(static_cast<IKVertexBuffer*>(KNEW KVulkanVertexBuffer()));
	return true;
}

bool KVulkanRenderDevice::CreateIndexBuffer(IKIndexBufferPtr& buffer)
{
	buffer = IKIndexBufferPtr(static_cast<IKIndexBuffer*>(KNEW KVulkanIndexBuffer()));
	return true;
}

bool KVulkanRenderDevice::CreateStorageBuffer(IKStorageBufferPtr& buffer)
{
	buffer = IKStorageBufferPtr(static_cast<IKStorageBuffer*>(KNEW KVulkanStorageBuffer()));
	return true;
}

bool KVulkanRenderDevice::CreateUniformBuffer(IKUniformBufferPtr& buffer)
{
	buffer = IKUniformBufferPtr(static_cast<IKUniformBuffer*>(KNEW KVulkanUniformBuffer()));
	return true;
}

bool KVulkanRenderDevice::CreateAccelerationStructure(IKAccelerationStructurePtr& as)
{
	as = IKAccelerationStructurePtr(static_cast<IKAccelerationStructure*>(KNEW KVulkanAccelerationStructure()));
	return true;
}

bool KVulkanRenderDevice::CreateTexture(IKTexturePtr& texture)
{
	texture = IKTexturePtr(static_cast<IKTexture*>(KNEW KVulkanTexture()));
	return true;
}

bool KVulkanRenderDevice::CreateSampler(IKSamplerPtr& sampler)
{
	sampler = IKSamplerPtr(static_cast<IKSampler*>(KNEW KVulkanSampler()));
	return true;
}

bool KVulkanRenderDevice::CreateSwapChain(IKSwapChainPtr& swapChain)
{
	swapChain = IKSwapChainPtr(static_cast<IKSwapChain*>(KNEW KVulkanSwapChain()));
	return true;
}

bool KVulkanRenderDevice::CreateRenderPass(IKRenderPassPtr& renderPass)
{
	renderPass = IKRenderPassPtr(static_cast<IKRenderPass*>(KNEW KVulkanRenderPass()));
	return true;
}

bool KVulkanRenderDevice::CreateRenderTarget(IKRenderTargetPtr& target)
{
	target = IKRenderTargetPtr(KNEW KVulkanRenderTarget());
	return true;
}

bool KVulkanRenderDevice::CreatePipeline(IKPipelinePtr& pipeline)
{
	pipeline = IKPipelinePtr(KNEW KVulkanPipeline());
	return true;
}

bool KVulkanRenderDevice::CreateRayTracePipeline(IKRayTracePipelinePtr& raytrace)
{
	raytrace = IKRayTracePipelinePtr(KNEW KVulkanRayTracePipeline());
	return true;
}

bool KVulkanRenderDevice::CreateComputePipeline(IKComputePipelinePtr& compute)
{
	compute = IKComputePipelinePtr(KNEW KVulkanComputePipeline());
	return true;
}

bool KVulkanRenderDevice::CreateUIOverlay(IKUIOverlayPtr& ui)
{
	ui = IKUIOverlayPtr(static_cast<IKUIOverlay*>(KNEW KVulkanUIOverlay()));
	return true;
}

bool KVulkanRenderDevice::Present()
{
	VkResult vkResult = VK_RESULT_MAX_ENUM;

	uint32_t frameIndex = 0;
	vkResult = ((KVulkanSwapChain*)m_SwapChain.get())->WaitForInFightFrame(frameIndex);
	VK_ASSERT_RESULT(vkResult);

	uint32_t chainImageIndex = 0;
	vkResult = ((KVulkanSwapChain*)m_SwapChain.get())->AcquireNextImage(chainImageIndex);

	KRenderGlobal::RenderDispatcher.Update(frameIndex);

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain(m_SwapChain.get(), m_UIOverlay.get());
	}
	else if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
	{
		// FATAL ERROR
		return false;
	}
	else
	{
		KRenderGlobal::RenderDispatcher.SetSwapChain(m_SwapChain.get(), m_UIOverlay.get());

		for (KDevicePresentCallback* callback : m_PrePresentCallback)
		{
			(*callback)(chainImageIndex, frameIndex);
		}

		KRenderGlobal::RenderDispatcher.Execute(chainImageIndex, frameIndex);
		VkCommandBuffer primaryCommandBuffer = ((KVulkanCommandBuffer*)KRenderGlobal::RenderDispatcher.GetPrimaryCommandBuffer(frameIndex).get())->GetVkHandle();
		vkResult = ((KVulkanSwapChain*)m_SwapChain.get())->PresentQueue(chainImageIndex, primaryCommandBuffer);

		for (KDevicePresentCallback* callback : m_PostPresentCallback)
		{
			(*callback)(chainImageIndex, frameIndex);
		}

		if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapChain(m_SwapChain.get(), m_UIOverlay.get());
		}
	}

	if (!m_SecordarySwapChains.empty())
	{
		// 等待设置空闲 主交换链不再进行FrameInFlight
		vkDeviceWaitIdle(m_Device);
	}

	for (IKSwapChain* secordarySwapChain : m_SecordarySwapChains)
	{
		// 处理次交换链的FrameInFlight 但是FrameIndex依然使用主交换链的结果
		uint32_t secordaryFrameIndex = 0;
		((KVulkanSwapChain*)secordarySwapChain)->WaitForInFightFrame(secordaryFrameIndex);

		vkResult = ((KVulkanSwapChain*)secordarySwapChain)->AcquireNextImage(chainImageIndex);
		if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain(secordarySwapChain, nullptr);
		}
		else if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
		{
			// FATAL ERROR
			continue;
		}
		else
		{
			KRenderGlobal::RenderDispatcher.SetSwapChain(secordarySwapChain, nullptr);
		
			KRenderGlobal::RenderDispatcher.Execute(chainImageIndex, frameIndex);
			VkCommandBuffer primaryCommandBuffer = ((KVulkanCommandBuffer*)KRenderGlobal::RenderDispatcher.GetPrimaryCommandBuffer(frameIndex).get())->GetVkHandle();
			vkResult = ((KVulkanSwapChain*)secordarySwapChain)->PresentQueue(chainImageIndex, primaryCommandBuffer);

			if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR)
			{
				RecreateSwapChain(secordarySwapChain, nullptr);
			}
		}

		// 等待设置空闲 次交换链不再进行FrameInFlight
		vkDeviceWaitIdle(m_Device);
	}

	return true;
}

bool KVulkanRenderDevice::RegisterSecordarySwapChain(IKSwapChain* swapChain)
{
	if (std::find(m_SecordarySwapChains.begin(), m_SecordarySwapChains.end(), swapChain) == m_SecordarySwapChains.end())
	{
		m_SecordarySwapChains.push_back(swapChain);
	}
	return true;
}

bool KVulkanRenderDevice::UnRegisterSecordarySwapChain(IKSwapChain* swapChain)
{
	auto it = std::find(m_SecordarySwapChains.begin(), m_SecordarySwapChains.end(), swapChain);
	if (it != m_SecordarySwapChains.end())
	{
		m_SecordarySwapChains.erase(it);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::QueryProperty(KRenderDeviceProperties** ppProperty)
{
	if (ppProperty)
	{
		*ppProperty = &m_Properties;
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::CreateCommandPool(IKCommandPoolPtr& pool)
{
	pool = IKCommandPoolPtr(KNEW KVulkanCommandPool());
	return true;
}

bool KVulkanRenderDevice::CreateCommandBuffer(IKCommandBufferPtr& buffer)
{
	buffer = IKCommandBufferPtr(KNEW KVulkanCommandBuffer());
	return true;
}

bool KVulkanRenderDevice::CreateQuery(IKQueryPtr& query)
{
	query = IKQueryPtr(KNEW KVulkanQuery());
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

bool KVulkanRenderDevice::RegisterPrePresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		m_PrePresentCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::UnRegisterPrePresentCallback(KDevicePresentCallback* callback)
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

bool KVulkanRenderDevice::RegisterPostPresentCallback(KDevicePresentCallback* callback)
{
	if (callback)
	{
		m_PostPresentCallback.insert(callback);
		return true;
	}
	return false;
}

bool KVulkanRenderDevice::UnRegisterPostPresentCallback(KDevicePresentCallback* callback)
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

IKSwapChain* KVulkanRenderDevice::GetSwapChain()
{
	return m_SwapChain.get();
}

IKRenderWindow* KVulkanRenderDevice::GetMainWindow()
{
	return m_pWindow;
}

IKUIOverlay* KVulkanRenderDevice::GetUIOverlay()
{
	return m_UIOverlay.get();
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
bool KVulkanRenderDevice::RecreateSwapChain(IKSwapChain* swapChain, IKUIOverlay* ui)
{
	if (swapChain)
	{
		IKRenderWindow* window = swapChain->GetWindow();
		uint32_t frameInFlight = swapChain->GetFrameInFlight();

		// 主交换链必须等到撤销最小化
		if (swapChain == m_SwapChain.get())
		{
			window->IdleUntilForeground();
		}
		else
		{
			size_t width = 0, height = 0;
			window->GetSize(width, height);
			if (width == 0 && height == 0)
			{
				return true;
			}
		}

		vkDeviceWaitIdle(m_Device);

		swapChain->UnInit();
		swapChain->Init(window, frameInFlight);

		if (ui)
		{
			ui->UnInit();
			ui->Init(this, frameInFlight);
			ui->Resize(swapChain->GetWidth(), swapChain->GetHeight());
		}

		// 只对主交换链促发回调
		if (swapChain == m_SwapChain.get())
		{
			for (KSwapChainRecreateCallback* callback : m_SwapChainCallback)
			{
				(*callback)((uint32_t)m_SwapChain->GetWidth(), (uint32_t)m_SwapChain->GetHeight());
			}
		}

		return true;
	}
	return false;
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
#ifndef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
	if (m_EnableValidationLayer)
#endif
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	return true;
#elif defined(__ANDROID__)
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#ifdef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	if (m_EnableValidationLayer)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return true;
#else
	return false;
#endif
}