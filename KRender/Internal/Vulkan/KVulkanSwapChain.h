#pragma once
#include "KVulkanConfig.h"
#include <vector>
#include <memory>

class KVulkanSwapChain
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
protected:
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;

	VkSwapchainKHR  m_SwapChain;

	VkExtent2D m_Extend;
	VkSurfaceKHR m_Surface;

	VkPresentModeKHR m_PresentMode;
	VkSurfaceFormatKHR m_SurfaceFormat;

	size_t m_MaxFramesInFight;
	size_t m_CurrentFlightIndex;

	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	SwapChainSupportDetails m_SwapChainSupportDetails;

	bool QuerySwapChainSupport();
	bool ChooseSwapSurfaceFormat();
	bool ChooseSwapPresentMode();
	bool ChooseSwapExtent(uint32_t windowWidth, uint32_t windowHeight);

	bool CreateSwapChain(uint32_t windowWidth, uint32_t windowHeight,
		uint32_t graphIndex, uint32_t presentIndex);
	bool CreateSyncObjects();

	bool CleanupSwapChain();
	bool DestroySyncObjects();
public:
	KVulkanSwapChain();
	~KVulkanSwapChain();

	bool Init(VkDevice device,
		VkPhysicalDevice physicalDevice,
		uint32_t graphIndex,
		uint32_t presentIndex,
		VkSurfaceKHR surface,
		uint32_t windowWidth,
		uint32_t windowHeight);

	bool UnInit();

	VkResult WaitForInfightFrame();
	VkResult AcquireNextImage(uint32_t& imageIndex);
	VkResult PresentQueue(VkQueue graphicsQueue, VkQueue presentQueue, uint32_t imageIndex, VkCommandBuffer commandBuffer);

	bool GetImage(size_t imageIndex, VkImage& vkImage);
	inline size_t GetImageCount() { return m_SwapChainImages.size(); }
	inline VkExtent2D GetExtent() { return m_Extend; }
	inline VkFormat GetFormat() { return m_SurfaceFormat.format; }
};

typedef std::shared_ptr<KVulkanSwapChain> KVulkanSwapChainPtr;