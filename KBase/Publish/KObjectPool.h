#pragma once
#include "KBase/Publish/KList.h"

#include <mutex>
#include <memory>
#include <assert.h>

template<typename OBJECT_TYPE>
class KObjectPool
{
protected:
	struct BlockItem;
	struct ObjectItem
	{
		BlockItem* block;
		KLIST_NODE node;
		OBJECT_TYPE object;
		size_t uMagicNum;
	};

	struct BlockItem
	{
		KLIST_NODE node;

		ObjectItem* items;

		KLIST_NODE useHead;
		KLIST_NODE freeHead;

		size_t useCount;
		size_t freeCount;
		size_t uMagicNum;
	};

	KLIST_NODE m_BlockItemHead;

	size_t m_uBlockNum;
	size_t m_uCountInBlock;
	size_t m_uMagicNum;

	std::mutex m_Lock;

	void AppendBlock()
	{
		++m_uBlockNum;

		BlockItem* pBlockItem = new BlockItem();

		KLIST_INIT(&(pBlockItem->node));
		pBlockItem->items = (ObjectItem*)malloc(m_uCountInBlock * sizeof(ObjectItem));

		KLIST_INIT(&(pBlockItem->useHead));
		KLIST_INIT(&(pBlockItem->freeHead));

		pBlockItem->useCount = 0;
		pBlockItem->freeCount = m_uCountInBlock;

		pBlockItem->uMagicNum = m_uMagicNum;

		for(size_t i = 0; i < m_uCountInBlock; ++i)
		{
			ObjectItem* pObjectItem = ((ObjectItem*)pBlockItem->items) + i;

			pObjectItem->block = pBlockItem;
			pObjectItem->uMagicNum = m_uMagicNum;

			KLIST_NODE* objectNode = &(pObjectItem->node);
			KLIST_INIT(objectNode);
			KLIST_PUSH_BACK(&pBlockItem->freeHead, objectNode);
		}

		KLIST_PUSH_BACK(&m_BlockItemHead, &(pBlockItem->node));
	}
public:
	KObjectPool() :
		m_uBlockNum(0),
		m_uCountInBlock(0),
		m_uMagicNum(0)
	{
		KLIST_INIT(&m_BlockItemHead);
	}

	~KObjectPool()
	{
		assert(m_uBlockNum == 0);
		assert(m_uCountInBlock == 0);
		assert(m_uMagicNum == 0);
	}

	void Init(size_t countInBlock)
	{
		UnInit();

		m_uBlockNum = 0;
		m_uCountInBlock = countInBlock;
		m_uMagicNum = (size_t)rand();

		KLIST_INIT(&m_BlockItemHead);
		AppendBlock();
	}

	void UnInit()
	{
		size_t nRestCount = 0;
		nRestCount = m_uBlockNum;
		for(KLIST_NODE* pNode = m_BlockItemHead.pNext; pNode != &m_BlockItemHead; --nRestCount)
		{
			KLIST_NODE* pNext = pNode->pNext;
			BlockItem* pItem = CONTAINING_RECORD(pNode, BlockItem, node);
			assert(pItem->uMagicNum == m_uMagicNum);

			for(KLIST_NODE* pUseNode = pItem->useHead.pNext; pUseNode != &(pItem->useHead);)
			{
				KLIST_NODE* pNextUseNode = pUseNode->pNext;
				ObjectItem* pObjectItem = CONTAINING_RECORD(pUseNode, ObjectItem, node);
				assert(pObjectItem->uMagicNum == m_uMagicNum);

				OBJECT_TYPE* object = &pObjectItem->object;
				object->~OBJECT_TYPE();

				KLIST_ERASE(pUseNode);
				pUseNode = pNextUseNode;
			}

			for(KLIST_NODE* pfreeNode = pItem->freeHead.pNext; pfreeNode != &(pItem->freeHead);)
			{
				KLIST_NODE* pNextFreeNode = pfreeNode->pNext;
				ObjectItem* pObjectItem = CONTAINING_RECORD(pfreeNode, ObjectItem, node);
				assert(pObjectItem->uMagicNum == m_uMagicNum);

				KLIST_ERASE(pfreeNode);
				pfreeNode = pNextFreeNode;
			}

			free(pItem->items);

			KLIST_ERASE(pNode);
			pNode = pNext;

			delete pItem;
		}

		m_uBlockNum = 0;
		assert(nRestCount == 0);

		assert(KLIST_EMPTY(&m_BlockItemHead));

		m_uCountInBlock = 0;
		m_uMagicNum = 0;
	}

	OBJECT_TYPE* Alloc()
	{
		//std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

		OBJECT_TYPE*	pRet		= nullptr;
		ObjectItem*		pObjectItem	= nullptr;
		KLIST_NODE*		pNode		= nullptr;

		for(KLIST_NODE* pNode = m_BlockItemHead.pPrev; pNode != &m_BlockItemHead;)
		{
			BlockItem* pBlockItem = CONTAINING_RECORD(pNode, BlockItem, node);
			if(pBlockItem->freeCount > 0)
			{
				KLIST_NODE* pfreeNode = pBlockItem->freeHead.pPrev;
				assert(pfreeNode != &pBlockItem->freeHead);

				pObjectItem = CONTAINING_RECORD(pfreeNode, ObjectItem, node);

				KLIST_ERASE(pfreeNode);
				--pBlockItem->freeCount;

				KLIST_PUSH_BACK(&pBlockItem->useHead, pfreeNode);
				++pBlockItem->useCount;
				break;
			}
			pNode = pNode->pPrev;
		}

		if(!pObjectItem)
		{
			AppendBlock();

			KLIST_NODE* pNode = m_BlockItemHead.pPrev;
			BlockItem* pBlockItem = CONTAINING_RECORD(pNode, BlockItem, node);

			KLIST_NODE* pfreeNode = pBlockItem->freeHead.pPrev;
			assert(pfreeNode != &pBlockItem->freeHead);

			pObjectItem = CONTAINING_RECORD(pfreeNode, ObjectItem, node);

			KLIST_ERASE(pfreeNode);
			--pBlockItem->freeCount;

			KLIST_PUSH_BACK(&pBlockItem->useHead, pfreeNode);
			++pBlockItem->useCount;
		}

		pRet = &(pObjectItem->object);
		new (pRet) OBJECT_TYPE;
		return pRet;
	}

	void Free(OBJECT_TYPE* pObject)
	{
		//std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

		ObjectItem* pObjectItem = CONTAINING_RECORD(pObject, ObjectItem, object);
		assert(pObjectItem->uMagicNum == m_uMagicNum);
		assert(&(pObjectItem->object) == pObject);

		pObject->~OBJECT_TYPE();

		BlockItem* pBlockItem = pObjectItem->block;

		KLIST_ERASE(&pObjectItem->node);
		KLIST_PUSH_BACK(&(pBlockItem->freeHead), &(pObjectItem->node));

		--pBlockItem->useCount;
		++pBlockItem->freeCount;

		if(pBlockItem->useCount == 0)
		{
			free(pBlockItem->items);
			KLIST_ERASE(&pBlockItem->node);
			delete pBlockItem;
			--m_uBlockNum;
		}
	}
};