#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKFrameBuffer.h"
#include "KRender/Interface/IKResource.h"
#include "KRender/Interface/IKFrameBuffer.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Publish/KReferenceHolder.h"

struct IKTexture : public IKResource
{
	virtual ~IKTexture() {}

	virtual bool InitMemoryFromFile(const std::string& filePath, bool bGenerateMipmap, bool async) = 0;
	// TODO 直接走KCodecResult
	virtual bool InitMemoryFromData(const void* pRawData, const std::string& name, uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, bool cubeMap, bool bGenerateMipmap, bool async) = 0;
	virtual bool InitMemoryFrom2DArray(const std::string& name, uint32_t width, uint32_t height, uint32_t slices, PixelFormat format, bool bGenerateMipmap, bool async) = 0;
	virtual bool InitDevice(bool async) = 0;
	virtual bool UnInit() = 0;

	virtual IKFrameBufferPtr GetFrameBuffer() = 0;
	virtual bool CopyFromFrameBuffer(IKFrameBufferPtr srcFrameBuffer, uint32_t dstfaceIndex, uint32_t dstmipLevel) = 0;
	virtual bool CopyFromFrameBufferToSlice(IKFrameBufferPtr srcFrameBuffer, uint32_t dstSliceIndex) = 0;

	virtual uint32_t GetWidth() = 0;
	virtual uint32_t GetHeight() = 0;
	virtual uint32_t GetDepth() = 0;
	virtual uint32_t GetSlice() = 0;
	virtual uint32_t GetMipmaps() = 0;

	virtual TextureType GetTextureType() = 0;
	virtual PixelFormat GetTextureFormat() = 0;

	virtual const char* GetPath() = 0;
};

typedef KReferenceHolder<IKTexturePtr> KTextureRef;