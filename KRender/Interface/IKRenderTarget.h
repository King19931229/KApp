#pragma once
#include "IKRenderConfig.h"

struct IKRenderTarget
{
	virtual ~IKRenderTarget() {}

	virtual bool InitFromDepthStencil(uint32_t width, uint32_t height, uint32_t msaaCount, bool bStencil) = 0;
	// 延迟初始化仅分配图像；调用方必须在命令流中转换 UNDEFINED 布局并在读取前写全有效内容。
	// 默认保持原来的同步布局转换/Storage 清零，DOF 使用延迟路径避免交互切档等待 GPU。
	virtual bool InitFromColor(uint32_t width, uint32_t height, uint32_t msaaCount, uint32_t mipmaps, PixelFormat format, bool deferredInitialization = false) = 0;
	virtual bool InitFromStorage(uint32_t width, uint32_t height, uint32_t mipmaps, PixelFormat format, bool deferredInitialization = false) = 0;
	virtual bool InitFromStorage3D(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, PixelFormat format) = 0;
	virtual bool InitFromReadback(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, PixelFormat format) = 0;

	virtual bool UnInit() = 0;
	virtual IKFrameBufferPtr GetFrameBuffer() = 0;
	virtual bool GetSize(uint32_t& width, uint32_t& height) = 0;
};
