#include "KVulkanRenderDevice.h"
#include <assert.h>

const char* VALIDATION_LAYER[] =
{
	"VK_LAYER_KHRONOS_validation"
};
 
KVulkanRenderDevice::KVulkanRenderDevice()
	: mEnableValidationLayer(true)
{
	memset(&m_Instance, 0, sizeof(m_Instance));
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

std::vector<const char*> KVulkanRenderDevice::PrepareExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (mEnableValidationLayer)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return std::move(extensions);
}

bool KVulkanRenderDevice::Init()
{
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

	// 挂接验证层
	if(mEnableValidationLayer)
	{
		bool bCheckResult = CheckValidationLayerAvailable();
		assert(bCheckResult);
		createInfo.enabledLayerCount = sizeof(VALIDATION_LAYER) / sizeof(VALIDATION_LAYER[0]);
		createInfo.ppEnabledLayerNames = VALIDATION_LAYER;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	// 挂接扩展
	auto extensions = PrepareExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) == VK_SUCCESS)
	{
		if(PostInit())
			return true;
	}
	else
	{
		memset(&m_Instance, 0, sizeof(m_Instance));
	}
	return false;
}

bool KVulkanRenderDevice::UnInit()
{
	vkDestroyInstance(m_Instance, nullptr);
	return true;
}

bool KVulkanRenderDevice::PostInit()
{
	m_Extentions.clear();
	uint32_t extensionCount = 0;
	if(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) == VK_SUCCESS)
	{
		std::vector<VkExtensionProperties> extensions(extensionCount);
		if(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()) == VK_SUCCESS)
		{
			for(auto it = extensions.begin(), itEnd = extensions.end(); it != itEnd; ++it)
			{
				ExtensionProperties prop = { it->extensionName, it->specVersion };
				m_Extentions.push_back(std::move(prop));
			}
			return true;
		}
	}
	return false;	
}

bool KVulkanRenderDevice::QueryExtensions(DeviceExtensions& exts)
{
	exts = m_Extentions;
	return true;
}