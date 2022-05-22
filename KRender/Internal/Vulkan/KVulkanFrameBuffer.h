#pragma once
#include "KRender/Interface/IKFrameBuffer.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"
#include <unordered_map>

class KVulkanFrameBuffer : public IKFrameBuffer
{
protected:
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	KVulkanHeapAllocator::AllocInfo m_MSAAAllocInfo;

	VkImageType m_ImageType;
	VkImageViewType m_ImageViewType;

	VkImage m_Image;
	VkImageView m_ImageView;

	VkImage m_MSAAImage;
	VkImageView m_MSAAImageView;

	VkSampleCountFlagBits m_MSAAFlag;
	VkImageLayout m_ImageLayout;

	VkFormat m_Format;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;
	uint32_t m_Mipmaps;
	uint32_t m_MSAA;
	uint32_t m_Layers;

	bool m_External;

	std::vector<VkImageView> m_MipmapImageViews;
	std::unordered_map<ElementFormat, VkImageView> m_ReinterpretImageView;

	std::vector<VkImageView> CreateMipmapImageViews(VkFormat format);
	bool InitStorageInternal(VkFormat format, TextureType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps);
public:
	KVulkanFrameBuffer();
	~KVulkanFrameBuffer();

	// 创建为持有外部句柄
	enum ExternalType
	{
		ET_SWAPCHAIN,
		ET_TEXTUREIMAGE
	};
	bool InitExternal(ExternalType type, VkImage image, VkImageView imageView, VkImageType imageType, VkImageViewType imageViewType, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t msaa);
	// 创建为ColorAttachment
	bool InitColor(VkFormat format, TextureType textureType, uint32_t width, uint32_t height, uint32_t msaa);
	// 创建为DepthStencilAttachment
	bool InitDepthStencil(uint32_t width, uint32_t height, uint32_t msaa, bool stencil);
	// 创建为StorageImage
	bool InitStorage(VkFormat format, uint32_t width, uint32_t height, uint32_t mipmaps);
	bool InitStorage3D(VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps);

	bool UnInit();

	bool Translate(IKCommandBuffer* cmd, ImageLayout layout) override;

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }
	uint32_t GetDepth() const override { return m_Depth; }
	uint32_t GetMipmaps() const override { return m_Mipmaps; }
	uint32_t GetMSAA() const override { return m_MSAA; }

	bool IsDepthStencil() const override { return m_ImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; }
	bool IsStorageImage() const override { return m_ImageLayout == VK_IMAGE_LAYOUT_GENERAL; }

	inline VkImage GetImage() const { return m_Image; }
	inline VkImageView GetImageView() const { return m_ImageView; }
	inline VkImage GetMSAAImage() const { return m_MSAAImage; }
	inline VkImageView GetMSAAImageView() const { return m_MSAAImageView; }
	inline VkSampleCountFlagBits GetMSAAFlag() const { return m_MSAAFlag; }
	inline VkFormat GetForamt() const { return m_Format; }
	inline VkImageLayout GetImageLayout() const { return m_ImageLayout; }

	VkImageView GetMipmapImageView(uint32_t mipmap);
	VkImageView GetReinterpretImageView(ElementFormat format);
};