#include "KBufferBase.h"

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
	m_VertexCount = vertexCount;
	m_VertexSize = vertexSize;
	m_BufferSize = m_VertexCount * m_VertexSize;
	m_Data.resize(m_BufferSize);
	if(pInitData)
	{
		memcpy(m_Data.data(), pInitData, m_BufferSize);
	}
	else
	{
		memset(m_Data.data(), 0, m_BufferSize);
	}
	return true;
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
	m_uIndexCount = count;
	m_BufferSize = (indexType == IT_32 ? 4 : 2) * count;
	m_IndexType = indexType;
	m_Data.resize(m_BufferSize);	
	if(pInitData)
	{
		memcpy(m_Data.data(), pInitData, m_BufferSize);
	}
	else
	{
		memset(m_Data.data(), 0, m_BufferSize);
	}
	return true;
}

bool KIndexBufferBase::UnInit()
{
	m_uIndexCount = 0;
	m_IndexType = IT_16;
	m_BufferSize = 0;
	m_Data.clear();
	return true;
}

KStorageBufferBase::KStorageBufferBase()
	: m_BufferSize(0)
	, m_bIndirect(false)
{

}

KStorageBufferBase::~KStorageBufferBase()
{

}

bool KStorageBufferBase::InitMemory(size_t bufferSize, const void* pInitData)
{
	m_BufferSize = bufferSize;
	if (bufferSize > 0)
	{
		m_Data.resize(bufferSize);
		if (pInitData)
		{
			memcpy(m_Data.data(), pInitData, bufferSize);
		}
		else
		{
			memset(m_Data.data(), 0, bufferSize);
		}
		return true;
	}
	else
	{
		m_Data.clear();
		return false;
	}
}

bool KStorageBufferBase::InitDevice(bool indirect, bool hostVisble)
{
	(hostVisble);
	m_bIndirect = indirect;
	return true;
}

bool KStorageBufferBase::UnInit()
{
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
	if(bufferSize > 0)
	{
		m_Data.resize(bufferSize);	
		if(pInitData)
		{
			memcpy(m_Data.data(), pInitData, bufferSize);
		}
		else
		{
			memset(m_Data.data(), 0, bufferSize);
		}
		return true;
	}
	else
	{
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