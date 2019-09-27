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

	VkFormat FindDepthFormat(bool bStencil);
public:
	KVulkanDepthBuffer();
	~KVulkanDepthBuffer();

	bool Init(size_t uWidth, size_t uHeight, bool bStencil);
	bool UnInit();

	bool Resize(size_t uWidth, size_t uHeigh);
};