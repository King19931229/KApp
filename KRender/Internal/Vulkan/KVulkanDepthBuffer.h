#pragma once
#include "KVulkanConfig.h"

class KVulkanDepthBuffer
{
protected:
	VkFormat m_Format;
	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	bool m_bStencil;
	bool m_bDeviceInit;

	static VkFormat FindDepthFormat(bool bStencil);
public:
	KVulkanDepthBuffer();
	~KVulkanDepthBuffer();

	bool InitDevice(size_t uWidth, size_t uHeight, bool bStencil);
	bool UnInit();

	bool Resize(size_t uWidth, size_t uHeigh);

	inline VkFormat GetVkFormat() { return m_Format; }
	inline VkImageView GetVkImageView() { return m_DepthImageView; }
};