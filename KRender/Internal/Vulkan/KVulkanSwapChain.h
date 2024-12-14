#pragma once
#include "Interface/IKSwapChain.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanConfig.h"
#include <vector>
#include <unordered_set>
#include <memory>

class KVulkanSwapChain : public IKSwapChain
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct FrameBuffer
	{
		IKFrameBufferPtr colorFrameBuffer;
		IKFrameBufferPtr depthStencilFrameBuffer;
	};
protected:
	IKRenderWindow* m_pWindow;

	VkSurfaceKHR m_Surface;
	VkSwapchainKHR  m_SwapChain;
	VkExtent2D m_Extend;
	VkQueue m_PresentQueue;

	VkPresentModeKHR m_PresentMode;
	VkSurfaceFormatKHR m_SurfaceFormat;

	uint32_t m_MaxFramesInFight;
	uint32_t m_CurrentFlightIndex;
	uint32_t m_CurrentImageIndex;

	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;
	// 这个信号量是给获取交换链Image后促发用的
	IKSemaphorePtr m_ImageAvailableSemaphore;
	// 这个信号量是给完成交换链Image后促发用的
	IKSemaphorePtr m_RenderFinishedSemaphore;
	IKFencePtr m_InFlightFence;
	SwapChainSupportDetails m_SwapChainSupportDetails;
	std::vector<FrameBuffer> m_FrameBuffers;
	std::vector<IKRenderPassPtr> m_RenderPasses;

	typedef std::unordered_set<KDevicePresentCallback*> PresentCallbackSet;
	PresentCallbackSet m_PrePresentCallback;
	PresentCallbackSet m_PostPresentCallback;

	typedef std::unordered_set<KSwapChainRecreateCallback*> SwapChainCallbackSet;
	SwapChainCallbackSet m_SwapChainCallback;

	bool QuerySwapChainSupport();
	bool ChooseSwapSurfaceFormat();
	bool ChooseSwapPresentMode();
	bool ChooseSwapExtent(uint32_t windowWidth, uint32_t windowHeight);

	bool ChooseQueue();
	bool CreateSurface();
	bool CreateSwapChain();
	bool CreateSyncObjects();
	bool CreateFrameBuffers();

	bool CleanupSwapChain();
	bool DestroySyncObjects();
	bool DestroyFrameBuffers();
public:
	KVulkanSwapChain();
	~KVulkanSwapChain();

	virtual bool Init(IKRenderWindow* window);
	virtual bool UnInit();
	virtual IKRenderWindow* GetWindow();
	virtual uint32_t GetFrameInFlight();

	virtual uint32_t GetWidth() { return m_Extend.width; }
	virtual uint32_t GetHeight() { return m_Extend.height; }

	virtual IKSemaphorePtr GetImageAvailableSemaphore();
	virtual IKSemaphorePtr GetRenderFinishSemaphore();
	virtual IKFencePtr GetInFlightFence();

	virtual IKRenderPassPtr GetRenderPass(uint32_t chainIndex);
	virtual IKFrameBufferPtr GetColorFrameBuffer(uint32_t chainIndex);
	virtual IKFrameBufferPtr GetDepthStencilFrameBuffer(uint32_t chainIndex);

	virtual bool RegisterPrePresentCallback(KDevicePresentCallback* callback);
	virtual bool UnRegisterPrePresentCallback(KDevicePresentCallback* callback);
	virtual bool RegisterPostPresentCallback(KDevicePresentCallback* callback);
	virtual bool UnRegisterPostPresentCallback(KDevicePresentCallback* callback);

	virtual bool RegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback);
	virtual bool UnRegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback);

	virtual void WaitForInFlightFrame(uint32_t& frameIndex);
	virtual void AcquireNextImage(uint32_t& imageIndex);
	virtual void PresentQueue(bool& needResize);

	inline VkImage GetImage(size_t imageIndex) { return (imageIndex >= m_SwapChainImages.size()) ? VK_NULL_HANDLE : m_SwapChainImages[imageIndex]; }
	inline VkImageView GetImageView(size_t imageIndex) { return (imageIndex >= m_SwapChainImageViews.size()) ? VK_NULL_HANDLE : m_SwapChainImageViews[imageIndex]; }
	inline size_t GetImageCount() { return m_SwapChainImages.size(); }
	inline VkExtent2D GetExtent() { return m_Extend; }
	inline VkFormat GetImageFormat() { return m_SurfaceFormat.format; }
};