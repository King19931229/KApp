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

	VkImageTiling m_Tiling;

	static std::atomic_uint32_t ms_UniqueIDCounter;
	uint32_t m_UniqueID;

	FrameBufferType m_FrameBufferType;

	std::unordered_map<uint32_t, VkImageView> m_MipmapImageViews;
	std::unordered_map<uint64_t, VkImageView> m_ReinterpretImageView;

	bool InitStorageInternal(VkFormat format, TextureType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps);
	bool TransitionLayoutImpl(VkCommandBuffer cmdBuffer, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, uint32_t baseMip, uint32_t numMip, VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages, VkImageLayout oldLayout, VkImageLayout newLayout);
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
	bool InitColor(VkFormat format, TextureType textureType, uint32_t width, uint32_t height, uint32_t mipmaps, uint32_t msaa);
	// 创建为DepthStencilAttachment
	bool InitDepthStencil(uint32_t width, uint32_t height, uint32_t msaa, bool stencil);
	// 创建为StorageImage
	bool InitStorage2D(VkFormat format, uint32_t width, uint32_t height, uint32_t mipmaps);
	bool InitStorage3D(VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps);
	// 创建为回读
	bool InitReadback(VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps);

	bool UnInit();

	bool SetDebugName(const char* name) override;

	bool CopyToReadback(IKFrameBuffer* framebuffer) override;
	bool Readback(void* pDest, size_t size) override;

	bool Transition(IKCommandBuffer* cmd, IKQueue* srcQueue, IKQueue* dstQueue, uint32_t baseMip, uint32_t numMip, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout) override;

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }
	uint32_t GetDepth() const override { return m_Depth; }
	uint32_t GetMipmaps() const override { return m_Mipmaps; }
	uint32_t GetMSAA() const override { return m_MSAA; }

	FrameBufferType GetType() const override { return m_FrameBufferType; }
	bool IsDepthStencil() const override { return m_FrameBufferType == FT_DEPTH_ATTACHMENT; }
	bool IsStorageImage() const override { return m_FrameBufferType == FT_STORAGE_IMAGE; }
	bool IsReadback() const override { return m_FrameBufferType == FT_READBACK_USAGE; }

	bool SupportBlit() const override;

	inline VkImage GetImage() const { return m_Image; }
	VkImageView GetImageView() const;
	inline VkImage GetMSAAImage() const { return m_MSAAImage; }
	inline VkImageView GetMSAAImageView() const { return m_MSAAImageView; }
	inline VkSampleCountFlagBits GetMSAAFlag() const { return m_MSAAFlag; }
	inline VkFormat GetForamt() const { return m_Format; }
	inline VkImageLayout GetImageLayout() const { return m_ImageLayout; }
	inline uint32_t GetUniqueID() const { return m_UniqueID; }

	VkImageView GetReinterpretImageView(ElementFormat format);
	VkImageView GetMipmapImageView(uint32_t startMip, uint32_t numMip);
	VkImageView GetMipmapReinterpretImageView(ElementFormat format, uint32_t startMip, uint32_t numMip);
};