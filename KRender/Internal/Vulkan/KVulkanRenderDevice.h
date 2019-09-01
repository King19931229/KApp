#pragma once
#include "Interface/IKRenderDevice.h"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

class KVulkanRenderDevice : IKRenderDevice
{
	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily;
		bool graphicsFamilyFound;
	};

	struct PhysicalDevice
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		bool suitable;
		int score;
	};
protected:
	VkInstance m_Instance;
	VkDevice m_Device;
	VkDebugUtilsMessengerEXT m_DebugMessenger;

	PhysicalDevice m_PhysicalDevice;
	DeviceExtensions m_Extentions;
	bool m_EnableValidationLayer;

	bool CheckValidationLayerAvailable();
	bool SetupDebugMessenger();
	bool UnsetDebugMessenger();

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	PhysicalDevice GetPhysicalDeviceProperty(VkPhysicalDevice device);

	bool PickPhysicsDevice();
	bool CreateLogicalDevice();
	bool RecordExtentions();

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

	virtual bool Init();
	virtual bool UnInit();

	virtual bool QueryExtensions(DeviceExtensions& exts);
};