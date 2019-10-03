#pragma once
#include "Interface/IKRenderDevice.h"
#include "KVulkanHeapAllocator.h"
#include "KVulkanHelper.h"
#include "KVulkanDepthBuffer.h"

#include "KBase/Publish/KThreadPool.h"
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

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct PhysicalDevice
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		QueueFamilyIndices queueFamilyIndices;
		SwapChainSupportDetails swapChainSupportDetails;

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
	typedef std::vector<VkCommandBuffer> VkCommandBufferList;
	struct ThreadData
	{
		VkCommandBufferList commandBuffers;
		size_t counting;
	};
	struct CommandBuffer
	{
		VkCommandBuffer primaryCommandBuffer;
		std::vector<ThreadData> threadDatas;
	};
	std::vector<CommandBuffer> m_CommandBuffers;
	KRenderThreadPool m_RenderThreadPool;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	struct ObjectTransform
	{
		glm::mat4 initRotate;
		glm::mat4 initTranslate;
		glm::mat4 rotate;
		glm::mat4 translate;
	};
	std::vector<ObjectTransform> m_ObjectTransforms;

	IKUniformBufferPtr m_ObjectBuffer;
	IKUniformBufferPtr m_CameraBuffer;

	IKTexturePtr m_Texture;
	IKSamplerPtr m_Sampler;
	//
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	PhysicalDevice m_PhysicalDevice;

	VkSwapchainKHR  m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	std::vector<IKRenderTargetPtr> m_SwapChainRenderTargets;
	std::vector<IKPipelinePtr> m_SwapChainPipelines;

	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	size_t m_MaxFramesInFight;
	size_t m_CurrentFlightIndex;

	VkFormat m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;

	VkCommandPool m_CommandPool;

	SwapChainSupportDetails	QuerySwapChainSupport(VkPhysicalDevice device);

	bool CheckValidationLayerAvailable();
	bool SetupDebugMessenger();
	bool UnsetDebugMessenger();

	bool CheckDeviceSuitable(PhysicalDevice& device);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckExtentionsSupported(VkPhysicalDevice device);
	PhysicalDevice GetPhysicalDeviceProperty(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat();
	VkPresentModeKHR ChooseSwapPresentMode();
	VkExtent2D ChooseSwapExtent();

	bool CreateSurface();
	bool PickPhysicsDevice();
	bool CreateLogicalDevice();
	bool CreateSwapChain();
	bool CreateImageViews();
	bool CreatePipelines();
	bool CreateCommandPool();
	bool CreateSyncObjects();

	bool DestroySyncObjects();

	// Temporarily for demo use
	bool CreateVertexInput();
	bool CreateUniform();
	bool CreateTex();
	bool CreateCommandBuffers();
	bool UpdateCommandBuffer(unsigned int idx);

	bool UpdateCamera();
	bool UpdateObjectTransform();

	void ThreadRenderObject(uint32_t threadIndex, uint32_t imageIndex, size_t objectIndex);

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