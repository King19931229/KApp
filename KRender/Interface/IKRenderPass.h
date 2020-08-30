#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include <functional>

struct KClearColor
{
	float r;
	float g;
	float b;
	float a;

	KClearColor()
	{
		r = g = b = a = 0.0f;
	}

	KClearColor(float _r, float _g, float _b, float _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
};

struct KClearDepthStencil
{
	float depth;
	unsigned int stencil;

	KClearDepthStencil()
	{
		depth = 1.0f;
		stencil = 0;
	}

	KClearDepthStencil(float _depth, unsigned int _stencil)
	{
		depth = _depth;
		stencil = _stencil;
	}
};

struct KClearValue
{
	KClearColor color;
	KClearDepthStencil depthStencil;
};

struct KViewPortArea
{
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;

	KViewPortArea()
	{
		x = y = width = height = 0;
	}
};

struct KRenderPassOperation
{
	LoadOperation loadOp;
	StoreOperation storeOp;
	KRenderPassOperation()
	{
		loadOp = LO_CLEAR;
		storeOp = SO_STORE;
	}
	KRenderPassOperation(LoadOperation _loadOp, StoreOperation _storeOp)
	{
		loadOp = _loadOp;
		storeOp = _storeOp;
	}
};

typedef std::function<void(IKRenderPass*)> RenderPassInvalidCallback;

struct IKRenderPass
{
	virtual ~IKRenderPass() {}

	virtual bool SetColorAttachment(uint32_t attachment, IKFrameBufferPtr color) = 0;
	virtual bool SetDepthStencilAttachment(IKFrameBufferPtr depthStencil) = 0;

	virtual bool SetClearColor(uint32_t attachment, const KClearColor& clearColor) = 0;
	virtual bool SetClearDepthStencil(const KClearDepthStencil& clearDepthStencil) = 0;

	virtual bool SetOpColor(uint32_t attachment, LoadOperation loadOp, StoreOperation storeOp) = 0;
	virtual bool SetOpDepthStencil(LoadOperation depthLoadOp, StoreOperation depthStoreOp, LoadOperation stencilLoadOp, StoreOperation stencilStoreOp) = 0;

	virtual bool SetAsSwapChainPass(bool swapChain) = 0;

	virtual bool HasColorAttachment() = 0;
	virtual bool HasDepthStencilAttachment() = 0;
	virtual uint32_t GetColorAttachmentCount() = 0;

	virtual const KViewPortArea& GetViewPort() = 0;

	virtual bool RegisterInvalidCallback(RenderPassInvalidCallback* callback) = 0;
	virtual bool UnRegisterInvalidCallback(RenderPassInvalidCallback* callback) = 0;

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
};