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
	IKFrameBufferPtr m_FrameBuffer;
	bool m_bDeviceInit;

	std::mutex m_LoadTaskLock;
	KTaskUnitProcessorPtr m_LoadTask;

	bool CancelDeviceTask();
	bool WaitDeviceTask();
	bool ReleaseDevice();
public:
	KVulkanTexture();
	virtual ~KVulkanTexture();

	virtual ResourceState GetResourceState();
	virtual void WaitForMemory();
	virtual void WaitForDevice();

	virtual bool InitDevice(bool async);
	virtual bool UnInit();

	virtual IKFrameBufferPtr GetFrameBuffer();
	virtual bool CopyFromFrameBuffer(IKFrameBufferPtr srcFrameBuffer, uint32_t dstfaceIndex, uint32_t dstmipLevel);

	inline VkImage GetImage() { return m_TextureImage; }
	inline VkImageView GetImageView() { return m_TextureImageView; }
	inline VkFormat GetImageFormat() { return m_TextureFormat; }
};