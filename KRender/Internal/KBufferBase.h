#include "Interface/IKBuffer.h"
#include <vector>

class KVertexBufferBase : public IKVertexBuffer
{
protected:
	size_t m_VertexCount;
	size_t m_VertexSize;
	size_t m_BufferSize;
	std::vector<char> m_Data;
public:
	KVertexBufferBase();
	virtual ~KVertexBufferBase();

	virtual size_t GetVertexCount() { return m_VertexCount; }
	virtual size_t GetBufferSize() { return m_BufferSize; }
	virtual size_t GetVertexSize() { return m_VertexSize; }

	virtual bool InitMemory(size_t vertexCount, size_t vertexSize, const void* pInitData);
	virtual bool InitDevice(bool hostVisible) = 0;
	virtual bool UnInit();

	virtual bool Map(void** ppData) = 0;
	virtual bool UnMap() = 0;
	virtual bool Write(const void* pData) = 0;
	virtual bool Read(void* pData) = 0;

	virtual bool CopyFrom(IKVertexBufferPtr pSource) = 0;
	virtual bool CopyTo(IKVertexBufferPtr pDest) = 0;
};

struct KIndexBufferBase : public IKIndexBuffer
{
protected:
	size_t m_uIndexCount;
	IndexType m_IndexType;
	size_t m_BufferSize;
	std::vector<char> m_Data;
public:
	KIndexBufferBase();
	virtual ~KIndexBufferBase();

	virtual IndexType GetIndexType() { return m_IndexType; }
	virtual size_t GetBufferSize() { return m_BufferSize; }
	virtual size_t GetIndexCount() { return m_uIndexCount; }

	virtual bool InitMemory(IndexType indexType, size_t count, const void* pInitData);
	virtual bool InitDevice(bool hostVisible) = 0;
	virtual bool UnInit() = 0;

	virtual bool Map(void** ppData) = 0;
	virtual bool UnMap() = 0;
	virtual bool Write(const void* pData) = 0;
	virtual bool Read(void* pData) = 0;

	virtual bool CopyFrom(IKIndexBufferPtr pSource) = 0;
	virtual bool CopyTo(IKIndexBufferPtr pDest) = 0;
};

class KStorageBufferBase : public IKStorageBuffer
{
protected:
	size_t m_BufferSize;
	std::vector<char> m_Data;
	bool m_bIndirect;
public:
	KStorageBufferBase();
	virtual ~KStorageBufferBase();

	virtual size_t GetBufferSize() { return m_BufferSize; }
	// 初始化内存数据 通常用于异步IO
	virtual bool InitMemory(size_t bufferSize, const void* pInitData);

	virtual bool InitDevice(bool indirect) = 0;
	virtual bool UnInit() = 0;

	virtual bool IsIndirect() { return m_bIndirect; }

	virtual bool Map(void** ppData) = 0;
	virtual bool UnMap() = 0;
	virtual bool Write(const void* pData) = 0;
	virtual bool Read(void* pData) = 0;

	virtual bool CopyFrom(IKStorageBufferPtr pSource) = 0;
	virtual bool CopyTo(IKStorageBufferPtr pDest) = 0;
};

class KUniformBufferBase : public IKUniformBuffer
{
protected:
	size_t m_BufferSize;
	std::vector<char> m_Data;
public:
	KUniformBufferBase();
	virtual ~KUniformBufferBase();

	virtual size_t GetBufferSize() { return m_BufferSize; }
	// 初始化内存数据 通常用于异步IO
	virtual bool InitMemory(size_t bufferSize, const void* pInitData);

	virtual bool InitDevice() = 0;
	virtual bool UnInit() = 0;

	virtual bool Map(void** ppData) = 0;
	virtual bool UnMap() = 0;
	virtual bool Write(const void* pData) = 0;
	virtual bool Read(void* pData) = 0;

	virtual bool Reference(void **ppData) = 0;

	virtual bool CopyFrom(IKUniformBufferPtr pSource) = 0;
	virtual bool CopyTo(IKUniformBufferPtr pDest) = 0;
};