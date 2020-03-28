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
#include "Publish/KAABBBox.h"
#include "Interface/IKGizmo.h"

#include <algorithm>
#include <vector>

#ifndef __ANDROID__
#include "GLFW/glfw3.h"
#else

#endif

class KVulkanRenderDevice : IKRenderDevice
{
	struct QueueFamilyIndices
	{
		typedef std::pair<uint32_t, bool> QueueFamilyIndex;
		QueueFamilyIndex graphicsFamily;
		QueueFamilyIndex presentFamily;

		inline bool IsComplete() const
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
		std::vector<std::string> supportedExtensions;

		bool suitable;
		int score;
	};
protected:
	IKRenderWindow* m_pWindow;
	VkInstance m_Instance;
	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
	VkSurfaceKHR m_Surface;
	VkPipelineCache m_PipelineCache;
	VkCommandPool m_GraphicCommandPool;
	int32_t m_ValidationLayerIdx;
	bool m_EnableValidationLayer;

	uint32_t m_FrameInFlight;

	typedef std::unordered_set<KDevicePresentCallback*> PresentCallbackSet;
	PresentCallbackSet m_PresentCallback;

	VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
	VkDebugReportCallbackEXT m_DebugReportCallback;
	PhysicalDevice m_PhysicalDevice;

	IKSwapChainPtr m_SwapChain;
	IKUIOverlayPtr m_UIOverlay;

	bool PopulateInstanceExtensions(std::vector<const char*>& extensions);
	bool CheckValidationLayerAvailable(int32_t& candidateIdx);
	bool SetupDebugMessenger();
	bool UnsetDebugMessenger();

	bool InitGlobalManager();
	bool UnInitGlobalManager();

	bool CheckDeviceSuitable(PhysicalDevice& device);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckExtentionsSupported(PhysicalDevice& device);
	PhysicalDevice GetPhysicalDeviceProperty(VkPhysicalDevice device);

	bool CreateSurface();
	bool PickPhysicsDevice();
	bool CreateLogicalDevice();
	bool CreatePipelineCache();
	bool CreateSwapChain();
	bool CreateCommandPool();

	bool CreateMesh();
	bool CreateUI();
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

	virtual bool CreateShader(IKShaderPtr& shader);

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer);
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer);
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer);

	virtual bool CreateTexture(IKTexturePtr& texture);
	virtual bool CreateSampler(IKSamplerPtr& sampler);
	virtual bool CreateSwapChain(IKSwapChainPtr& swapChain);

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target);
	virtual bool CreatePipeline(IKPipelinePtr& pipeline);
	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle);

	virtual bool CreateUIOverlay(IKUIOverlayPtr& ui);

	virtual bool CreateCommandPool(IKCommandPoolPtr& pool);
	virtual bool CreateCommandBuffer(IKCommandBufferPtr& buffer);
	
	virtual bool Present();
	virtual bool Wait();

	virtual bool RegisterPresentCallback(KDevicePresentCallback* callback);
	virtual bool UnRegisterPresentCallback(KDevicePresentCallback* callback);

	virtual bool RecreateSwapChain();

	virtual IKSwapChainPtr GetCurrentSwapChain();
	virtual IKUIOverlayPtr GetCurrentUIOverlay();
	virtual uint32_t GetFrameInFlight();
};