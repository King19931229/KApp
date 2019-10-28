#include "KVulkanRenderDevice.h"
#include "KVulkanRenderWindow.h"

#include "KVulkanShader.h"
#include "KVulkanProgram.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"
#include "KVulkanUIOverlay.h"

#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"

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
	m_MultiThreadSumbit(true),
	m_Texture(nullptr),
	m_Sampler(nullptr)
{
	ZERO_ARRAY_MEMORY(m_Move);
	ZERO_ARRAY_MEMORY(m_MouseDown);
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
		(uint32_t)windowHeight));

	return true;
}

bool KVulkanRenderDevice::CreateImageViews()
{
	size_t imageCount	= m_pSwapChain->GetImageCount();
	VkExtent2D extend	= m_pSwapChain->GetExtent();
	VkFormat format		= m_pSwapChain->GetFormat();

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

	m_OffScreenTextures.resize(imageCount);
	for(size_t i = 0; i < m_OffScreenTextures.size(); ++i)
	{
		CreateTexture(m_OffScreenTextures[i]);
		m_OffScreenTextures[i]->InitMemeoryAsRT(extend.width, extend.height, EF_R16G16B16A16_FLOAT);
		m_OffScreenTextures[i]->InitDevice();
	}

	m_OffscreenRenderTargets.resize(imageCount);
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
		
	m_SkyBox.Init(this, renderTargets, "Textures/uffizi_cube.ktx");

	renderTargets.clear();
	m_SwapChainRenderTargets.resize(imageCount);
	for(size_t i = 0; i < m_SwapChainRenderTargets.size(); ++i)
	{
		CreateRenderTarget(m_SwapChainRenderTargets[i]);
		m_SwapChainRenderTargets[i]->SetColorClear(0.0f, 0.0f, 0.0f, 1.0f);
		m_SwapChainRenderTargets[i]->SetDepthStencilClear(1.0, 0);
		// TODO init from imageview
		m_SwapChainRenderTargets[i]->SetSize(extend.width, extend.height);

		ImageView imageView = {0};
		m_pSwapChain->GetImageView(i, imageView);

		m_SwapChainRenderTargets[i]->InitFromImageView(imageView, true, true, msaaCount);

		renderTargets.push_back(m_SwapChainRenderTargets[i].get());
	}

	CreateUIOVerlay(m_UIOverlay);

	m_UIOverlay->Init(this, renderTargets);
	m_UIOverlay->Resize(extend.width, extend.height);

	return true;
}

bool KVulkanRenderDevice::CreatePipelines()
{
	IKShaderPtr vertexShader = nullptr;
	IKShaderPtr fragmentShader = nullptr;

	{
		CreateShader(vertexShader);
		CreateShader(fragmentShader);

		ASSERT_RESULT(vertexShader->InitFromFile("Shaders/shader.vert") && fragmentShader->InitFromFile("Shaders/shader.frag"));

		m_OffscreenPipelines.resize(m_OffscreenRenderTargets.size());
		for(size_t i = 0; i < m_OffscreenRenderTargets.size(); ++i)
		{
			CreatePipeline(m_OffscreenPipelines[i]);

			IKPipelinePtr pipeline = m_OffscreenPipelines[i];

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

			pipeline->SetConstantBuffer(0, ST_VERTEX, m_CameraBuffers[i]);
			pipeline->SetSampler(1, m_Texture->GetImageView(), m_Sampler);
			pipeline->SetSampler(2, m_SkyBox.GetCubeTexture()->GetImageView(), m_SkyBox.GetSampler());

			pipeline->PushConstantBlock(m_ObjectConstant, m_ObjectConstantLoc);

			pipeline->Init(m_OffscreenRenderTargets[i].get());
		}

		vertexShader->UnInit();
		fragmentShader->UnInit();
	}

	{
		CreateShader(vertexShader);
		CreateShader(fragmentShader);

		ASSERT_RESULT(vertexShader->InitFromFile("Shaders/screenquad.vert") && fragmentShader->InitFromFile("Shaders/screenquad.frag"));

		m_SwapChainPipelines.resize(m_SwapChainRenderTargets.size());
		for(size_t i = 0; i < m_SwapChainRenderTargets.size(); ++i)
		{
			CreatePipeline(m_SwapChainPipelines[i]);

			IKPipelinePtr pipeline = m_SwapChainPipelines[i];

			VertexInputDetail bindingDetail = {};
			VertexFormat formats[] = {VF_SCREENQUAD_POS};
			bindingDetail.formats = formats;
			bindingDetail.count = ARRAY_SIZE(formats);

			pipeline->SetVertexBinding(&bindingDetail, 1);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetShader(ST_VERTEX, vertexShader);
			pipeline->SetShader(ST_FRAGMENT, fragmentShader);

			pipeline->SetBlendEnable(false);

			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);

			pipeline->SetSampler(0, m_OffScreenTextures[i]->GetImageView(), m_Sampler);
			pipeline->Init(m_SwapChainRenderTargets[i].get());
		}

		vertexShader->UnInit();
		fragmentShader->UnInit();
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

bool KVulkanRenderDevice::CreateUniform()
{
	m_ObjectConstant.shaderTypes = ST_VERTEX;
	m_ObjectConstant.size = (int)KConstantDefinition::GetConstantBufferDetail(CBT_OBJECT).bufferSize;

	m_CameraBuffers.resize(m_SwapChainRenderTargets.size());
	for(size_t i = 0; i < m_CameraBuffers.size(); ++i)
	{
		CreateUniformBuffer(m_CameraBuffers[i]);
		m_CameraBuffers[i]->InitMemory(KConstantDefinition::GetConstantBufferDetail(CBT_CAMERA).bufferSize,
			KConstantGlobal::GetGlobalConstantData(CBT_CAMERA));
		m_CameraBuffers[i]->InitDevice();
	}

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
					glm::vec3(0.0f, 1.0f, 0.0f)),
				glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0.0, yPos)),
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

	m_Texture->InitMemoryFromFile("Textures/texture.jpg", true);
	m_Texture->InitDevice();
	m_Texture->UnInit();

	m_Texture->InitMemoryFromFile("Textures/vulkan_11_rgba.ktx", true);
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

	m_pWindow = renderWindow;

	m_pWindow->RegisterKeyboardCallback(&m_KeyCallback);
	m_pWindow->RegisterMouseCallback(&m_MouseCallback);
	m_pWindow->RegisterScrollCallback(&m_ScrollCallback);

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
		if(!CreatePipelineCache())
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

		// Temporarily for demo use
		if(!CreateVertexInput())
			return false;
		if(!CreateUniform())
			return false;
		if(!CreateTex())
			return false;
		if(!CreatePipelines())
			return false;
#ifndef THREAD_MODE_ONE
		m_ThreadPool.PushWorkerThreads(std::thread::hardware_concurrency());
#else
		m_ThreadPool.SetThreadCount(std::thread::hardware_concurrency());
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
	CreateUniform();
	CreatePipelines();
	CreateCommandBuffers();

	return true;
}

bool KVulkanRenderDevice::CleanupSwapChain()
{
	if(m_UIOverlay)
	{
		m_UIOverlay->UnInit();
		m_UIOverlay = nullptr;
	}

	m_SkyBox.UnInit();
	
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

	// clear camera buffer
	for(size_t i = 0; i < m_CameraBuffers.size(); ++i)
	{
		m_CameraBuffers[i]->UnInit();
	}
	m_CameraBuffers.clear();

	// clear command buffers
	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		for (ThreadData& thread : m_CommandBuffers[i].threadDatas)
		{
			vkDestroyCommandPool(m_Device, thread.commandPool, nullptr);
		}
		vkDestroyCommandPool(m_Device, m_CommandBuffers[i].commandPool, nullptr);
	}
	m_CommandBuffers.clear();

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

	vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);

	vkDestroyCommandPool(m_Device, m_GraphicCommandPool, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	KVulkanHeapAllocator::UnInit();

	UnsetDebugMessenger();
	vkDestroyDevice(m_Device, nullptr);
	vkDestroyInstance(m_Instance, nullptr);

	PostUnInit();
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

bool KVulkanRenderDevice::PostInit()
{
	KVulkanGlobal::device = m_Device;
	KVulkanGlobal::physicalDevice = m_PhysicalDevice.device;
	KVulkanGlobal::graphicsCommandPool = m_GraphicCommandPool;
	KVulkanGlobal::graphicsQueue = m_GraphicsQueue;
	KVulkanGlobal::pipelineCache = m_PipelineCache;

	KVulkanGlobal::deviceReady = true;

	return true;
}

bool KVulkanRenderDevice::PostUnInit()
{
	KVulkanGlobal::deviceReady = false;

	KVulkanGlobal::device = VK_NULL_HANDLE;
	KVulkanGlobal::physicalDevice = VK_NULL_HANDLE;
	KVulkanGlobal::graphicsCommandPool = VK_NULL_HANDLE;
	KVulkanGlobal::graphicsQueue = VK_NULL_HANDLE;
	KVulkanGlobal::pipelineCache = VK_NULL_HANDLE;

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

bool KVulkanRenderDevice::CreateUIOVerlay(IKUIOverlayPtr& ui)
{
	ui = IKUIOverlayPtr(static_cast<IKUIOverlay*>(new KVulkanUIOverlay()));
	return true;
}

bool KVulkanRenderDevice::UpdateCamera(unsigned int idx)
{
	ASSERT_RESULT(idx < m_CameraBuffers.size());

	static KTimer m_MoveTimer;

	const float dt = m_MoveTimer.GetSeconds();
	const float moveSpeed = 300.0f;
	m_MoveTimer.Reset();

	m_Camera.MoveRight(dt * moveSpeed * m_Move[0]);
	m_Camera.Move(dt * moveSpeed * m_Move[1] * glm::vec3(0,1,0));
	m_Camera.MoveForward(dt * moveSpeed * m_Move[2]);

	VkExtent2D extend = m_pSwapChain->GetExtent();

	m_Camera.SetPerspective(glm::radians(45.0f), extend.width / (float) extend.height, 0.1f, 3000.0f);

	glm::mat4 view = m_Camera.GetViewMatrix();
	glm::mat4 proj = m_Camera.GetProjectiveMatrix();
	glm::mat4 viewInv = glm::inverse(view);

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
	m_CameraBuffers[idx]->Write(pData);

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

void KVulkanRenderDevice::ThreadRenderObject(uint32_t threadIndex, uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	ThreadData& threadData = m_CommandBuffers[imageIndex].threadDatas[threadIndex];

	// https://devblogs.nvidia.com/vulkan-dos-donts/ ResetCommandPool释放内存
	vkResetCommandPool(m_Device, threadData.commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	// 命令开始时候创建需要一个命令开始信息
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

#ifndef THREAD_MODE_ONE
	size_t numThread = m_ThreadPool.GetWorkerThreadNum();
#else
	size_t numThread = m_ThreadPool.GetThreadCount();
#endif

	VkCommandBuffer commandBuffer = threadData.commandBuffer;
	VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));	

	KVulkanRenderTarget* target = (KVulkanRenderTarget*)m_OffscreenRenderTargets[imageIndex].get();
	{
		KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_OffscreenPipelines[imageIndex].get();

		VkPipeline pipeline = vulkanPipeline->GetVkPipeline();
		VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
		VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

		// 绑定管线
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// 绑定管线布局
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// 绑定顶点缓冲
		KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_SqaureData.vertexBuffer.get();
		VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		// 绑定索引缓冲
		KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_SqaureData.indexBuffer.get();
		vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

		VkOffset2D offset = {0, 0};
		VkExtent2D extent = target->GetExtend();

		VkRect2D scissorRect = { offset, extent};
		VkViewport viewPort = 
		{
			0.0f,
			0.0f,
			(float)extent.width,
			(float)extent.height,
			0.0f,
			1.0f 
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

		for(size_t i = 0; i < threadData.num; ++i)
		{
			glm::mat4& model = m_ObjectFinalTransforms[i + threadData.offset];

			KAABBBox objectBox;
			m_Box.Transform(model, objectBox);
			if(m_Camera.CheckVisible(objectBox))
			{
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, m_ObjectConstantLoc.offset, m_ObjectConstant.size, &model);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
			}
		}
	}
	VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));
}

bool KVulkanRenderDevice::SubmitCommandBufferSingleThread(unsigned int imageIndex)
{
	assert(imageIndex < m_CommandBuffers.size());

	vkResetCommandPool(m_Device, m_CommandBuffers[imageIndex].commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// 开始渲染过程
	VkCommandBuffer commandBuffer = m_CommandBuffers[imageIndex].primaryCommandBuffer;
	VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));	
	{
		// 第一个RenderPass 绘制场景
		{
			KVulkanRenderTarget* target = (KVulkanRenderTarget*)m_OffscreenRenderTargets[imageIndex].get();

			// 创建开始渲染过程描述
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			// 指定渲染通道
			renderPassInfo.renderPass = target->GetRenderPass();
			// 指定帧缓冲
			renderPassInfo.framebuffer = target->GetFrameBuffer();

			VkOffset2D offset = {0, 0};
			VkExtent2D extent = target->GetExtend();

			renderPassInfo.renderArea.offset = offset;
			renderPassInfo.renderArea.extent = extent;

			// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致
			auto clearValuesPair = target->GetVkClearValues();
			renderPassInfo.pClearValues = clearValuesPair.first;
			renderPassInfo.clearValueCount = clearValuesPair.second;

			// 开始渲染天空盒
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				m_SkyBox.Draw(imageIndex, &commandBuffer);
			}
			// 开始渲染物件
			{
				KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_OffscreenPipelines[imageIndex].get();

				VkPipeline pipeline = vulkanPipeline->GetVkPipeline();
				VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
				VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

				// 绑定管线
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				// 绑定管线布局
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

				// 绑定顶点缓冲
				KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_SqaureData.vertexBuffer.get();
				VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				// 绑定索引缓冲
				KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_SqaureData.indexBuffer.get();
				vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

				VkRect2D scissorRect = { offset, extent};
				VkViewport viewPort = 
				{
					0.0f,
					0.0f,
					(float)extent.width,
					(float)extent.height,
					0.0f,
					1.0f 
				};
				vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

				// 绘制调用
				for(glm::mat4& model : m_ObjectFinalTransforms)
				{
					KAABBBox objectBox;
					m_Box.Transform(model, objectBox);
					if(m_Camera.CheckVisible(objectBox))
					{
						vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, m_ObjectConstantLoc.offset, m_ObjectConstant.size, &model);
						vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
					}
				}
			}
			// 结束渲染过程
			vkCmdEndRenderPass(commandBuffer);
		}
		// 第二个RenderPass 绘制ScreenQuad与UI
		{
			// 命令开始时候创建需要一个命令开始信息
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			beginInfo.pInheritanceInfo = nullptr;

			KVulkanRenderTarget* target = (KVulkanRenderTarget*)m_SwapChainRenderTargets[imageIndex].get();

			// 创建开始渲染过程描述
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			// 指定渲染通道
			renderPassInfo.renderPass = target->GetRenderPass();
			// 指定帧缓冲
			renderPassInfo.framebuffer = target->GetFrameBuffer();

			VkOffset2D offset = {0, 0};
			VkExtent2D extent = target->GetExtend();

			renderPassInfo.renderArea.offset = offset;
			renderPassInfo.renderArea.extent = extent;

			// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致
			auto clearValuesPair = target->GetVkClearValues();
			renderPassInfo.pClearValues = clearValuesPair.first;
			renderPassInfo.clearValueCount = clearValuesPair.second;

			// 开始渲染过程
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_SwapChainPipelines[imageIndex].get();

				VkPipeline pipeline = vulkanPipeline->GetVkPipeline();
				VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
				VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

				// 绑定管线
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				// 绑定管线布局
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

				// 绑定顶点缓冲
				KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_QuadData.vertexBuffer.get();
				VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				// 绑定索引缓冲
				KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_QuadData.indexBuffer.get();
				vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

				VkOffset2D offset = {0, 0};
				VkExtent2D extent = target->GetExtend();

				VkRect2D scissorRect = { offset, extent};
				VkViewport viewPort = 
				{
					0.0f,
					0.0f,
					(float)extent.width,
					(float)extent.height,
					0.0f,
					1.0f 
				};
				vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);
			}
			{
				m_UIOverlay->Draw(imageIndex, &commandBuffer);
			}
			// 结束渲染过程
			vkCmdEndRenderPass(commandBuffer);
		}
	}
	VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));

	return true;
}

bool KVulkanRenderDevice::SubmitCommandBufferMuitiThread(unsigned int imageIndex)
{
	assert(imageIndex < m_CommandBuffers.size());

	vkResetCommandPool(m_Device, m_CommandBuffers[imageIndex].commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	KVulkanRenderTarget* offscreenTarget = (KVulkanRenderTarget*)m_OffscreenRenderTargets[imageIndex].get();
	KVulkanRenderTarget* swapChainTarget = (KVulkanRenderTarget*)m_SwapChainRenderTargets[imageIndex].get();

	VkCommandBuffer primaryCommandBuffer = m_CommandBuffers[imageIndex].primaryCommandBuffer;
	VkCommandBuffer skyBoxCommandBuffer = m_CommandBuffers[imageIndex].skyBoxCommandBuffer;	
	VkCommandBuffer uiCommandBuffer = m_CommandBuffers[imageIndex].uiCommandBuffer;
	VkCommandBuffer postprocessCommandBuffer = m_CommandBuffers[imageIndex].postprocessCommandBuffer;

	// 开始渲染过程
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_ASSERT_RESULT(vkBeginCommandBuffer(primaryCommandBuffer, &cmdBufferBeginInfo));
	{
		{
			// 创建开始渲染过程描述
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			// 指定渲染通道
			renderPassInfo.renderPass = offscreenTarget->GetRenderPass();
			// 指定帧缓冲
			renderPassInfo.framebuffer = offscreenTarget->GetFrameBuffer();

			renderPassInfo.renderArea.offset.x = 0;
			renderPassInfo.renderArea.offset.y = 0;
			renderPassInfo.renderArea.extent = offscreenTarget->GetExtend();

			// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致
			auto clearValuesPair = offscreenTarget->GetVkClearValues();
			renderPassInfo.pClearValues = clearValuesPair.first;
			renderPassInfo.clearValueCount = clearValuesPair.second;

			vkCmdBeginRenderPass(primaryCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
			{
				VkCommandBufferInheritanceInfo inheritanceInfo = {};
				inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritanceInfo.renderPass = offscreenTarget->GetRenderPass();
				inheritanceInfo.framebuffer = offscreenTarget->GetFrameBuffer();

				auto commandBuffers = m_CommandBuffers[imageIndex].commandBuffersExec;
				commandBuffers.clear();

				{
					// 命令开始时候创建需要一个命令开始信息
					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
					beginInfo.pInheritanceInfo = &inheritanceInfo;

					VK_ASSERT_RESULT(vkBeginCommandBuffer(skyBoxCommandBuffer, &beginInfo));
					{
						m_SkyBox.Draw(imageIndex, &skyBoxCommandBuffer);
					}
					VK_ASSERT_RESULT(vkEndCommandBuffer(skyBoxCommandBuffer));

					commandBuffers.push_back(skyBoxCommandBuffer);
				}

#ifndef THREAD_MODE_ONE
				for(size_t i = 0; i < m_ThreadPool.GetWorkerThreadNum(); ++i)
				{
					m_ThreadPool.SubmitTask([=]()
					{
						ThreadRenderObject((uint32_t)i, imageIndex, inheritanceInfo);
					});
				}
				m_ThreadPool.WaitAllAsyncTaskDone();
#else
				for(size_t i = 0; i < m_ThreadPool.GetThreadCount(); ++i)
				{
					m_ThreadPool.AddJob(i, [=]()
					{
						ThreadRenderObject((uint32_t)i, imageIndex, inheritanceInfo);
					});
				}

				m_ThreadPool.WaitAll();
#endif
#ifndef THREAD_MODE_ONE
				size_t numThread = m_ThreadPool.GetWorkerThreadNum();
#else
				size_t numThread = m_ThreadPool.GetThreadCount();
#endif
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

		{
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			// 指定渲染通道
			renderPassInfo.renderPass = swapChainTarget->GetRenderPass();
			// 指定帧缓冲
			renderPassInfo.framebuffer = swapChainTarget->GetFrameBuffer();

			renderPassInfo.renderArea.offset.x = 0;
			renderPassInfo.renderArea.offset.y = 0;
			renderPassInfo.renderArea.extent = swapChainTarget->GetExtend();

			// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致
			auto clearValuesPair = swapChainTarget->GetVkClearValues();
			renderPassInfo.pClearValues = clearValuesPair.first;
			renderPassInfo.clearValueCount = clearValuesPair.second;
			vkCmdBeginRenderPass(primaryCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
			{
				VkCommandBufferInheritanceInfo inheritanceInfo = {};
				inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritanceInfo.renderPass = swapChainTarget->GetRenderPass();
				inheritanceInfo.framebuffer = swapChainTarget->GetFrameBuffer();

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
				beginInfo.pInheritanceInfo = &inheritanceInfo;
				VK_ASSERT_RESULT(vkBeginCommandBuffer(uiCommandBuffer, &beginInfo));
				{
					VkOffset2D offset = {0, 0};
					VkExtent2D extent = swapChainTarget->GetExtend();

					VkRect2D scissorRect = { offset, extent};
					VkViewport viewPort = 
					{
						0.0f,
						0.0f,
						(float)extent.width,
						(float)extent.height,
						0.0f,
						1.0f 
					};
					vkCmdSetViewport(uiCommandBuffer, 0, 1, &viewPort);
					vkCmdSetScissor(uiCommandBuffer, 0, 1, &scissorRect);

					m_UIOverlay->Draw(imageIndex, &uiCommandBuffer);
					VK_ASSERT_RESULT(vkEndCommandBuffer(uiCommandBuffer));
				}

				VK_ASSERT_RESULT(vkBeginCommandBuffer(postprocessCommandBuffer, &beginInfo));
				{
					KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_SwapChainPipelines[imageIndex].get();

					VkPipeline pipeline = vulkanPipeline->GetVkPipeline();
					VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
					VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

					// 绑定管线
					vkCmdBindPipeline(postprocessCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
					// 绑定管线布局
					vkCmdBindDescriptorSets(postprocessCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

					// 绑定顶点缓冲
					KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_QuadData.vertexBuffer.get();
					VkBuffer vertexBuffers[] = {vulkanVertexBuffer->GetVulkanHandle()};
					VkDeviceSize offsets[] = {0};
					vkCmdBindVertexBuffers(postprocessCommandBuffer, 0, 1, vertexBuffers, offsets);
					// 绑定索引缓冲
					KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_QuadData.indexBuffer.get();
					vkCmdBindIndexBuffer(postprocessCommandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

					VkOffset2D offset = {0, 0};
					VkExtent2D extent = swapChainTarget->GetExtend();

					VkRect2D scissorRect = { offset, extent};
					VkViewport viewPort = 
					{
						0.0f,
						0.0f,
						(float)extent.width,
						(float)extent.height,
						0.0f,
						1.0f 
					};
					vkCmdSetViewport(postprocessCommandBuffer, 0, 1, &viewPort);
					vkCmdSetScissor(postprocessCommandBuffer, 0, 1, &scissorRect);
					vkCmdDrawIndexed(postprocessCommandBuffer, static_cast<uint32_t>(vulkanIndexBuffer->GetIndexCount()), 1, 0, 0, 0);

					VK_ASSERT_RESULT(vkEndCommandBuffer(postprocessCommandBuffer));
				}

				VkCommandBuffer commandBuffers[] = {postprocessCommandBuffer, uiCommandBuffer};
				vkCmdExecuteCommands(primaryCommandBuffer, ARRAY_SIZE(commandBuffers), commandBuffers);
			}
			vkCmdEndRenderPass(primaryCommandBuffer);
		}
	}
	VK_ASSERT_RESULT(vkEndCommandBuffer(primaryCommandBuffer));

	return true;
}

bool KVulkanRenderDevice::CreateCommandBuffers()
{
	// 交换链上的每个帧缓冲都需要提交命令
	m_CommandBuffers.resize(m_SwapChainRenderTargets.size());

#ifndef THREAD_MODE_ONE
	size_t numThread = m_ThreadPool.GetWorkerThreadNum();
#else
	size_t numThread = m_ThreadPool.GetThreadCount();
#endif

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_ASSERT_RESULT(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandBuffers[i].commandPool));

		// 创建主命令缓冲
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = m_CommandBuffers[i].commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffers[i].primaryCommandBuffer);
		}

		// 创建天空盒子命令缓冲
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = m_CommandBuffers[i].commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffers[i].skyBoxCommandBuffer);
		}

		// 创建UI命令缓冲
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = m_CommandBuffers[i].commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffers[i].uiCommandBuffer);
		}

		// 创建后处理命令缓冲
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = m_CommandBuffers[i].commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffers[i].postprocessCommandBuffer);
		}

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

			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = m_PhysicalDevice.queueFamilyIndices.graphicsFamily.first;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VK_ASSERT_RESULT(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &threadData.commandPool));

			threadData.num = numPerThread + ((threadIdx == numThread - 1) ? numRemain : 0);
			threadData.offset = numPerThread * threadIdx;

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
	VkResult vkResult;

	vkResult = m_pSwapChain->WaitForInfightFrame();
	VK_ASSERT_RESULT(vkResult);

	uint32_t imageIndex = 0;
	vkResult = m_pSwapChain->AcquireNextImage(imageIndex);

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return true;
	}
	else if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
	{
		return false;
	}

	UpdateCamera(imageIndex);
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
			}
			m_UIOverlay->PopItemWidth();
		}
		m_UIOverlay->End();
	}
	m_UIOverlay->EndNewFrame();

	m_UIOverlay->Update(imageIndex);

	if(m_MultiThreadSumbit)
	{
		SubmitCommandBufferMuitiThread(imageIndex);
	}
	else
	{
		SubmitCommandBufferSingleThread(imageIndex);
	}

	VkCommandBuffer primaryCommandBuffer = m_CommandBuffers[imageIndex].primaryCommandBuffer;
	vkResult = m_pSwapChain->PresentQueue(m_GraphicsQueue, m_PresentQueue, imageIndex, primaryCommandBuffer);
	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
	}
	return true;
}

bool KVulkanRenderDevice::Wait()
{
	vkDeviceWaitIdle(m_Device);
	return true;
}