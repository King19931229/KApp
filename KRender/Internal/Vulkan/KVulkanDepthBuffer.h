#pragma once
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanDepthBuffer
{
protected:
	VkFormat m_Format;
	VkImage m_DepthImage;
	VkImageView m_DepthImageView;
	VkSampleCountFlagBits m_SampleCountFlag;

	unsigned short m_MsaaCount;
	bool m_bStencil;
	bool m_bDeviceInit;

	KVulkanHeapAllocator::AllocInfo m_AllocInfo;

	static VkFormat FindDepthFormat(bool bStencil);
public:
	KVulkanDepthBuffer();
	~KVulkanDepthBuffer();

	bool InitDevice(size_t uWidth, size_t uHeight, unsigned short msaaCount, bool bStencil);
	bool UnInit();

	bool Resize(size_t uWidth, size_t uHeigh);

	inline VkFormat GetVkFormat() { return m_Format; }
	inline VkImageView GetVkImageView() { return m_DepthImageView; }
	inline VkSampleCountFlagBits GetVkSampleCountFlagBits() { return m_SampleCountFlag; }
};