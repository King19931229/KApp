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

bool KVertexBufferBase::InitMemory(size_t vertexCount, size_t vertexSize, const void* pInitData)
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

KIndexBufferBase::KIndexBufferBase()
	: m_uIndexCount(0),
	m_IndexType(IT_16),
	m_BufferSize(0)
{

}

KIndexBufferBase::~KIndexBufferBase()
{	

}

bool KIndexBufferBase::InitMemory(IndexType indexType, size_t count, const void* pInitData)
{
	if(pInitData)
	{
		m_uIndexCount = count;
		m_BufferSize = (indexType == IT_32 ? sizeof(int32_t) : sizeof(int16_t)) * count;
		m_Data.resize(m_BufferSize);	
		memcpy(m_Data.data(), pInitData, m_BufferSize);
		return true;
	}
	else
	{
		m_uIndexCount = 0;
		m_IndexType = IT_16;
		m_BufferSize = 0;
		m_Data.clear();
		return false;
	}	
}

bool KIndexBufferBase::UnInit()
{
	m_uIndexCount = 0;
	m_IndexType = IT_16;
	m_BufferSize = 0;
	m_Data.clear();
	return true;
}

KUniformBufferBase::KUniformBufferBase()
	: m_BufferSize(0)
{

}

KUniformBufferBase::~KUniformBufferBase()
{

}

bool KUniformBufferBase::InitMemory(size_t bufferSize, const void* pInitData)
{
	m_BufferSize = bufferSize;
	if(pInitData)
	{
		m_BufferSize = bufferSize;
		m_Data.resize(m_BufferSize);	
		memcpy(m_Data.data(), pInitData, m_BufferSize);
		return true;
	}
	else
	{
		m_BufferSize = 0;
		m_Data.clear();
		return false;
	}	
}

bool KUniformBufferBase::UnInit()
{
	m_BufferSize = 0;
	m_Data.clear();
	return true;
}