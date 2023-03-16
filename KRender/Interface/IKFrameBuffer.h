#pragma once
#include "KRender/Interface/IKRenderDevice.h"

struct IKFrameBuffer
{
	virtual ~IKFrameBuffer() {}
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual uint32_t GetDepth() const = 0;
	virtual uint32_t GetMipmaps() const = 0;
	virtual uint32_t GetMSAA() const = 0;
	virtual bool IsDepthStencil() const = 0;
	virtual bool IsStorageImage() const = 0;
	virtual bool IsReadback() const = 0;
	virtual bool CopyToReadback(IKFrameBuffer* framebuffer) = 0;
	virtual bool Readback(void* pDest, size_t offset, size_t size) = 0;
	virtual bool Translate(IKCommandBuffer* cmd, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout) = 0;
	virtual bool Translate(IKCommandBuffer* cmd, PipelineStages srcStages, PipelineStages dstStages, ImageLayout layout) = 0;
	virtual bool TranslateMipmap(IKCommandBuffer* cmd, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout) = 0;
	virtual bool TranslateMipmap(IKCommandBuffer* cmd, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout layout) = 0;
};