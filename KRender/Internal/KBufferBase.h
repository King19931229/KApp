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

	virtual bool InitMemory(size_t vertexCount, size_t vertexSize, void* pInitData);
	virtual bool InitDevice() = 0;
	virtual bool UnInit();

	virtual bool Write(const void* pData) = 0;
	virtual bool Read(const void* pData) = 0;

	virtual bool CopyFrom(IKVertexBufferPtr pSource) = 0;
	virtual bool CopyTo(IKVertexBufferPtr pDest) = 0;
};