#include "KVulkanRenderDevice.h"
#include "KVulkanRenderWindow.h"

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
#include "Internal/ECS/System/KCullSystem.h"

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
const char* KHRONOS_VALIDATION_LAYERS[] =
{
	"VK_LAYER_KHRONOS_validation",
};

const char* LUNARG_GOOGLE_VALIDATION_LAYERS[] =
{
	"VK_LAYER_LUNARG_standard_validation",
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
	{KHRONOS_VALIDATION_LAYERS, ARRAY_SIZE(LUNARG_GOOGLE_VALIDATION_LAYERS)},
	{LUNARG_GOOGLE_VALIDATION_LAYERS, ARRAY_SIZE(LUNARG_GOOGLE_VALIDATION_LAYERS)},
};

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
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = pCallBack;
} 

/* TODO LIST
材质文件
*/

KCullSystem CULL_SYSTEM;

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
	m_ValidationLayerIdx(-1),
	m_MultiThreadSumbit(true),
	m_Texture(nullptr),
	m_Sampler(nullptr),
	m_FrameInFlight(2)
{
	m_MaxRenderThreadNum = std::thread::hardware_concurrency();
	ZERO_ARRAY_MEMORY(m_Move);
	ZERO_ARRAY_MEMORY(m_MouseDown);
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

	ASSERT_RESULT(m_pSwapChain == nullptr);
	m_pSwapChain = KVulkanSwapChainPtr(new KVulkanSwapChain());

	ASSERT_RESULT(m_pSwapChain->Init(m_Device,
		m_PhysicalDevice.device,
		m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first,
		m_PhysicalDevice.queueFamilyIndices.presentFamily.first,
		m_Surface,
		(uint32_t)windowWidth,
		(uint32_t)windowHeight,
		m_FrameInFlight));

	return true;
}

bool KVulkanRenderDevice::CreateImageViews()
{
	size_t chainImageCount	= m_pSwapChain->GetImageCount();
	VkExtent2D extend		= m_pSwapChain->GetExtent();
	VkFormat format			= m_pSwapChain->GetFormat();

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

	std::vector<IKRenderTarget*> renderTargets;

	VkImage image = VK_NULL_HANDLE;	

	m_OffScreenTextures.resize(m_FrameInFlight);
	for(size_t i = 0; i < m_OffScreenTextures.size(); ++i)
	{
		CreateTexture(m_OffScreenTextures[i]);
		m_OffScreenTextures[i]->InitMemeoryAsRT(extend.width, extend.height, EF_R16G16B16A16_FLOAT);
		m_OffScreenTextures[i]->InitDevice();
	}

	m_OffscreenRenderTargets.resize(m_FrameInFlight);
	for(size_t i = 0; i < m_OffscreenRenderTargets.size(); ++i)
	{
		CreateRenderTarget(m_OffscreenRenderTargets[i]);
#define HEX_COL(NUM) ((float)NUM / float(0XFF))
		m_OffscreenRenderTargets[i]->SetColorClear(HEX_COL(0x87), HEX_COL(0xCE), HEX_COL(0xFF), HEX_COL(0XFF));
#undef HEX_COL
		m_OffscreenRenderTargets[i]->SetDepthStencilClear(1.0, 0);
		m_OffscreenRenderTargets[i]->SetSize(extend.width, extend.height);
		m_OffscreenRenderTargets[i]->InitFromImageView(m_OffScreenTextures[i]->GetImageView(), true, true, msaaCount);

		renderTargets.push_back(m_OffscreenRenderTargets[i].get());
	}

	renderTargets.clear();
	m_SwapChainRenderTargets.resize(chainImageCount);
	for(size_t i = 0; i < m_SwapChainRenderTargets.size(); ++i)
	{
		CreateRenderTarget(m_SwapChainRenderTargets[i]);
		m_SwapChainRenderTargets[i]->SetColorClear(0.0f, 0.0f, 0.0f, 1.0f);
		m_SwapChainRenderTargets[i]->SetDepthStencilClear(1.0, 0);
		// TODO init from imageview
		m_SwapChainRenderTargets[i]->SetSize(extend.width, extend.height);

		ImageView imageView = {0};
		m_pSwapChain->GetImageView(i, imageView);

		m_SwapChainRenderTargets[i]->InitFromImageView(imageView, true, true, 1);

		renderTargets.push_back(m_SwapChainRenderTargets[i].get());
	}

	CreateUIOVerlay(m_UIOverlay);

	m_UIOverlay->Init(this, m_FrameInFlight);
	m_UIOverlay->Resize(extend.width, extend.height);

	return true;
}

bool KVulkanRenderDevice::CreatePipelines()
{
	{
		m_OffscreenPipelines.resize(m_FrameInFlight);
		for(size_t i = 0; i < m_OffscreenPipelines.size(); ++i)
		{
			CreatePipeline(m_OffscreenPipelines[i]);

			IKPipelinePtr pipeline = m_OffscreenPipelines[i];

			VertexFormat formats[] = {VF_POINT_NORMAL_UV};

			pipeline->SetVertexBinding(formats, 1);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetShader(ST_VERTEX, m_SceneVertexShader);
			pipeline->SetShader(ST_FRAGMENT, m_SceneFragmentShader);

			pipeline->SetBlendEnable(false);

			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);

			IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, 0, CBT_CAMERA);
			pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

			pipeline->SetSampler(CBT_COUNT, m_Texture->GetImageView(), m_Sampler);
			pipeline->SetSampler(CBT_COUNT + 1, KRenderGlobal::SkyBox.GetCubeTexture()->GetImageView(), KRenderGlobal::SkyBox.GetSampler());

			pipeline->PushConstantBlock(m_ObjectConstant.shaderTypes, m_ObjectConstant.size, m_ObjectConstant.offset); 

			pipeline->Init();
		}
	}

	{
		m_SwapChainPipelines.resize(m_FrameInFlight);
		for(size_t i = 0; i < m_SwapChainPipelines.size(); ++i)
		{
			CreatePipeline(m_SwapChainPipelines[i]);

			IKPipelinePtr pipeline = m_SwapChainPipelines[i];

			VertexFormat formats[] = {VF_SCREENQUAD_POS};

			pipeline->SetVertexBinding(formats, 1);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetShader(ST_VERTEX, m_PostVertexShader);
			pipeline->SetShader(ST_FRAGMENT, m_PostFragmentShader);

			pipeline->SetBlendEnable(false);

			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);

			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, 0, CBT_SHADOW);
			pipeline->SetConstantBuffer(CBT_SHADOW, ST_FRAGMENT, shadowBuffer);

			pipeline->SetSampler(0, m_OffScreenTextures[i]->GetImageView(), m_Sampler);

			pipeline->Init();
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

bool KVulkanRenderDevice::UpdateFrameTime()
{
	static int numFrames = 0;
	static int numFramesTotal = 0;

	static float fps = 0.0f;
	static float frameTime = 0.0f;

	static float maxFrameTime = 0;
	static float minFrameTime = 0;

	static KTimer FPSTimer;
	static KTimer MaxMinTimer;

	if(MaxMinTimer.GetMilliseconds() > 5000.0f)
	{
		maxFrameTime = frameTime;
		minFrameTime = frameTime;
		MaxMinTimer.Reset();
	}

	++numFrames;
	if(FPSTimer.GetMilliseconds() > 500.0f)
	{
		float time = FPSTimer.GetMilliseconds();
		time = time / (float)numFrames;
		frameTime = time;

		fps = 1000.0f / frameTime;

		if(frameTime > maxFrameTime)
		{
			maxFrameTime = time;
		}
		if(frameTime < minFrameTime)
		{
			minFrameTime = time;
		}
		FPSTimer.Reset();
		numFrames = 0;
	}

	char szBuffer[1024] = {};
	sprintf(szBuffer, "[FPS] %f [FrameTime] %f [MinTime] %f [MaxTime] %f [Frame]%d", fps, frameTime, minFrameTime, maxFrameTime, numFramesTotal++);
	m_pWindow->SetWindowTitle(szBuffer);

	return true;
}

bool KVulkanRenderDevice::Reload()
{
	Wait();
	KRenderGlobal::ShaderManager.Reload();
	KRenderGlobal::PipelineManager.Reload();
	return true;
}

bool KVulkanRenderDevice::CreateMesh()
{
	const char* szPaths[] =
	{
		"../Dependency/assimp-3.3.1/test/models/OBJ/spider.obj",
		"../Sponza/sponza.obj"
	};

	const char* destPaths[] =
	{
		"../Dependency/assimp-3.3.1/test/models/OBJ/spider.mesh",
		"../Sponza/sponza.mesh"
	};

#if 1
#ifdef _DEBUG
	int width = 3, height = 3;
#else
	int width = 30, height = 30;
#endif
	int widthExtend = width * 8, heightExtend = height * 8;
	for(int i = 0; i <= width; ++i)
	{
		for(int j = 0; j <= height; ++j)
		{
			KEntityPtr entity = KECSGlobal::EntityManager.CreateEntity();

			KComponentBase* component = nullptr;
			if(entity->RegisterComponent(CT_RENDER, &component))
			{
				((KRenderComponent*)component)->InitFromAsset(szPaths[0]);
			}

			if(entity->RegisterComponent(CT_TRANSFORM, &component))
			{
				glm::vec3& pos = ((KTransformComponent*)component)->GetPosition();
				pos.x =(float)(i * 2 - width) / width * widthExtend;
				pos.z = (float)(j * 2 - height) / height * heightExtend;
				pos.y = 0;

				glm::vec3& scale = ((KTransformComponent*)component)->GetScale();
				scale = glm::vec3(0.1f, 0.1f, 0.1f);
			}
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
	}
#endif
	return true;
}

bool KVulkanRenderDevice::CreateVertexInput()
{
	using namespace KVertexDefinition;

	// Square
	{
		const POS_3F_NORM_3F_UV_2F vertices[] =
		{
			{glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
			{glm::vec3(0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
			{glm::vec3(-0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
			{glm::vec3(-0.5f, 0.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)},
		};

		m_Box.SetNull();
		KAABBBox tempCalcResult;
		for(int i = 0; i < ARRAY_SIZE(vertices); ++i)
		{
			m_Box.Merge(vertices[i].POSITION, tempCalcResult);
			m_Box = tempCalcResult;
		}

		CreateVertexBuffer(m_SqaureData.vertexBuffer);
		m_SqaureData.vertexBuffer->InitMemory(sizeof(vertices) / sizeof(vertices[0]), sizeof(vertices[0]), vertices);
		m_SqaureData.vertexBuffer->InitDevice(false);

		const uint32_t indices[] = {0, 1, 2, 2, 3, 0};
		CreateIndexBuffer(m_SqaureData.indexBuffer);
		m_SqaureData.indexBuffer->InitMemory(IT_32, sizeof(indices) / sizeof(indices[0]), indices);
		m_SqaureData.indexBuffer->InitDevice(false);
	}

	// Screen Quad
	{
		const SCREENQUAD_POS_2F vertices[] =
		{
			glm::vec2(-1.0f, -1.0f),
			glm::vec2(1.0f, -1.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(-1.0f, 1.0f)
		};

		CreateVertexBuffer(m_QuadData.vertexBuffer);
		m_QuadData.vertexBuffer->InitMemory(sizeof(vertices) / sizeof(vertices[0]), sizeof(vertices[0]), vertices);
		m_QuadData.vertexBuffer->InitDevice(false);

		const uint32_t indices[] = {0, 1, 2, 2, 3, 0};
		CreateIndexBuffer(m_QuadData.indexBuffer);
		m_QuadData.indexBuffer->InitMemory(IT_32, sizeof(indices) / sizeof(indices[0]), indices);
		m_QuadData.indexBuffer->InitDevice(false);
	}

	return true;
}

bool KVulkanRenderDevice::CreateTransform()
{
	m_ObjectConstant.shaderTypes = ST_VERTEX;
	m_ObjectConstant.size = (int)KConstantDefinition::GetConstantBufferDetail(CBT_OBJECT).bufferSize;

#ifdef _DEBUG
	const int width = 40;
	const int height = 40;
#else
	const int width = 0;
	const int height = 0;
#endif

	m_ObjectTransforms.clear();
	m_ObjectTransforms.reserve(width * height);

	m_ObjectFinalTransforms.clear();
	m_ObjectFinalTransforms.reserve(width * height);
	for(size_t x = 0; x <= width; ++x)
	{
		for(size_t y = 0; y <= height; ++y)
		{
			float xPos = ((float)x - (float)width / 2.0f);
			float yPos = ((float)y - (float)height / 2.0f);
			ObjectInitTransform transform = 
			{
				glm::rotate(glm::mat4(1.0f),
				glm::radians((1000.0f * float(rand() % 1000)) * glm::two_pi<float>()), 
				glm::vec3(0.0f, 1.0f, 0.0f)),
				glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0.0, yPos)),
			};
			m_ObjectTransforms.push_back(transform);
			m_ObjectFinalTransforms.push_back(glm::mat4(1.0f));
		}
	}

	return true;
}

bool KVulkanRenderDevice::CreateResource()
{
	ASSERT_RESULT(KRenderGlobal::TextrueManager.Acquire("Textures/vulkan_11_rgba.ktx", m_Texture));

	CreateSampler(m_Sampler);
	//m_Sampler->SetAnisotropic(true);
	//m_Sampler->SetAnisotropicCount(16);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->SetMipmapLod(0, m_Texture->GetMipmaps());
	m_Sampler->Init();

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/shader.vert", m_SceneVertexShader));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/demo.frag", m_SceneFragmentShader));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/screenquad.vert", m_PostVertexShader));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/screenquad.frag", m_PostFragmentShader));

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

bool KVulkanRenderDevice::InitGlobalManager()
{
	KVulkanHeapAllocator::Init();

	KRenderGlobal::PipelineManager.Init(this);
	KRenderGlobal::FrameResourceManager.Init(this, m_FrameInFlight, m_MaxRenderThreadNum);
	KRenderGlobal::MeshManager.Init(this, m_FrameInFlight, m_MaxRenderThreadNum);
	KRenderGlobal::ShaderManager.Init(this);
	KRenderGlobal::TextrueManager.Init(this);

	KRenderGlobal::SkyBox.Init(this, m_FrameInFlight, "Textures/uffizi_cube.ktx");
	KRenderGlobal::ShadowMap.Init(this, m_FrameInFlight, 2048);

	KECSGlobal::Init();

	return true;
}

bool KVulkanRenderDevice::UnInitGlobalManager()
{
	KECSGlobal::UnInit();

	KRenderGlobal::SkyBox.UnInit();
	KRenderGlobal::ShadowMap.UnInit();

	KRenderGlobal::PipelineManager.UnInit();
	KRenderGlobal::FrameResourceManager.UnInit();
	KRenderGlobal::MeshManager.UnInit();
	KRenderGlobal::ShaderManager.UnInit();
	KRenderGlobal::TextrueManager.UnInit();

	KVulkanHeapAllocator::UnInit();

	return true;
}

bool KVulkanRenderDevice::AddWindowCallback()
{
	ZERO_ARRAY_MEMORY(m_Move);
	ZERO_ARRAY_MEMORY(m_MouseDown);

	for(int i = 0; i < ARRAY_SIZE(m_MousePos); ++i)
	{
		m_MousePos[0] = 0.0f;
		m_MousePos[1] = 0.0f;
	}

	m_Camera.SetPosition(glm::vec3(0, 400.0f, 400.0f));
	m_Camera.LookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_Camera.SetCustomLockYAxis(glm::vec3(0,1,0));
	m_Camera.SetLockYEnable(true);

	m_KeyCallback = [this](InputKeyboard key, InputAction action)
	{
		int sign = 0;
		if(action == INPUT_ACTION_PRESS)
		{
			sign = 1;
		}
		else if(action == INPUT_ACTION_RELEASE)
		{
			sign = -1;
		}

		switch (key)
		{
		case INPUT_KEY_W:
			m_Move[2] += sign;
			break;
		case INPUT_KEY_S:
			m_Move[2] += -sign;
			break;
		case INPUT_KEY_A:
			m_Move[0] -= sign;
			break;
		case INPUT_KEY_D:
			m_Move[0] += sign;
			break;
		case INPUT_KEY_E:
			m_Move[1] += sign;
			break;
		case INPUT_KEY_Q:
			m_Move[1] -= sign;
			break;
		default:
			break;
		}

		if(key == INPUT_KEY_ENTER)
		{
			Reload();
		}
	};

	m_MouseCallback = [this](InputMouseButton mouse, InputAction action, float xPos, float yPos)
	{
		m_UIOverlay->SetMousePosition((unsigned int)xPos, (unsigned int)yPos);

		if(action == INPUT_ACTION_PRESS)
		{
			m_MousePos[0] = xPos;
			m_MousePos[1] = yPos;

			m_MouseDown[mouse] = true;
			m_UIOverlay->SetMouseDown(mouse, true);
		}
		if(action == INPUT_ACTION_RELEASE)
		{
			m_MouseDown[mouse] = false;
			m_UIOverlay->SetMouseDown(mouse, false);
		}
		if(action == INPUT_ACTION_REPEAT)
		{
			float deltaX = xPos - m_MousePos[0];
			float deltaY = yPos - m_MousePos[1];

			size_t width; size_t height;
			m_pWindow->GetSize(width, height);

			if(m_MouseDown[INPUT_MOUSE_BUTTON_RIGHT])
			{
				if(abs(deltaX) > 0.0001f)
				{
					m_Camera.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -glm::quarter_pi<float>() * deltaX / width);
				}
				if(abs(deltaY) > 0.0001f)
				{
					m_Camera.RotateRight(-glm::quarter_pi<float>() * deltaY / height);
				}
			}
			if(m_MouseDown[INPUT_MOUSE_BUTTON_LEFT])
			{
				const float fSpeed = 500.f;

				glm::vec3 forward = m_Camera.GetForward(); forward.y = 0.0f; forward = glm::normalize(forward);
				glm::vec3 right = m_Camera.GetRight(); right.y = 0.0f; right = glm::normalize(right);

				m_Camera.Move(deltaY * fSpeed * forward / (float)width);
				m_Camera.Move(-deltaX * fSpeed * right / (float)height);
			}

			m_MousePos[0] = xPos;
			m_MousePos[1] = yPos;
		}
	};

	m_ScrollCallback = [this](float xOffset, float yOffset)
	{
		const float fSpeed = 15.0f;
		m_Camera.MoveForward(fSpeed * yOffset);
	};

	m_pWindow->RegisterKeyboardCallback(&m_KeyCallback);
	m_pWindow->RegisterMouseCallback(&m_MouseCallback);
	m_pWindow->RegisterScrollCallback(&m_ScrollCallback);

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

	// temp
	AddWindowCallback();

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
			PopulateDebugMessengerCreateInfo(debugCreateInfo, DebugCallback);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
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
		if(!CreatePipelineCache())
			return false;
		if(!CreateCommandPool())
			return false;

		// 实际完成了设备初始化
		if(!InitDeviceGlobal())
			return false;
		// 初始化全局对象
		if(!InitGlobalManager())
			return false;

		if(!CreateSwapChain())
			return false;
		if(!CreateImageViews())
			return false;

		// Temporarily for demo use
		if(!CreateMesh())
			return false;
		if(!CreateVertexInput())
			return false;
		if(!CreateTransform())
			return false;
		if(!CreateResource())
			return false;
		if(!CreatePipelines())
			return false;
#ifndef THREAD_MODE_ONE
		m_ThreadPool.PushWorkerThreads(m_MaxRenderThreadNum);
#else
		m_ThreadPool.SetThreadCount(m_MaxRenderThreadNum);
#endif
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

	CleanupSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreatePipelines();

	return true;
}

bool KVulkanRenderDevice::CleanupSwapChain()
{
	if(m_UIOverlay)
	{
		m_UIOverlay->UnInit();
		m_UIOverlay = nullptr;
	}
	
	// clear offscreen rts
	for (IKTexturePtr texture : m_OffScreenTextures)
	{
		texture->UnInit();
	}
	m_OffScreenTextures.clear();
	for (IKRenderTargetPtr target :  m_OffscreenRenderTargets)
	{
		target->UnInit();
	}
	m_OffscreenRenderTargets.clear();
	for (IKPipelinePtr pipeline :  m_OffscreenPipelines)
	{
		pipeline->UnInit();
	}
	m_OffscreenPipelines.clear();

	// clear swapchain rts
	for (IKRenderTargetPtr target :  m_SwapChainRenderTargets)
	{
		target->UnInit();
	}
	m_SwapChainRenderTargets.clear();

	for (IKPipelinePtr pipeline :  m_SwapChainPipelines)
	{
		pipeline->UnInit();
	}
	m_SwapChainPipelines.clear();

	// clear swapchain
	m_pSwapChain->UnInit();
	m_pSwapChain = nullptr;

	return true;
}

bool KVulkanRenderDevice::UnInit()
{
#ifndef THREAD_MODE_ONE
	m_ThreadPool.WaitAllAsyncTaskDone();
	m_ThreadPool.PopAllWorkerThreads();
#else
	m_ThreadPool.WaitAll();
#endif
	
	m_pWindow = nullptr;

	if(m_SqaureData.indexBuffer)
	{
		m_SqaureData.indexBuffer->UnInit();
		m_SqaureData.indexBuffer = nullptr;
	}
	if(m_SqaureData.vertexBuffer)
	{
		m_SqaureData.vertexBuffer->UnInit();
		m_SqaureData.vertexBuffer = nullptr;
	}

	if(m_QuadData.indexBuffer)
	{
		m_QuadData.indexBuffer->UnInit();
		m_QuadData.indexBuffer = nullptr;
	}
	if(m_QuadData.vertexBuffer)
	{
		m_QuadData.vertexBuffer->UnInit();
		m_QuadData.vertexBuffer = nullptr;
	}

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
	
	KRenderGlobal::TextrueManager.Release(m_Texture);
	m_Texture = nullptr;

	if(m_Sampler)
	{
		m_Sampler->UnInit();
		m_Sampler = nullptr;
	}

	KRenderGlobal::ShaderManager.Release(m_SceneVertexShader);
	m_SceneVertexShader = nullptr;
	KRenderGlobal::ShaderManager.Release(m_SceneFragmentShader);
	m_SceneFragmentShader = nullptr;
	KRenderGlobal::ShaderManager.Release(m_PostVertexShader);
	m_PostVertexShader = nullptr;
	KRenderGlobal::ShaderManager.Release(m_PostFragmentShader);
	m_PostFragmentShader = nullptr;

	// clear command buffers
	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		for (ThreadData& thread : m_CommandBuffers[i].threadDatas)
		{
			thread.commandBuffer->UnInit();
			thread.commandBuffer = nullptr;

			thread.commandPool->UnInit();
			thread.commandPool = nullptr;
		}

		m_CommandBuffers[i].primaryCommandBuffer->UnInit();
		m_CommandBuffers[i].skyBoxCommandBuffer->UnInit();
		m_CommandBuffers[i].shadowMapCommandBuffer->UnInit();
		m_CommandBuffers[i].uiCommandBuffer->UnInit();
		m_CommandBuffers[i].postprocessCommandBuffer->UnInit();

		m_CommandBuffers[i].commandPool->UnInit();
		m_CommandBuffers[i].commandPool = nullptr;
	}
	m_CommandBuffers.clear();

	CleanupSwapChain();

	vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);

	vkDestroyCommandPool(m_Device, m_GraphicCommandPool, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	UnInitGlobalManager();
	UnsetDebugMessenger();

	vkDestroyDevice(m_Device, nullptr);
	vkDestroyInstance(m_Instance, nullptr);

	UnInitDeviceGlobal();
	return true;
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

bool KVulkanRenderDevice::InitDeviceGlobal()
{
	KVulkanGlobal::device = m_Device;
	KVulkanGlobal::physicalDevice = m_PhysicalDevice.device;
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

bool KVulkanRenderDevice::CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle)
{
	pipelineHandle = IKPipelineHandlePtr(static_cast<IKPipelineHandle*>(new KVulkanPipelineHandle()));
	return true;
}

bool KVulkanRenderDevice::CreateUIOVerlay(IKUIOverlayPtr& ui)
{
	ui = IKUIOverlayPtr(static_cast<IKUIOverlay*>(new KVulkanUIOverlay()));
	return true;
}

bool KVulkanRenderDevice::UpdateCamera(size_t idx)
{
	ASSERT_RESULT(idx < m_FrameInFlight);

	static KTimer m_MoveTimer;

	const float dt = m_MoveTimer.GetSeconds();
	const float moveSpeed = 300.0f;
	m_MoveTimer.Reset();

	m_Camera.MoveRight(dt * moveSpeed * m_Move[0]);
	m_Camera.Move(dt * moveSpeed * m_Move[1] * glm::vec3(0,1,0));
	m_Camera.MoveForward(dt * moveSpeed * m_Move[2]);

	VkExtent2D extend = m_pSwapChain->GetExtent();

	m_Camera.SetPerspective(glm::radians(45.0f), extend.width / (float) extend.height, 1.0f, 10000.0f);

	glm::mat4 view = m_Camera.GetViewMatrix();
	glm::mat4 proj = m_Camera.GetProjectiveMatrix();
	glm::mat4 viewInv = glm::inverse(view);
	glm::vec2 near_far = glm::vec2(m_Camera.GetNear(), m_Camera.GetFar());
	{
		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(idx, 0, CBT_CAMERA);
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
			else if(detail.semantic == CS_VIEW_INV)
			{
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				assert(sizeof(viewInv) == detail.size);
				memcpy(pWritePos, &viewInv, sizeof(viewInv));
			}
		}
		cameraBuffer->Write(pData);
	}
	return true;
}

bool KVulkanRenderDevice::UpdateObjectTransform()
{
	static auto lastTime = std::chrono::high_resolution_clock::now();
	static auto firstTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();

	float fLastTime = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>>(lastTime - firstTime).count();
	float fCurrentTime = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>>(currentTime - firstTime).count();

	glm::mat4 deltaRotate = glm::rotate(glm::mat4(1.0f), (fCurrentTime - fLastTime) * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	const float speed = 1.0f;
	const float maxRange = 5.0f;
	float translate = maxRange * ((sinf(fCurrentTime * speed) - sinf(fLastTime * speed)) / glm::pi<float>());

	glm::mat4 deltaTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, translate, 0.0f));

	for(size_t i = 0; i < m_ObjectTransforms.size(); ++i)
	{
		ObjectInitTransform& transform = m_ObjectTransforms[i];
		transform.translate = deltaTranslate * transform.translate;
		transform.rotate = deltaRotate * transform.rotate;
	}

	for(size_t i = 0; i < m_ObjectFinalTransforms.size(); ++i)
	{
		ObjectInitTransform& transform = m_ObjectTransforms[i];
		glm::mat4& model = m_ObjectFinalTransforms[i];
		model = transform.translate * transform.rotate;
	}

	lastTime = currentTime;
	return true;
}

void KVulkanRenderDevice::ThreadRenderObject(uint32_t threadIndex, uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKPipelineHandlePtr pipelineHandle;

	ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];

	// https://devblogs.nvidia.com/vulkan-dos-donts/ ResetCommandPool释放内存
	threadData.commandPool->Reset();

#ifndef THREAD_MODE_ONE
	size_t numThread = m_ThreadPool.GetWorkerThreadNum();
#else
	size_t numThread = m_ThreadPool.GetThreadCount();
#endif

	threadData.commandBuffer->BeginSecondary(m_OffscreenRenderTargets[frameIndex].get());
	threadData.commandBuffer->SetViewport(m_OffscreenRenderTargets[frameIndex].get());

	VkCommandBuffer vkBufferHandle = ((KVulkanCommandBuffer*)threadData.commandBuffer.get())->GetVkHandle();

	KVulkanRenderTarget* target = (KVulkanRenderTarget*)m_OffscreenRenderTargets[frameIndex].get();
	{
		KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_OffscreenPipelines[frameIndex].get();

		KRenderGlobal::PipelineManager.GetPipelineHandle(vulkanPipeline, target, pipelineHandle);
		VkPipeline pipeline = ((KVulkanPipelineHandle*)pipelineHandle.get())->GetVkPipeline();

		VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
		VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

		// 绑定管线
		vkCmdBindPipeline(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// 绑定管线布局
		vkCmdBindDescriptorSets(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// 绑定顶点缓冲
		KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_SqaureData.vertexBuffer.get();
		VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(vkBufferHandle, 0, 1, vertexBuffers, offsets);
		// 绑定索引缓冲
		KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_SqaureData.indexBuffer.get();
		vkCmdBindIndexBuffer(vkBufferHandle, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

		for(size_t i = 0; i < threadData.num; ++i)
		{
			glm::mat4& model = m_ObjectFinalTransforms[i + threadData.offset];

			KAABBBox objectBox;
			m_Box.Transform(model, objectBox);
			if(m_Camera.CheckVisible(objectBox))
			{
				vkCmdPushConstants(vkBufferHandle, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, m_ObjectConstant.offset, m_ObjectConstant.size, &model);
				vkCmdDrawIndexed(vkBufferHandle, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
			}
		}
	}

	for(KRenderCommand& command : threadData.commands)
	{
		threadData.commandBuffer->Render(command);
	}

	threadData.commandBuffer->End();
}

bool KVulkanRenderDevice::SubmitCommandBufferSingleThread(uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKPipelineHandlePtr pipelineHandle = nullptr;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// 开始渲染过程

	IKCommandBufferPtr primaryBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	VkCommandBuffer vkBufferHandle = ((KVulkanCommandBuffer*)primaryBuffer.get())->GetVkHandle();

	primaryBuffer->BeginPrimary();
	{
		primaryBuffer->BeginRenderPass((KRenderGlobal::ShadowMap.GetShadowMapTarget(frameIndex)).get(), SUBPASS_CONTENTS_INLINE);
		{
			KRenderGlobal::ShadowMap.UpdateShadowMap(this, primaryBuffer.get(), frameIndex);
		}
		primaryBuffer->EndRenderPass();

		primaryBuffer->BeginRenderPass(m_OffscreenRenderTargets[frameIndex].get(), SUBPASS_CONTENTS_INLINE);
		{
			primaryBuffer->SetViewport(m_OffscreenRenderTargets[frameIndex].get());
			// 开始渲染SkyBox
			{
				KRenderCommand command;
				if(KRenderGlobal::SkyBox.GetRenderCommand(frameIndex, command))
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, m_OffscreenRenderTargets[frameIndex].get(), handle);
					command.pipelineHandle = handle.get();
					primaryBuffer->Render(command);
				}
			}
			// 开始渲染物件
			{
				KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_OffscreenPipelines[frameIndex].get();

				KRenderGlobal::PipelineManager.GetPipelineHandle(vulkanPipeline, m_OffscreenRenderTargets[frameIndex].get(), pipelineHandle);
				VkPipeline pipeline = ((KVulkanPipelineHandle*)pipelineHandle.get())->GetVkPipeline();

				VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
				VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

				// 绑定管线
				vkCmdBindPipeline(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				// 绑定管线布局
				vkCmdBindDescriptorSets(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

				// 绑定顶点缓冲
				KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_SqaureData.vertexBuffer.get();
				VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(vkBufferHandle, 0, 1, vertexBuffers, offsets);
				// 绑定索引缓冲
				KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_SqaureData.indexBuffer.get();
				vkCmdBindIndexBuffer(vkBufferHandle, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

				// 绘制调用
				for(glm::mat4& model : m_ObjectFinalTransforms)
				{
					KAABBBox objectBox;
					m_Box.Transform(model, objectBox);
					if(m_Camera.CheckVisible(objectBox))
					{
						vkCmdPushConstants(vkBufferHandle, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, m_ObjectConstant.offset, m_ObjectConstant.size, &model);
						vkCmdDrawIndexed(vkBufferHandle, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
					}
				}

				KRenderCommandList commandList;

				std::vector<KRenderComponent*> cullRes;
				CULL_SYSTEM.Execute(m_Camera, cullRes);

				for(KRenderComponent* component : cullRes)
				{
					KEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if(entity->GetComponent(CT_TRANSFORM, (KComponentBase**)&transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, 0, [&](KRenderCommand command)
						{
							command.useObjectData = true;
							command.objectData = &transform->FinalTransform();
							commandList.push_back(command);
						});
					}
				}

				for(KRenderCommand& command : commandList)
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, m_OffscreenRenderTargets[frameIndex].get(), handle);
					command.pipelineHandle = handle.get();
					primaryBuffer->Render(command);
				}
			}
		}
		primaryBuffer->EndRenderPass();

		// 绘制ScreenQuad与UI
		primaryBuffer->BeginRenderPass(m_SwapChainRenderTargets[chainImageIndex].get(), SUBPASS_CONTENTS_INLINE);
		{
			primaryBuffer->SetViewport(m_SwapChainRenderTargets[chainImageIndex].get());
			{
				KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_SwapChainPipelines[frameIndex].get();

				KRenderGlobal::PipelineManager.GetPipelineHandle(vulkanPipeline, m_SwapChainRenderTargets[chainImageIndex].get(), pipelineHandle);
				VkPipeline pipeline = ((KVulkanPipelineHandle*)pipelineHandle.get())->GetVkPipeline();

				VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
				VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

				// 绑定管线
				vkCmdBindPipeline(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				// 绑定管线布局
				vkCmdBindDescriptorSets(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

				// 绑定顶点缓冲
				KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_QuadData.vertexBuffer.get();
				VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(vkBufferHandle, 0, 1, vertexBuffers, offsets);
				// 绑定索引缓冲
				KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_QuadData.indexBuffer.get();
				vkCmdBindIndexBuffer(vkBufferHandle, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

				vkCmdDrawIndexed(vkBufferHandle, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
			}
			{
				m_UIOverlay->Draw(frameIndex, m_SwapChainRenderTargets[chainImageIndex].get(), &vkBufferHandle);
			}
		}
		primaryBuffer->EndRenderPass();
	}
	m_CommandBuffers[frameIndex].primaryCommandBuffer->End();

	return true;
}

bool KVulkanRenderDevice::SubmitCommandBufferMuitiThread(uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKPipelineHandlePtr pipelineHandle;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	KVulkanRenderTarget* offscreenTarget = (KVulkanRenderTarget*)m_OffscreenRenderTargets[frameIndex].get();
	KVulkanRenderTarget* swapChainTarget = (KVulkanRenderTarget*)m_SwapChainRenderTargets[chainImageIndex].get();
	KVulkanRenderTarget* shadowMapTarget = (KVulkanRenderTarget*)KRenderGlobal::ShadowMap.GetShadowMapTarget(frameIndex).get();

	IKCommandBufferPtr primaryCommandBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	IKCommandBufferPtr skyBoxCommandBuffer = m_CommandBuffers[frameIndex].skyBoxCommandBuffer;
	IKCommandBufferPtr shadowMapCommandBuffer = m_CommandBuffers[frameIndex].shadowMapCommandBuffer;
	IKCommandBufferPtr uiCommandBuffer = m_CommandBuffers[frameIndex].uiCommandBuffer;
	IKCommandBufferPtr postprocessCommandBuffer = m_CommandBuffers[frameIndex].postprocessCommandBuffer;

	VkCommandBuffer vkBufferHandle = VK_NULL_HANDLE;
	// 开始渲染过程
	primaryCommandBuffer->BeginPrimary();
	{
		// 阴影绘制RenderPass
		{
			{
				primaryCommandBuffer->BeginRenderPass(shadowMapTarget, SUBPASS_CONTENTS_SECONDARY);
				{
					shadowMapCommandBuffer->BeginSecondary(shadowMapTarget);
					KRenderGlobal::ShadowMap.UpdateShadowMap(this, shadowMapCommandBuffer.get(), frameIndex);
					shadowMapCommandBuffer->End();
				}
				primaryCommandBuffer->Execute(shadowMapCommandBuffer.get());
				primaryCommandBuffer->EndRenderPass();
			}
		}
		// 物件绘制RenderPass
		{
			primaryCommandBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_SECONDARY);

			auto commandBuffers = m_CommandBuffers[frameIndex].commandBuffersExec;
			commandBuffers.clear();

			// 绘制SkyBox
			{
				skyBoxCommandBuffer->BeginSecondary(offscreenTarget);
				skyBoxCommandBuffer->SetViewport(offscreenTarget);

				KRenderCommand command;
				if(KRenderGlobal::SkyBox.GetRenderCommand(frameIndex, command))
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle);
					command.pipelineHandle = handle.get();
					skyBoxCommandBuffer->Render(command);
				}
				skyBoxCommandBuffer->End();

				vkBufferHandle = ((KVulkanCommandBuffer*)skyBoxCommandBuffer.get())->GetVkHandle();
				commandBuffers.push_back(skyBoxCommandBuffer.get());
			}

#ifndef THREAD_MODE_ONE
			size_t threadCount = m_ThreadPool.GetWorkerThreadNum();
#else
			size_t threadCount = m_ThreadPool.GetThreadCount();
#endif

			std::vector<KRenderComponent*> cullRes;
			CULL_SYSTEM.Execute(m_Camera, cullRes);

			size_t drawEachThread = cullRes.size() / threadCount;
			size_t reaminCount = cullRes.size() % threadCount;

			for(size_t i = 0; i < threadCount; ++i)
			{
				ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[i];
				threadData.commands.clear();

				for(size_t st = i * drawEachThread, ed = st + drawEachThread + (i == threadCount - 1 ? reaminCount : 0); st < ed; ++st)
				{
					KRenderComponent* component = cullRes[st];
					KMeshPtr mesh = component->GetMesh();

					KEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if(entity->GetComponent(CT_TRANSFORM, (KComponentBase**)&transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, i, [&](KRenderCommand command)
						{
							command.useObjectData = true;
							command.objectData = &transform->FinalTransform();
							threadData.commands.push_back(command);
						});
					}
				}

				for(KRenderCommand& command : threadData.commands)
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle);
					command.pipelineHandle = handle.get();
				}
			}

#ifndef THREAD_MODE_ONE
			for(size_t i = 0; i < m_ThreadPool.GetWorkerThreadNum(); ++i)
			{
				m_ThreadPool.SubmitTask([=]()
				{
					ThreadRenderObject((uint32_t)i, chainImageIndex, frameIndex);
				});
			}
			m_ThreadPool.WaitAllAsyncTaskDone();
#else
			for(size_t i = 0; i < m_ThreadPool.GetThreadCount(); ++i)
			{
				m_ThreadPool.AddJob(i, [=]()
				{
					ThreadRenderObject((uint32_t)i, chainImageIndex, frameIndex);
				});
			}

			m_ThreadPool.WaitAll();
#endif

			for(size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
			{
				ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];
				commandBuffers.push_back(threadData.commandBuffer.get());
			}
			primaryCommandBuffer->ExecuteAll(std::move(commandBuffers));
		}
		primaryCommandBuffer->EndRenderPass();

		// 后处理与UI RenderPass
		primaryCommandBuffer->BeginRenderPass(swapChainTarget, SUBPASS_CONTENTS_SECONDARY);
		{
			{
				postprocessCommandBuffer->BeginSecondary(swapChainTarget);
				postprocessCommandBuffer->SetViewport(swapChainTarget);

				vkBufferHandle = ((KVulkanCommandBuffer*)postprocessCommandBuffer.get())->GetVkHandle();

				KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_SwapChainPipelines[frameIndex].get();

				KRenderGlobal::PipelineManager.GetPipelineHandle(vulkanPipeline, swapChainTarget, pipelineHandle);
				VkPipeline pipeline = ((KVulkanPipelineHandle*)pipelineHandle.get())->GetVkPipeline();

				VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
				VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

				// 绑定管线
				vkCmdBindPipeline(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				// 绑定管线布局
				vkCmdBindDescriptorSets(vkBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

				// 绑定顶点缓冲
				KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_QuadData.vertexBuffer.get();
				VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(vkBufferHandle, 0, 1, vertexBuffers, offsets);
				// 绑定索引缓冲
				KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_QuadData.indexBuffer.get();
				vkCmdBindIndexBuffer(vkBufferHandle, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

				VkOffset2D offset = {0, 0};
				VkExtent2D extent = swapChainTarget->GetExtend();

				vkCmdDrawIndexed(vkBufferHandle, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);

				postprocessCommandBuffer->End();
			}
			primaryCommandBuffer->Execute(postprocessCommandBuffer.get());

			{
				uiCommandBuffer->BeginSecondary(swapChainTarget);
				uiCommandBuffer->SetViewport(swapChainTarget);
				vkBufferHandle = ((KVulkanCommandBuffer*)uiCommandBuffer.get())->GetVkHandle();
				m_UIOverlay->Draw(frameIndex, swapChainTarget, &vkBufferHandle);

				uiCommandBuffer->End();
			}
			primaryCommandBuffer->Execute(uiCommandBuffer.get());
		}
		primaryCommandBuffer->EndRenderPass();
	}
	primaryCommandBuffer->End();

	return true;
}

bool KVulkanRenderDevice::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_FrameInFlight);

#ifndef THREAD_MODE_ONE
	size_t numThread = m_ThreadPool.GetWorkerThreadNum();
#else
	size_t numThread = m_ThreadPool.GetThreadCount();
#endif

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		CreateCommandPool(m_CommandBuffers[i].commandPool);
		m_CommandBuffers[i].commandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
		VkCommandPool pool = ((KVulkanCommandPool*)m_CommandBuffers[i].commandPool.get())->GetVkHandle();

		CreateCommandBuffer(m_CommandBuffers[i].primaryCommandBuffer);
		m_CommandBuffers[i].primaryCommandBuffer->Init(m_CommandBuffers[i].commandPool.get(), CBL_PRIMARY);

		CreateCommandBuffer(m_CommandBuffers[i].skyBoxCommandBuffer);
		m_CommandBuffers[i].skyBoxCommandBuffer->Init(m_CommandBuffers[i].commandPool.get(), CBL_SECONDARY);

		CreateCommandBuffer(m_CommandBuffers[i].shadowMapCommandBuffer);
		m_CommandBuffers[i].shadowMapCommandBuffer->Init(m_CommandBuffers[i].commandPool.get(), CBL_SECONDARY);

		CreateCommandBuffer(m_CommandBuffers[i].uiCommandBuffer);
		m_CommandBuffers[i].uiCommandBuffer->Init(m_CommandBuffers[i].commandPool.get(), CBL_SECONDARY);

		CreateCommandBuffer(m_CommandBuffers[i].postprocessCommandBuffer);
		m_CommandBuffers[i].postprocessCommandBuffer->Init(m_CommandBuffers[i].commandPool.get(), CBL_SECONDARY);

		m_CommandBuffers[i].threadDatas.resize(numThread);
#ifndef THREAD_MODE_ONE
		size_t numThread = m_ThreadPool.GetWorkerThreadNum();
#else
		size_t numThread = m_ThreadPool.GetThreadCount();
#endif

		size_t numPerThread = m_ObjectTransforms.size() / numThread;
		size_t numRemain = m_ObjectTransforms.size() % numThread;

		// 创建线程命令缓冲与命令池
		for (size_t threadIdx = 0; threadIdx < numThread; ++threadIdx)
		{
			ThreadData& threadData = m_CommandBuffers[i].threadDatas[threadIdx];

			CreateCommandPool(threadData.commandPool);
			threadData.commandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

			threadData.num = numPerThread + ((threadIdx == numThread - 1) ? numRemain : 0);
			threadData.offset = numPerThread * threadIdx;

			CreateCommandBuffer(threadData.commandBuffer);
			threadData.commandBuffer->Init(threadData.commandPool.get(), CBL_SECONDARY);
		}
	}
	return true;
}

bool KVulkanRenderDevice::Present()
{
	VkResult vkResult;

	size_t frameIndex = 0;
	vkResult = m_pSwapChain->WaitForInfightFrame(frameIndex);
	VK_ASSERT_RESULT(vkResult);

	uint32_t chainImageIndex = 0;
	vkResult = m_pSwapChain->AcquireNextImage(chainImageIndex);

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return true;
	}
	else if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
	{
		return false;
	}

	UpdateCamera(frameIndex);
	UpdateObjectTransform();
	UpdateFrameTime();

	m_UIOverlay->StartNewFrame();
	{
		m_UIOverlay->SetWindowPos(10, 10);
		m_UIOverlay->SetWindowSize(0, 0);
		m_UIOverlay->Begin("Example");
		{
			m_UIOverlay->PushItemWidth(110.0f);
			if (m_UIOverlay->Header("Setting"))
			{
				m_UIOverlay->CheckBox("MultiRender", &m_MultiThreadSumbit);
				m_UIOverlay->SliderFloat("Shadow DepthBiasConstant", &KRenderGlobal::ShadowMap.GetDepthBiasConstant(), 0.0f, 5.0f);
				m_UIOverlay->SliderFloat("Shadow DepthBiasSlope", &KRenderGlobal::ShadowMap.GetDepthBiasSlope(), 0.0f, 5.0f);
			}
			m_UIOverlay->PopItemWidth();
		}
		m_UIOverlay->End();
	}
	m_UIOverlay->EndNewFrame();

	m_UIOverlay->Update((uint32_t)frameIndex);

	if(m_MultiThreadSumbit)
	{
		SubmitCommandBufferMuitiThread(chainImageIndex, (uint32_t)frameIndex);
	}
	else
	{
		SubmitCommandBufferSingleThread(chainImageIndex, (uint32_t)frameIndex);
	}

	VkCommandBuffer primaryCommandBuffer = ((KVulkanCommandBuffer*)m_CommandBuffers[frameIndex].primaryCommandBuffer.get())->GetVkHandle();
	vkResult = m_pSwapChain->PresentQueue(m_GraphicsQueue, m_PresentQueue, chainImageIndex, primaryCommandBuffer);
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
	vkDeviceWaitIdle(m_Device);
	return true;
}