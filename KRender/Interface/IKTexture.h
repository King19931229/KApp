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
	virtual bool InitMemoryFromData(const void* pRawData, const std::string& name, size_t width, size_t height, size_t depth, ImageFormat format, bool cubeMap, bool bGenerateMipmap, bool async) = 0;
	virtual bool InitMemoryFrom2DArray(const std::string& name, size_t width, size_t height, size_t slices, ImageFormat format, bool bGenerateMipmap, bool async) = 0;
	virtual bool InitDevice(bool async) = 0;
	virtual bool UnInit() = 0;

	virtual IKFrameBufferPtr GetFrameBuffer() = 0;
	virtual bool CopyFromFrameBuffer(IKFrameBufferPtr srcFrameBuffer, uint32_t dstfaceIndex, uint32_t dstmipLevel) = 0;
	virtual bool CopyFromFrameBufferToSlice(IKFrameBufferPtr srcFrameBuffer, uint32_t dstSliceIndex) = 0;

	virtual size_t GetWidth() = 0;
	virtual size_t GetHeight() = 0;
	virtual size_t GetDepth() = 0;
	virtual size_t GetSlice() = 0;
	virtual unsigned short GetMipmaps() = 0;

	virtual TextureType GetTextureType() = 0;
	virtual ElementFormat GetTextureFormat() = 0;

	virtual const char* GetPath() = 0;
};

typedef KReferenceHolder<IKTexturePtr> KTextureRef;