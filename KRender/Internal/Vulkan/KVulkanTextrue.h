#pragma once
#include "Internal/KTextureBase.h"
#include "KVulkanConfig.h"

class KVulkanTexture : public KTextureBase
{
protected:
	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	bool m_bDeviceInit;
public:
	KVulkanTexture();
	virtual ~KVulkanTexture();

	virtual bool InitDevice();
	virtual bool UnInit();
};
