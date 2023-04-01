#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKRenderWindow.h"
#include "Internal/KRenderThreadPool.h"

#include "KVulkanHeapAllocator.h"
#include "KVulkanHelper.h"
#include "KVulkanSwapChain.h"

#include "Internal/Object/KSkyBox.h"
#include "Internal/Shadow/KShadowMap.h"

#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"

#include "Internal/KRenderGlobal.h"

#include "Publish/KCamera.h"
#include "Interface/IKGizmo.h"

#include <algorithm>
#include <vector>

#ifndef __ANDROID__
#include "GLFW/glfw3.h"
#else

#endif

class KVulkanRenderDevice : public IKRenderDevice
{
	struct PhysicalDevice
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;

		std::vector<uint32_t> graphicsFamilyIndices;
		std::vector<uint32_t> computeFamilyIndices;
		std::vector<uint32_t> transferFamilyIndices;

		std::vector<std::string> supportedExtensions;

		bool queueComplete;
		bool suitable;
		bool supportNvExtension;
		bool supportRaytraceExtension;
		bool supportMeshShaderExtension;
		bool supportDebugMarker;
		int score;

		PhysicalDevice()
		{
			device = VK_NULL_HANDEL;
			deviceProperties = {};
			deviceFeatures = {};
			queueFamilyProperties = {};
			graphicsFamilyIndices = {};
			computeFamilyIndices = {};
			transferFamilyIndices = {};
			supportedExtensions = {};
			queueComplete = false;
			suitable = false;
			supportNvExtension = false;
			supportRaytraceExtension = false;
			supportMeshShaderExtension = false;
			supportDebugMarker = false;
			score = 0;
		}
	};
protected:
	KRenderDeviceProperties m_Properties;
	IKRenderWindow* m_pWindow;	
	VkInstance m_Instance;
	VkDevice m_Device;

	VkPipelineCache m_PipelineCache;
	VkCommandPool m_GraphicCommandPool;
	int32_t m_ValidationLayerIdx;
	bool m_EnableValidationLayer;

	VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
	VkDebugReportCallbackEXT m_DebugReportCallback;
	PhysicalDevice m_PhysicalDevice;

	IKSwapChainPtr m_SwapChain;
	std::vector<IKSwapChain*> m_SecordarySwapChains;
	IKUIOverlayPtr m_UIOverlay;

	void* m_GpuCrashTracker;

	typedef std::unordered_set<KDevicePresentCallback*> PresentCallbackSet;
	PresentCallbackSet m_PrePresentCallback;
	PresentCallbackSet m_PostPresentCallback;

	typedef std::unordered_set<KSwapChainRecreateCallback*> SwapChainCallbackSet;
	SwapChainCallbackSet m_SwapChainCallback;

	typedef std::unordered_set<KDeviceInitCallback*> DeviceInitCallbackSet;
	DeviceInitCallbackSet m_InitCallback;

	typedef std::unordered_set<KDeviceUnInitCallback*> DeviceUnInitCallbackSet;
	DeviceUnInitCallbackSet m_UnInitCallback;

	bool PopulateInstanceExtensions(std::vector<const char*>& extensions);
	bool CheckValidationLayerAvailable(int32_t& candidateIdx);
	bool SetupDebugMessenger();
	bool UnsetDebugMessenger();

	bool InitHeapAllocator();
	bool UnInitHeapAllocator();

	bool CheckDeviceSuitable(PhysicalDevice& device);

	bool FindQueue(PhysicalDevice& device, VkQueueFlags bits, const std::vector<uint32_t>& forbidQueueFamilies, uint32_t& queueFamily);
	bool CheckExtentionsSupported(PhysicalDevice& device);
	PhysicalDevice GetPhysicalDeviceProperty(VkPhysicalDevice device);

	void* GetEnabledFeatures();

	bool CreateUIOverlay(IKUIOverlayPtr& ui);

	bool PickPhysicsDevice();
	bool CreateLogicalDevice();
	bool CreatePipelineCache();
	bool CreateCommandPool();

	bool InitSwapChain();
	bool CleanupSwapChain();

	bool InitDeviceGlobal();
	bool UnInitDeviceGlobal();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		size_t location,
		int32_t messageCode,
		const char *pLayerPrefix,
		const char *pMessage,
		void *pUserData
	);

public:
	KVulkanRenderDevice();
	virtual ~KVulkanRenderDevice();

	virtual bool Init(IKRenderWindow* window);
	virtual bool UnInit();

	virtual bool CreateSemaphore(IKSemaphorePtr& semaphore);
	virtual bool CreateQueue(IKQueuePtr& queue);

	virtual bool CreateShader(IKShaderPtr& shader);

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer);
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer);
	virtual bool CreateStorageBuffer(IKStorageBufferPtr& buffer);
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer);

	virtual bool CreateAccelerationStructure(IKAccelerationStructurePtr& as);

	virtual bool CreateTexture(IKTexturePtr& texture);
	virtual bool CreateSampler(IKSamplerPtr& sampler);

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target);
	virtual bool CreatePipeline(IKPipelinePtr& pipeline);
	virtual bool CreateRayTracePipeline(IKRayTracePipelinePtr& raytrace);

	virtual bool CreateComputePipeline(IKComputePipelinePtr& compute);

	virtual bool CreateCommandPool(IKCommandPoolPtr& pool);
	virtual bool CreateCommandBuffer(IKCommandBufferPtr& buffer);

	virtual bool CreateQuery(IKQueryPtr& query);
	virtual bool CreateSwapChain(IKSwapChainPtr& swapChain);

	virtual bool CreateRenderPass(IKRenderPassPtr& renderPass);

	virtual bool Present();
	virtual bool Wait();

	virtual bool RecreateSwapChain(IKSwapChain* swapChain, IKUIOverlay* ui);

	virtual bool RegisterPrePresentCallback(KDevicePresentCallback* callback);
	virtual bool UnRegisterPrePresentCallback(KDevicePresentCallback* callback);
	virtual bool RegisterPostPresentCallback(KDevicePresentCallback* callback);
	virtual bool UnRegisterPostPresentCallback(KDevicePresentCallback* callback);

	virtual bool RegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback);
	virtual bool UnRegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback);

	virtual bool RegisterDeviceInitCallback(KDeviceInitCallback* callback);
	virtual bool UnRegisterDeviceInitCallback(KDeviceInitCallback* callback);

	virtual bool RegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback);
	virtual bool UnRegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback);

	virtual bool RegisterSecordarySwapChain(IKSwapChain* swapChain);
	virtual bool UnRegisterSecordarySwapChain(IKSwapChain* swapChain);

	virtual bool QueryProperty(KRenderDeviceProperties** ppProperty);

	virtual IKSwapChain* GetSwapChain();
	virtual IKRenderWindow* GetMainWindow();
	virtual IKUIOverlay* GetUIOverlay();

	virtual bool SetCheckPointMarker(IKCommandBuffer* commandBuffer, uint32_t frameNum, const char* marker);
};