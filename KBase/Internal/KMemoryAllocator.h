#pragma once
#include "Interface/IKMemory.h"
#include "Publish/KList.h"

class KMemoryAllocator : public IKMemoryAllocator
{
	struct AllocInfo
	{
		KLIST_NODE node;
		size_t uOffset;
	};
protected:
	KLIST_NODE m_AllocHead;
public:
	KMemoryAllocator();
	~KMemoryAllocator();

	virtual void* Alloc(size_t uSize, size_t uAlignment);
	virtual void Free(void* pMemory);
};