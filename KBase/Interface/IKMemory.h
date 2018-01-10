#pragma once

#include "Interface/IKConfig.h"
#include <memory>

struct IKMemoryAllocator;
typedef std::shared_ptr<IKMemoryAllocator> IKMemoryAllocatorPtr;

struct IKMemoryAllocator
{
	virtual ~IKMemoryAllocator() {}
	virtual void* Alloc(size_t uSize, size_t uAlignment) = 0;
	virtual void Free(void* pMemory) = 0;
};

EXPORT_DLL IKMemoryAllocatorPtr CreateAllocator();