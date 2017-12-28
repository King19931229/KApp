#pragma once
#include "Publish/KList.h"
#include <memory>
#include <assert.h>

template<typename OBJECT_TYPE>
class KObjectPool
{
protected:
	struct ListItem
	{
		KLIST_NODE node;
		OBJECT_TYPE object;
		unsigned uMagicNum;
	};

	KLIST_NODE m_UseListHead;
	KLIST_NODE m_FreeListHead;

	unsigned m_uUseCount;
	unsigned m_uFreeCount;
	unsigned m_uInitCount;
	unsigned m_uMagicNum;
public:
	KObjectPool()
		: m_uUseCount(0),
		m_uFreeCount(0),
		m_uInitCount(0),
		m_uMagicNum(0)
	{
		KLIST_INIT(&m_UseListHead);
		KLIST_INIT(&m_FreeListHead);
	}

	~KObjectPool()
	{
		assert(m_uUseCount == 0);
		assert(m_uFreeCount == 0);
		assert(m_uInitCount == 0);
	}

	void Init(unsigned uCount)
	{
		assert(m_uUseCount == 0);
		assert(m_uFreeCount == 0);
		assert(m_uInitCount == 0);

		UnInit();
		m_uMagicNum = (unsigned)rand();

		m_uInitCount = uCount;

		KLIST_INIT(&m_UseListHead);
		KLIST_INIT(&m_FreeListHead);

		for(unsigned i = 0; i < m_uInitCount; ++i)
		{
			ListItem* pNewItem = new ListItem();
			pNewItem->uMagicNum = m_uMagicNum;

			KLIST_INIT(&(pNewItem->node));
			KLIST_PUSH_BACK(&m_FreeListHead, &(pNewItem->node));
		}

		m_uFreeCount = m_uInitCount;
		m_uUseCount = 0;
	}

	void UnInit()
	{
		unsigned nRestCount = 0;
		
		nRestCount = m_uUseCount;
		for(KLIST_NODE* pNode = m_UseListHead.pNext; pNode != &m_UseListHead; --nRestCount)
		{
			KLIST_NODE* pNext = pNode->pNext;

			ListItem* pItem = CONTAINING_RECORD(pNode, ListItem, node);
			OBJECT_TYPE* pObject = &(pItem->object);
			pObject->~OBJECT_TYPE();

			KLIST_ERASE(pNode);
			delete pNode;
			pNode = pNext;
		}
		m_uUseCount = 0;
		assert(nRestCount == 0);
		assert(KLIST_EMPTY(&m_UseListHead));

		nRestCount = m_uFreeCount;
		for(KLIST_NODE* pNode = m_FreeListHead.pNext; pNode != &m_FreeListHead; --nRestCount)
		{
			KLIST_NODE* pNext = pNode->pNext;

			ListItem* pItem = CONTAINING_RECORD(pNode, ListItem, node);
			OBJECT_TYPE* pObject = &(pItem->object);

			KLIST_ERASE(pNode);
			delete pNode;

			pNode = pNext;
		}
		m_uFreeCount = 0;
		assert(nRestCount == 0);
		assert(KLIST_EMPTY(&m_FreeListHead));

		m_uInitCount = 0;
		m_uMagicNum = 0;
	}

	OBJECT_TYPE* Alloc()
	{
		OBJECT_TYPE*	pRet	= nullptr;
		ListItem*		pItem	= nullptr;
		KLIST_NODE*		pNode	= nullptr;

		if(m_uFreeCount > 0)
		{
			pNode = m_FreeListHead.pPrev;
			assert(pNode != &m_FreeListHead);

			pItem = CONTAINING_RECORD(pNode, ListItem, node);

			KLIST_ERASE(pNode);
			--m_uFreeCount;
		}
		else
		{
			pItem = new ListItem();
			pItem->uMagicNum = m_uMagicNum;

			pNode = &(pItem->node);
			KLIST_INIT(pNode);
		}
		pRet = &(pItem->object);
		KLIST_PUSH_BACK(&m_UseListHead, pNode);
		++m_uUseCount;
		new (pRet) OBJECT_TYPE;
		return pRet;
	}

	void Free(OBJECT_TYPE* pObject)
	{
		ListItem* pItem = CONTAINING_RECORD(pObject, ListItem, object);
		assert(pItem->uMagicNum == m_uMagicNum);
		assert(m_uUseCount > 0);

		KLIST_ERASE(&pItem->node);
		assert(&(pItem->object) == pObject);
		pObject->~OBJECT_TYPE();
		--m_uUseCount;

		KLIST_PUSH_BACK(&m_FreeListHead, &(pItem->node));
		++m_uFreeCount;
	}

	void Shrink_to_fit()
	{
		while(m_uFreeCount > m_uInitCount)
		{
			KLIST_NODE* pNode = m_FreeListHead.pPrev;
			assert(pNode != &m_FreeListHead);

			ListItem* pItem = CONTAINING_RECORD(pNode, ListItem, node);

			KLIST_ERASE(pNode);
			delete pItem;

			--m_uFreeCount;
		}
	}
};