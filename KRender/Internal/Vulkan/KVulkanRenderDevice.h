#pragma once
#include "Interface/IKRenderDevice.h"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

class KVulkanRenderDevice : IKRenderDevice
{
	VkInstance m_Instance;
	DeviceExtensions m_Extentions;

	bool mEnableValidationLayer;

	bool CheckValidationLayerAvailable();
	std::vector<const char*> PrepareExtensions();

	bool PostInit();
public:
	KVulkanRenderDevice();
	virtual ~KVulkanRenderDevice();

	virtual bool Init();
	virtual bool UnInit();

	virtual bool QueryExtensions(DeviceExtensions& exts);
};