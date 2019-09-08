#pragma once
#include "Interface/IKRenderDevice.h"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <algorithm>
#include <vector>

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
	VkInstance m_Instance;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface;
	bool m_EnableValidationLayer;
	// Temporarily for demo use
	IKShaderPtr m_VSShader;
	IKShaderPtr m_FGShader;
	IKProgramPtr m_Program;

	VkRenderPass m_RenderPass;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	//
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	PhysicalDevice m_PhysicalDevice;

	VkSwapchainKHR  m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

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
	VkExtent2D ChooseSwapExtent(KVulkanRenderWindow* window);

	bool CreateSurface(KVulkanRenderWindow* window);
	bool PickPhysicsDevice();
	bool CreateLogicalDevice();
	bool CreateSwapChain(KVulkanRenderWindow* window);
	bool CreateImageViews();
	bool CreateFramebuffers();
	bool CreateCommandPool();

	// Temporarily for demo use
	bool CreateRenderPass();
	bool CreateGraphicsPipeline();
	bool CreateCommandBuffers();

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

	virtual bool Init(IKRenderWindowPtr window);
	virtual bool UnInit();

	virtual bool CreateShader(IKShaderPtr& shader);
	virtual bool CreateProgram(IKProgramPtr& program);
};