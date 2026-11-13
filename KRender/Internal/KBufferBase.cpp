#include "KBufferBase.h"
#include "KBase/Publish/KInteger.h"

KVertexBufferBase::KVertexBufferBase()
	: m_VertexCount(0),
	m_VertexSize(0),
	m_BufferSize(0)
{

}

KVertexBufferBase::~KVertexBufferBase()
{	

}

bool KVertexBufferBase::InitMemory(uint32_t vertexCount, uint32_t vertexSize, const void* pInitData)
{
	const uint64_t bytes = uint64_t(vertexCount) * vertexSize;
	if (!KInteger::FitsMemorySize(bytes) || bytes > m_Data.max_size()) return false;
	m_VertexCount = vertexCount;
	m_VertexSize = vertexSize;
	m_BufferSize = bytes;
	m_Data.resize(static_cast<size_t>(bytes));
	if(pInitData)
	{
		memcpy(m_Data.data(), pInitData, static_cast<size_t>(m_BufferSize));
	}
	else
	{
		memset(m_Data.data(), 0, static_cast<size_t>(m_BufferSize));
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

bool KIndexBufferBase::InitMemory(IndexType indexType, uint32_t count, const void* pInitData)
{
	const uint64_t bytes = uint64_t(indexType == IT_32 ? 4 : 2) * count;
	if (!KInteger::FitsMemorySize(bytes) || bytes > m_Data.max_size()) return false;
	m_uIndexCount = count;
	m_BufferSize = bytes;
	m_IndexType = indexType;
	m_Data.resize(static_cast<size_t>(m_BufferSize));	
	if(pInitData)
	{
		memcpy(m_Data.data(), pInitData, static_cast<size_t>(m_BufferSize));
	}
	else
	{
		memset(m_Data.data(), 0, static_cast<size_t>(m_BufferSize));
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

bool KStorageBufferBase::InitMemory(uint64_t bufferSize, const void* pInitData)
{
	if (!KInteger::FitsMemorySize(bufferSize) || bufferSize > m_Data.max_size()) return false;
	m_BufferSize = bufferSize;
	if (bufferSize > 0)
	{
		m_Data.resize(static_cast<size_t>(bufferSize));
		if (pInitData)
		{
			memcpy(m_Data.data(), pInitData, static_cast<size_t>(bufferSize));
		}
		else
		{
			memset(m_Data.data(), 0, static_cast<size_t>(bufferSize));
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

bool KUniformBufferBase::InitMemory(uint64_t bufferSize, const void* pInitData)
{
	if (!KInteger::FitsMemorySize(bufferSize) || bufferSize > m_Data.max_size()) return false;
	m_BufferSize = bufferSize;
	if(bufferSize > 0)
	{
		m_Data.resize(static_cast<size_t>(bufferSize));	
		if(pInitData)
		{
			memcpy(m_Data.data(), pInitData, static_cast<size_t>(bufferSize));
		}
		else
		{
			memset(m_Data.data(), 0, static_cast<size_t>(bufferSize));
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