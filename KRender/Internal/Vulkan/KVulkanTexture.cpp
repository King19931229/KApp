#include "KVulkanTexture.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanFrameBuffer.h"
#include "Internal/KRenderGlobal.h"

KVulkanTexture::KVulkanTexture()
	: KTextureBase(),
	m_bDeviceInit(false)
{
	ZERO_MEMORY(m_AllocInfo);
	m_TextureImage = VK_NULL_HANDLE;
	m_TextureImageView = VK_NULL_HANDLE;
	m_TextureFormat = VK_FORMAT_UNDEFINED;
	m_FrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
}

KVulkanTexture::~KVulkanTexture()
{
	ASSERT_RESULT(!m_DeviceLoadTask.Get() || m_DeviceLoadTask->IsCompleted());
	ASSERT_RESULT(!m_bDeviceInit);
}

ResourceState KVulkanTexture::GetResourceState()
{
	return m_ResourceState;
}

void KVulkanTexture::WaitForDevice()
{
	WaitDeviceTask();
}

bool KVulkanTexture::CancelDeviceTask()
{
	if (m_DeviceLoadTask)
	{
		m_DeviceLoadTask->Abandon();
		m_DeviceLoadTask.Release();
	}
	return true;
}

bool KVulkanTexture::WaitDeviceTask()
{
	if (m_DeviceLoadTask)
	{
		m_DeviceLoadTask->WaitForCompletion();
		m_DeviceLoadTask.Release();
	}
	return true;
}

bool KVulkanTexture::ReleaseDevice()
{
	CancelDeviceTask();
	if (m_bDeviceInit)
	{
		using namespace KVulkanGlobal;
		vkDestroyImageView(device, m_TextureImageView, nullptr);
		m_TextureImageView = VK_NULL_HANDLE;
		KVulkanInitializer::FreeVkImage(m_TextureImage, m_AllocInfo);
		m_TextureImage = VK_NULL_HANDLE;	
		((KVulkanFrameBuffer*)m_FrameBuffer.get())->UnInit();
		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanTexture::InitDevice(bool async)
{
	ReleaseDevice();

	auto loadImpl = [=]()->bool
	{
		if (!m_ImageData.pData)
		{
			return false;
		}

		KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThread);

		ResourceState previousState = m_ResourceState;
		m_ResourceState = RS_DEVICE_LOADING;

		using namespace KVulkanGlobal;
		ASSERT_RESULT(!m_bDeviceInit);

		uint32_t layerCounts = 1;
		VkImageCreateFlags createFlags = 0;

		if (m_TextureType == TT_TEXTURE_CUBE_MAP)
		{
			layerCounts = 6;
			createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}
		else if (m_TextureType == TT_TEXTURE_2D_ARRAY)
		{
			layerCounts = (uint32_t)m_Slice;
		}

		VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;
		VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		m_TextureFormat = VK_FORMAT_UNDEFINED;

		ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(m_TextureType, imageType, imageViewType));
		ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(m_Format, m_TextureFormat));

		if (m_ImageData.pData && m_ImageData.pData->GetSize() > 0)
		{
			size_t imageSize = m_ImageData.pData->GetSize();

			VkBuffer stagingBuffer;
			KVulkanHeapAllocator::AllocInfo stagingAllocInfo;

			KVulkanInitializer::CreateVkBuffer((VkDeviceSize)imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingAllocInfo);
			{
				void* pixels = m_ImageData.pData->GetData();
				assert(pixels);

				void* data = nullptr;

				if (stagingAllocInfo.pMapped)
				{
					memcpy(stagingAllocInfo.pMapped, pixels, static_cast<size_t>(imageSize));
				}
				else
				{
					VK_ASSERT_RESULT(vkMapMemory(device, stagingAllocInfo.vkMemroy, stagingAllocInfo.vkOffset, imageSize, 0, &data));
					memcpy(data, pixels, static_cast<size_t>(imageSize));
					vkUnmapMemory(device, stagingAllocInfo.vkMemroy);
				}

				KVulkanInitializer::CreateVkImage((uint32_t)m_Width,
					(uint32_t)m_Height,
					(uint32_t)m_Depth,
					(uint32_t)layerCounts,
					(uint32_t)m_Mipmaps,
					VK_SAMPLE_COUNT_1_BIT,
					imageType,
					m_TextureFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					createFlags,
					m_TextureImage, m_AllocInfo);
				{
					// 先转换image layout为之后buffer拷贝数据到image作准备
					KVulkanInitializer::TransitionImageLayout(m_TextureImage,
						m_TextureFormat,
						0, (uint32_t)layerCounts,
						0, (uint32_t)m_Mipmaps,
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

					const KSubImageInfoList& subImageInfo = m_ImageData.pData->GetSubImageInfo();
					KVulkanInitializer::BufferSubRegionCopyInfoList copyInfo;
					copyInfo.reserve(subImageInfo.size());

					for (const KSubImageInfo& info : subImageInfo)
					{
						KVulkanInitializer::BufferSubRegionCopyInfo copy;

						copy.offset = static_cast<uint32_t>(info.uOffset);
						copy.width = static_cast<uint32_t>(info.uWidth);
						copy.height = static_cast<uint32_t>(info.uHeight);
						copy.mipLevel = static_cast<uint32_t>(info.uMipmapIndex);
						if (m_TextureType == TT_TEXTURE_2D_ARRAY)
						{
							copy.layer = static_cast<uint32_t>(info.uSliceIndex);
						}
						else if (m_TextureType == TT_TEXTURE_CUBE_MAP)
						{
							copy.layer = static_cast<uint32_t>(info.uFaceIndex);
						}
						else
						{
							copy.layer = 0;
						}
						copyInfo.push_back(copy);
					}

					// 拷贝buffer数据到image
					KVulkanInitializer::CopyVkBufferToVkImageByRegion(stagingBuffer, m_TextureImage, layerCounts, copyInfo);

					if (m_bGenerateMipmap)
					{
						KVulkanInitializer::GenerateMipmaps(m_TextureImage, m_TextureFormat, static_cast<int32_t>(m_Width), static_cast<int32_t>(m_Height), static_cast<int32_t>(m_Depth),
							0, layerCounts, static_cast<int32_t>(m_Mipmaps));
					}
					else
					{
						// 再转换image layout为之后shader使用image数据作准备
						KVulkanInitializer::TransitionImageLayout(m_TextureImage,
							m_TextureFormat,
							0, (uint32_t)layerCounts,
							0, (uint32_t)m_Mipmaps,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					}

					// 创建imageview
					KVulkanInitializer::CreateVkImageView(m_TextureImage, imageViewType, m_TextureFormat, VK_IMAGE_ASPECT_COLOR_BIT, 0, (uint32_t)m_Mipmaps, 0, layerCounts, m_TextureImageView);
					KVulkanInitializer::FreeVkBuffer(stagingBuffer, stagingAllocInfo);
				}

				m_bDeviceInit = true;
				m_ImageData.pData = nullptr;

				((KVulkanFrameBuffer*)m_FrameBuffer.get())->InitExternal(KVulkanFrameBuffer::ET_TEXTUREIMAGE, m_TextureImage, m_TextureImageView,
					imageType, imageViewType,
					m_TextureFormat,
					(uint32_t)m_Width, (uint32_t)m_Height, (uint32_t)m_Depth, m_Mipmaps, 1);

				m_FrameBuffer->SetDebugName(GetPath());

				m_ResourceState = RS_DEVICE_LOADED;

				return true;
			}
		}

		m_ResourceState = previousState;
		return false;
	};

	if (async)
	{
		m_DeviceLoadTask = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork(loadImpl)), NamedThread::RENDER_THREAD, { m_MemoryLoadTask });
		return true;
	}
	else
	{
		WaitMemoryTask();
		return loadImpl();
	}
}

bool KVulkanTexture::UnInit()
{
	KTextureBase::UnInit();
	ReleaseDevice();
	return true;
}

IKFrameBufferPtr KVulkanTexture::GetFrameBuffer()
{
	return m_FrameBuffer;
}

bool KVulkanTexture::CopyFromFrameBuffer(IKFrameBufferPtr src, uint32_t faceIndex, uint32_t mipLevel)
{
	if (src && mipLevel < m_Mipmaps)
	{
		KVulkanFrameBuffer* srcFrameBufer = (KVulkanFrameBuffer*)src.get();
		VkImage srcImage = srcFrameBufer->GetImage();

		KVulkanFrameBuffer* dstFrameBufer = (KVulkanFrameBuffer*)this;

		KVulkanInitializer::TransitionImageLayout(srcImage,
			m_TextureFormat,
			0, 1,
			0, 1,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		KVulkanInitializer::TransitionImageLayout(m_TextureImage,
			m_TextureFormat,
			faceIndex, 1,
			mipLevel, 1,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		bool supportsBlit = srcFrameBufer->SupportBlit() && m_FrameBuffer->SupportBlit();

		uint32_t width = std::max(1U, (uint32_t)(m_Width) >> mipLevel);
		uint32_t height = std::max(1U, (uint32_t)(m_Height) >> mipLevel);

		if (supportsBlit)
		{
			KVulkanInitializer::ImageBlitInfo blitInfo = {};

			blitInfo.srcWidth = src->GetWidth();
			blitInfo.srcHeight = src->GetHeight();
			blitInfo.dstWidth = width;
			blitInfo.dstHeight = height;
			blitInfo.srcMipLevel = 0;
			blitInfo.srcArrayIndex = 0;
			blitInfo.dstMipLevel = mipLevel;
			blitInfo.dstArrayIndex = faceIndex;
			blitInfo.linear = true;

			KVulkanInitializer::BlitVkImageToVkImage(srcImage, m_TextureImage, blitInfo);
		}
		else
		{
			assert(width == srcFrameBufer->GetWidth() && height == srcFrameBufer->GetHeight() && "must match");
			KVulkanInitializer::ImageSubRegionCopyInfo copyInfo = {};

			copyInfo.width = width;
			copyInfo.height = height;
			copyInfo.srcMipLevel = 0;
			copyInfo.srcArrayIndex = 0;
			copyInfo.dstMipLevel = mipLevel;
			copyInfo.dstArrayIndex = faceIndex;

			KVulkanInitializer::CopyVkImageToVkImage(srcImage, m_TextureImage, copyInfo);
		}

		KVulkanInitializer::TransitionImageLayout(srcImage,
			m_TextureFormat,
			0, 1,
			0, 1,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		KVulkanInitializer::TransitionImageLayout(m_TextureImage,
			m_TextureFormat,
			faceIndex, 1,
			mipLevel, 1,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		return true;
	}
	return false;
}

bool KVulkanTexture::CopyFromFrameBufferToSlice(IKFrameBufferPtr src, uint32_t sliceIndex)
{
	if (src && sliceIndex < m_Slice)
	{
		KVulkanFrameBuffer* srcFrameBufer = (KVulkanFrameBuffer*)src.get();
		VkImage srcImage = srcFrameBufer->GetImage();

		KVulkanInitializer::TransitionImageLayout(srcImage,
			m_TextureFormat,
			0, 1,
			0, 1,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		KVulkanInitializer::TransitionImageLayout(m_TextureImage,
			m_TextureFormat,
			sliceIndex, 1,
			0, (uint32_t)m_Mipmaps,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		bool supportsBlit = srcFrameBufer->SupportBlit() && m_FrameBuffer->SupportBlit();

		if (supportsBlit)
		{
			KVulkanInitializer::ImageBlitInfo blitInfo = {};

			blitInfo.srcWidth = src->GetWidth();
			blitInfo.srcHeight = src->GetHeight();
			blitInfo.dstWidth = (uint32_t)m_Width;
			blitInfo.dstHeight = (uint32_t)m_Height;
			blitInfo.srcMipLevel = 0;
			blitInfo.srcArrayIndex = 0;
			blitInfo.dstMipLevel = 0;
			blitInfo.dstArrayIndex = sliceIndex;
			blitInfo.linear = true;

			KVulkanInitializer::BlitVkImageToVkImage(srcImage, m_TextureImage, blitInfo);
		}
		else
		{
			assert(m_Width == srcFrameBufer->GetWidth() && m_Height == srcFrameBufer->GetHeight() && "must match");
			KVulkanInitializer::ImageSubRegionCopyInfo copyInfo = {};

			copyInfo.width = (uint32_t)m_Width;
			copyInfo.height = (uint32_t)m_Height;
			copyInfo.srcMipLevel = 0;
			copyInfo.srcArrayIndex = 0;
			copyInfo.dstMipLevel = 0;
			copyInfo.dstArrayIndex = sliceIndex;

			KVulkanInitializer::CopyVkImageToVkImage(srcImage, m_TextureImage, copyInfo);
		}

		KVulkanInitializer::TransitionImageLayout(srcImage,
			m_TextureFormat,
			0, 1,
			0, 1,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		
		// 强制硬件生成Mip，这里没有按Mip传入Slice的接口，每个Slice都必须有更新好的Mip
		KVulkanInitializer::GenerateMipmaps(m_TextureImage, m_TextureFormat,
			static_cast<int32_t>(m_Width), static_cast<int32_t>(m_Height), static_cast<int32_t>(m_Depth),
			sliceIndex, 1, static_cast<int32_t>(m_Mipmaps));

		return true;
	}
	return false;
}