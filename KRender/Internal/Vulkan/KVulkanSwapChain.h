#pragma once
#include "Interface/IKSwapChain.h"
#include "KVulkanConfig.h"
#include <vector>
#include <memory>

class KVulkanSwapChain : public IKSwapChain
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
	std::vector<VkImageView> m_SwapChainImageViews;
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

	bool Init(uint32_t width, uint32_t height, size_t frameInFlight);
	bool UnInit();

	virtual uint32_t GetWidth() { return m_Extend.width; }
	virtual uint32_t GetHeight() { return m_Extend.height; }

	VkResult WaitForInfightFrame(size_t& frameIndex);
	VkResult AcquireNextImage(uint32_t& imageIndex);
	VkResult PresentQueue(VkQueue graphicsQueue, VkQueue presentQueue, uint32_t imageIndex, VkCommandBuffer commandBuffer);

	inline VkImageView GetImageView(size_t imageIndex) { return (imageIndex >= m_SwapChainImageViews.size()) ? VK_NULL_HANDLE : m_SwapChainImageViews[imageIndex]; }
	inline size_t GetImageCount() { return m_SwapChainImages.size(); }
	inline VkExtent2D GetExtent() { return m_Extend; }
	inline VkFormat GetImageFormat() { return m_SurfaceFormat.format; }
};

typedef std::shared_ptr<KVulkanSwapChain> KVulkanSwapChainPtr;