#pragma once
#include "Internal/KTextureBase.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanTexture : public KTextureBase
{
protected:
	VkImage m_TextureImage;
	VkImageView m_TextureImageView;
	VkFormat m_TextureFormat;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	bool m_bDeviceInit;
public:
	KVulkanTexture();
	virtual ~KVulkanTexture();

	virtual bool InitDevice();
	virtual bool UnInit();

	inline VkImageView GetImageView() { return m_TextureImageView; }
	inline VkFormat GetImageFormat() { return m_TextureFormat; }
};