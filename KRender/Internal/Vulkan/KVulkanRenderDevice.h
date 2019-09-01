#pragma once
#include "Interface/IKRenderDevice.h"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <algorithm>

class KVulkanRenderWindow;

class KVulkanRenderDevice : IKRenderDevice
{
	struct ExtensionProperties
	{
		std::string property;
		unsigned int specVersion;
	};

	struct QueueFamilyIndices
	{
		typedef std::pair<uint32_t, bool> QueueFamilyIndex;
		QueueFamilyIndex graphicsFamily;
		QueueFamilyIndex presentFamily;

		inline bool IsComplete()
		{
			return graphicsFamily.second && presentFamily.second;
		}
	};

	struct PhysicalDevice
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		QueueFamilyIndices queueFamilyIndices;

		bool suitable;
		int score;
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
protected:
	VkInstance m_Instance;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface;

	VkDebugUtilsMessengerEXT m_DebugMessenger;

	PhysicalDevice m_PhysicalDevice;
	bool m_EnableValidationLayer;

	SwapChainSupportDetails	QuerySwapChainSupport(VkPhysicalDevice device);

	bool CheckValidationLayerAvailable();
	bool SetupDebugMessenger();
	bool UnsetDebugMessenger();

	bool CheckDeviceSuitable(PhysicalDevice& device);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckExtentionsSupported(VkPhysicalDevice device);
	PhysicalDevice GetPhysicalDeviceProperty(VkPhysicalDevice device);

	bool CreateSurface(KVulkanRenderWindow* window);
	bool PickPhysicsDevice();
	bool CreateLogicalDevice();

	bool PostInit();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
		);

public:
	KVulkanRenderDevice();
	virtual ~KVulkanRenderDevice();

	virtual bool Init(IKRenderWindow* window);
	virtual bool UnInit();
};