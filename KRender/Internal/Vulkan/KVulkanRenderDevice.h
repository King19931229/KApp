#pragma once
#include "Interface/IKRenderDevice.h"
#include "KVulkanHeapAllocator.h"
#include "KVulkanHelper.h"
#include "KVulkanSwapChain.h"

#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "Internal/KRenderThreadPool.h"

#include "GLFW/glfw3.h"
#include <algorithm>
#include <vector>

class KVulkanRenderWindow;
class KVulkanDepthBuffer;

class KVulkanRenderDevice : IKRenderDevice
{
	friend KVulkanRenderWindow;
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

		bool suitable;
		int score;
	};
protected:
	KVulkanRenderWindow* m_pWindow;
	VkInstance m_Instance;
	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
	VkSurfaceKHR m_Surface;
	bool m_EnableValidationLayer;
	// Temporarily for demo use
	bool m_MultiThreadSumbit;
	typedef std::vector<VkCommandBuffer> VkCommandBufferList;
	VkCommandPool m_CommandPool;

	struct ThreadData
	{
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
		size_t num;
		size_t offset;
	};

	struct CommandBuffer
	{
		VkCommandBuffer primaryCommandBuffer;
		std::vector<ThreadData> threadDatas;
		VkCommandBufferList commandBuffersExec;
	};
	std::vector<CommandBuffer> m_CommandBuffers;

#ifndef THREAD_MODE_ONE
	KThreadPool<std::function<void()>, true> m_ThreadPool;
#else
	KRenderThreadPool m_ThreadPool;
#endif
	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	KVulkanSwapChainPtr m_pSwapChain;

	struct ObjectInitTransform
	{
		glm::mat4 rotate;
		glm::mat4 translate;
	};
	std::vector<ObjectInitTransform> m_ObjectTransforms;
	std::vector<glm::mat4> m_ObjectFinalTransforms;

	IKUniformBufferPtr m_ObjectBuffer;
	IKUniformBufferPtr m_CameraBuffer;

	IKTexturePtr m_Texture;
	IKSamplerPtr m_Sampler;
	
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	PhysicalDevice m_PhysicalDevice;

	std::vector<IKRenderTargetPtr> m_SwapChainRenderTargets;
	std::vector<IKPipelinePtr> m_SwapChainPipelines;

	bool CheckValidationLayerAvailable();
	bool SetupDebugMessenger();
	bool UnsetDebugMessenger();

	bool CheckDeviceSuitable(PhysicalDevice& device);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckExtentionsSupported(VkPhysicalDevice device);
	PhysicalDevice GetPhysicalDeviceProperty(VkPhysicalDevice device);

	bool CreateSurface();
	bool PickPhysicsDevice();
	bool CreateLogicalDevice();
	bool CreateSwapChain();
	bool CreateImageViews();
	bool CreatePipelines();
	bool CreateCommandPool();

	// Temporarily for demo use
	bool CreateVertexInput();
	bool CreateUniform();
	bool CreateTex();
	bool CreateCommandBuffers();
	bool SubmitCommandBufferSingleThread(unsigned int idx);
	bool SubmitCommandBufferMuitiThread(unsigned int idx);
	bool UpdateFrameTime();

	bool UpdateCamera();
	bool UpdateObjectTransform();

	void ThreadRenderObject(uint32_t threadIndex, uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo);

	bool RecreateSwapChain();
	bool CleanupSwapChain();

	// Sync global variable
	bool PostInit();
	bool PostUnInit();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
		);

public:
	KVulkanRenderDevice();
	virtual ~KVulkanRenderDevice();

	virtual bool Init(IKRenderWindowPtr window);
	virtual bool UnInit();

	virtual bool CreateShader(IKShaderPtr& shader);
	virtual bool CreateProgram(IKProgramPtr& program);

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer);
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer);
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer);

	virtual bool CreateTexture(IKTexturePtr& texture);
	virtual bool CreateSampler(IKSamplerPtr& sampler);

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target);

	virtual bool CreatePipeline(IKPipelinePtr& pipeline);

	virtual bool Present();

	bool Wait();
};