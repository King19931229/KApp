#pragma once
#include "KRender/Interface/IKRenderDevice.h"

enum FrameBufferType
{
	FT_EXTERNAL_SOURCE,
	FT_COLOR_ATTACHMENT,
	FT_DEPTH_ATTACHMENT,
	FT_STORAGE_IMAGE,
	FT_READBACK_USAGE
};

struct IKFrameBuffer
{
	virtual ~IKFrameBuffer() {}
	virtual bool SetDebugName(const char* name) = 0;
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual uint32_t GetDepth() const = 0;
	virtual uint32_t GetMipmaps() const = 0;
	virtual uint32_t GetMSAA() const = 0;
	virtual FrameBufferType GetType() const = 0;
	virtual bool IsDepthStencil() const = 0;
	virtual bool IsStorageImage() const = 0;
	virtual bool IsReadback() const = 0;
	virtual bool SupportBlit() const = 0;
	virtual bool CopyToReadback(IKFrameBuffer* framebuffer) = 0;
	virtual bool Readback(void* pDest, size_t size) = 0;
	virtual bool Transition(IKCommandBuffer* cmd, IKQueue* srcQueue, IKQueue* dstQueue, uint32_t baseMip, uint32_t numMip, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout) = 0;
};