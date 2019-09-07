#pragma once
#include "Interface/IKRenderConfig.h"

struct IKVertexBuffer
{
	virtual ~IKVertexBuffer() {}
	virtual VertexElement GetVertexElement() = 0;
	virtual size_t GetBufferSize() = 0;
	virtual size_t GetVertexCount() = 0;

	virtual bool Init(VertexElement element, size_t count) = 0;
	virtual bool UnInit() = 0;

	virtual bool Write(const void* pData) = 0;
};

struct IKIndexBuffer
{
	virtual ~IKIndexBuffer() {}
	virtual IndexType GetIndexType() = 0;
	virtual size_t GetBufferSize() = 0;
	virtual size_t GetIndexCount() = 0;

	virtual bool Init(IndexType indexType, size_t count) = 0;
	virtual bool UnInit() = 0;

	virtual bool Write(const void* pData) = 0;
};

struct IKUniformBuffer
{
	virtual bool Init(size_t bufferSize) = 0;
	virtual bool Uninit() = 0;

	virtual bool Write(const void* pData) = 0;
};