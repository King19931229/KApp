#pragma once
#include "Interface/IKRenderConfig.h"

struct IKVertexBuffer
{
	virtual ~IKVertexBuffer() {}
	virtual size_t GetBufferSize() = 0;
	virtual size_t GetVertexCount() = 0;
	// 初始化内存数据 通常用于异步IO
	virtual bool InitMemory(size_t vertexCount, size_t vertexSize, const void* pInitData) = 0;
	// 初始化设备数据并释放内存数据 用于创建绘制API相关句柄
	virtual bool InitDevice() = 0;
	// 释放内存数据与设备相关句柄
	virtual bool UnInit() = 0;

	virtual bool Write(const void* pData) = 0;
	virtual bool Read(void* pData) = 0;

	virtual bool CopyFrom(IKVertexBufferPtr pSource) = 0;
	virtual bool CopyTo(IKVertexBufferPtr pDest) = 0;
};

struct IKIndexBuffer
{
	virtual ~IKIndexBuffer() {}
	virtual IndexType GetIndexType() = 0;
	virtual size_t GetBufferSize() = 0;
	virtual size_t GetIndexCount() = 0;
	// 初始化内存数据 通常用于异步IO
	virtual bool InitMemory(IndexType indexType, size_t count, const void* pInitData) = 0;
	// 初始化设备数据并释放内存数据 用于创建绘制API相关句柄
	virtual bool InitDevice() = 0;
	// 释放内存数据与设备相关句柄
	virtual bool UnInit() = 0;

	virtual bool Write(const void* pData) = 0;
	virtual bool Read(void* pData) = 0;

	virtual bool CopyFrom(IKIndexBufferPtr pSource) = 0;
	virtual bool CopyTo(IKIndexBufferPtr pDest) = 0;
};

struct IKUniformBuffer
{
	virtual ~IKUniformBuffer() {}
	virtual size_t GetBufferSize() = 0;
	// 初始化内存数据 通常用于异步IO
	virtual bool InitMemory(size_t bufferSize, const void* pInitData) = 0;
	// 初始化设备数据并释放内存数据 用于创建绘制API相关句柄
	virtual bool InitDevice() = 0;
	// 释放内存数据与设备相关句柄
	virtual bool UnInit() = 0;

	virtual bool Write(const void* pData) = 0;
	virtual bool Read(void* pData) = 0;

	virtual bool CopyFrom(IKUniformBufferPtr pSource) = 0;
	virtual bool CopyTo(IKUniformBufferPtr pDest) = 0;
};