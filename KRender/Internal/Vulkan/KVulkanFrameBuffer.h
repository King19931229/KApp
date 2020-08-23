#pragma once
#include "KRender/Interface/IKFrameBuffer.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanFrameBuffer : public IKFrameBuffer
{
protected:
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	KVulkanHeapAllocator::AllocInfo m_MSAAAllocInfo;

	VkImage m_Image;
	VkImageView m_ImageView;

	VkImage m_MSAAImage;
	VkImageView m_MSAAImageView;

	VkSampleCountFlagBits m_MSAAFlag;

	VkFormat m_Format;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;
	uint32_t m_Mipmaps;
	uint32_t m_MSAA;

	bool m_External;
	bool m_DepthStencil;
public:
	KVulkanFrameBuffer();
	~KVulkanFrameBuffer();

	// 创建为持有外部句柄
	bool InitExternal(VkImage image, VkImageView imageView, VkFormat format,
		uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t msaa);
	// 创建为ColorAttachment
	bool InitColor(VkFormat format, TextureType textureType, uint32_t width, uint32_t height, uint32_t msaa);
	// 创建为DepthStencilAttachment
	bool InitDepthStencil(uint32_t width, uint32_t height, uint32_t msaa, bool stencil);

	bool UnInit();

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }
	uint32_t GetDepth() const override { return m_Depth; }
	uint32_t GetMipmaps() const override { return m_Mipmaps; }
	uint32_t GetMSAA() const override { return m_MSAA; }
	bool IsDepthStencil() const { return m_DepthStencil; }

	inline VkImage GetImage() const { return m_Image; }
	inline VkImageView GetImageView() const { return m_ImageView; }
	inline VkImage GetMSAAImage() const { return m_MSAAImage; }
	inline VkImageView GetMSAAImageView() const { return m_MSAAImageView; }
	inline VkSampleCountFlagBits GetMSAAFlag() const { return m_MSAAFlag; }

	inline VkFormat GetForamt() const { return m_Format; }
};