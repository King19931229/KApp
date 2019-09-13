#pragma once
#include "Interface/IKRenderConfig.h"

struct IKVertexBuffer
{
	virtual ~IKVertexBuffer() {}
	virtual size_t GetBufferSize() = 0;
	virtual size_t GetVertexCount() = 0;
	// ��ʼ���ڴ����� ͨ�������첽IO
	virtual bool InitMemory(size_t vertexCount, size_t vertexSize, void* pInitData) = 0;
	// ��ʼ���豸���ݲ��ͷ��ڴ����� ���ڴ�������API��ؾ��
	virtual bool InitDevice() = 0;
	// �ͷ��ڴ��������豸��ؾ��
	virtual bool UnInit() = 0;

	virtual bool Write(const void* pData) = 0;
	virtual bool Read(const void* pData) = 0;

	virtual bool CopyFrom(IKVertexBufferPtr pSource) = 0;
	virtual bool CopyTo(IKVertexBufferPtr pDest) = 0;
};

struct IKIndexBuffer
{
	virtual ~IKIndexBuffer() {}
	virtual IndexType GetIndexType() = 0;
	virtual size_t GetBufferSize() = 0;
	virtual size_t GetIndexCount() = 0;
	// ��ʼ���ڴ����� ͨ�������첽IO
	virtual bool InitMemory(IndexType indexType, size_t count, void* pInitData) = 0;
	// ��ʼ���豸���ݲ��ͷ��ڴ����� ���ڴ�������API��ؾ��
	virtual bool InitDevice() = 0;
	// �ͷ��ڴ��������豸��ؾ��
	virtual bool UnInit() = 0;

	virtual bool Write(const void* pData) = 0;
	virtual bool Read(const void* pData) = 0;

	virtual bool CopyFrom(IKIndexBufferPtr pSource) = 0;
	virtual bool CopyTo(IKIndexBufferPtr pDest) = 0;
};

struct IKUniformBuffer
{
	// ��ʼ���ڴ����� ͨ�������첽IO
	virtual bool InitMemory(size_t bufferSize, void* pInitData) = 0;
	// ��ʼ���豸���ݲ��ͷ��ڴ����� ���ڴ�������API��ؾ��
	virtual bool InitDevice() = 0;
	// �ͷ��ڴ��������豸��ؾ��
	virtual bool Uninit() = 0;

	virtual bool Write(const void* pData) = 0;
	virtual bool Read(const void* pData) = 0;

	virtual bool CopyFrom(IKUniformBufferPtr pSource) = 0;
	virtual bool CopyTo(IKUniformBufferPtr pDest) = 0;
};