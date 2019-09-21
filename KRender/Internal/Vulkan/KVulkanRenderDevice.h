#pragma once
#include "Interface/IKRenderDevice.h"
#include "KVulkanHelper.h"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <algorithm>
#include <vector>

class KVulkanRenderWindow;

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
	KVulkanHelper::VulkanBindingDetailList m_VertexBindDetailList;
	VkRenderPass m_RenderPass;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorPool  m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;
	std::vector<IKUniformBufferPtr> m_UniformBuffers;
	//
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	PhysicalDevice m_PhysicalDevice;

	VkSwapchainKHR  m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

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
	bool CreateCommandPool();
	bool CreateSyncObjects();

	bool DestroySyncObjects();

	// Temporarily for demo use
	bool CreateVertexInput();
	bool CreateUniform();
	bool CreateDescriptorPool();
	bool CreateDescriptorSets();
	bool CreateRenderPass();
	bool CreateGraphicsPipeline();
	bool CreateFramebuffers();
	bool CreateCommandBuffers();
	bool CreateDescriptorSetLayout();

	bool UpdateUniformBuffer(uint32_t currentImage);

	bool RecreateSwapChain();
	bool CleanupSwapChain();

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

	virtual bool Present();

	bool Wait();
};