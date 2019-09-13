#include "KBufferBase.h"
#include "Interface/IKBuffer.h"

KVertexBufferBase::KVertexBufferBase()
	: m_VertexCount(0),
	m_VertexSize(0),
	m_BufferSize(0)
{

}

KVertexBufferBase::~KVertexBufferBase()
{	

}

bool KVertexBufferBase::InitMemory(size_t vertexCount, size_t vertexSize, void* pInitData)
{
	if(pInitData)
	{
		m_VertexCount = vertexCount;
		m_VertexSize = vertexSize;
		m_BufferSize = m_VertexCount * m_VertexSize;
		m_Data.resize(m_BufferSize);	
		memcpy(m_Data.data(), pInitData, m_BufferSize);
		return true;
	}
	else
	{
		m_VertexCount = 0;
		m_VertexSize = 0;
		m_BufferSize = 0;
		m_Data.clear();
		return false;
	}	
}

bool KVertexBufferBase::UnInit()
{
	m_VertexCount = 0;
	m_VertexSize = 0;
	m_BufferSize = 0;
	m_Data.clear();
	return true;
}