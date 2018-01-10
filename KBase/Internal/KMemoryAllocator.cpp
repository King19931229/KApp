#include "KMemoryAllocator.h"
#include <assert.h>

EXPORT_DLL IKMemoryAllocatorPtr CreateAllocator()
{
	return IKMemoryAllocatorPtr(new KMemoryAllocator());
}

KMemoryAllocator::KMemoryAllocator()
{
	KLIST_INIT(&m_AllocHead);
}

KMemoryAllocator::~KMemoryAllocator()
{
	//assert(KLIST_EMPTY(&m_AllocHead));
	for(KLIST_NODE* pNode = m_AllocHead.pNext, *pNext = nullptr;
		pNode != &m_AllocHead; pNode = pNext)
	{
		pNext = pNode->pNext;

		AllocInfo* pInfo = CONTAINING_RECORD(pNode, AllocInfo, node);
		void* pAllocRes = (void*)((size_t)(void*)(pInfo + 1) - sizeof(AllocInfo) - pInfo->uOffset);
		KLIST_ERASE(pNode);

		free(pAllocRes);
	}
}

void* KMemoryAllocator::Alloc(size_t uSize, size_t uAlignment)
{
	assert(!(uAlignment & (uAlignment - 1)));

	void* pAllocRes		= nullptr;
	void* pRet			= nullptr;
	AllocInfo* pInfo	= nullptr;

	size_t uAllocSize = uSize + uAlignment + sizeof(AllocInfo);
	pAllocRes = malloc(uAllocSize);

	if (pAllocRes)
	{
		size_t uOffset = uAlignment - (((size_t)pAllocRes + sizeof(AllocInfo)) & (uAlignment - 1));
		pRet = (void*)((size_t)pAllocRes + sizeof(AllocInfo) + uOffset);

		pInfo = (AllocInfo*)pRet - 1;

		KLIST_INIT(&(pInfo->node));
		pInfo->uOffset = uOffset;
	}
	assert(!((size_t)pRet % uAlignment));

	KLIST_PUSH_BACK(&m_AllocHead, &(pInfo->node));

	return pRet;
}

void KMemoryAllocator::Free(void* pMemory)
{
	assert(pMemory);

	AllocInfo* pInfo	= (AllocInfo*)pMemory - 1;
	size_t uOffset		= pInfo->uOffset;
	void* pAllocRes		= (void*)((size_t)pMemory - uOffset - sizeof(AllocInfo));
	
	KLIST_ERASE(&(pInfo->node));
	free(pAllocRes);
}