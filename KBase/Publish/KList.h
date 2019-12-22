#pragma once

struct KLIST_NODE
{
	KLIST_NODE* pPrev;
	KLIST_NODE* pNext;
};

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((long)&(((type *)0)->field))
#endif

#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) ((type *)( \
	(char *)(address) - \
	(char *)(&((type *)0)->field)))
#endif

inline bool KLIST_INIT(KLIST_NODE* pNode)
{
	if(pNode)
	{
		pNode->pNext = pNode->pPrev = pNode;
		return true;
	}
	return false;
}

inline bool KLIST_EMPTY(KLIST_NODE* pNode)
{
	return pNode ? pNode->pNext == pNode : false;
}

inline bool KLIST_PUSH_FRONT(KLIST_NODE* pListHead, KLIST_NODE* pNode)
{
	if(pListHead && pNode)
	{
		KLIST_NODE *pTempListHead = pListHead;
		KLIST_NODE *pTempNext = pTempListHead->pNext; 
		pNode->pNext = pTempNext;
		pNode->pPrev  = pTempListHead;
		pTempNext->pPrev = pNode;
		pTempListHead->pNext = pNode;
		return true;
	}
	return false;
}

inline bool KLIST_PUSH_BACK(KLIST_NODE* pListHead, KLIST_NODE* pNode)
{
	if(pListHead && pNode)
	{
		KLIST_NODE *pTempListHead = pListHead;
		KLIST_NODE *pTempPrev = pTempListHead->pPrev;
		pNode->pNext = pTempListHead;
		pNode->pPrev  = pTempPrev;
		pTempPrev->pNext = pNode;
		pTempListHead->pPrev = pNode;
		return true;
	}
	return false;
}

inline bool KLIST_ERASE(KLIST_NODE* pNode)
{
	if(pNode)
	{
		KLIST_NODE *pTempNext = pNode->pNext;
		KLIST_NODE *pTempPrev = pNode->pPrev;
		pTempPrev->pNext = pTempNext;
		pTempNext->pPrev = pTempPrev;
		pNode->pNext = pNode->pPrev = pNode;
		return true;
	}
	return false;
}

inline bool KLIST_POP_FRONT(KLIST_NODE* pHeadList)
{
	if(pHeadList)
	{
		KLIST_ERASE(pHeadList->pNext);
		return true;
	}
	return false;
}

inline bool KLIST_POP_BACK(KLIST_NODE* pHeadList)
{
	if(pHeadList)
	{
		KLIST_ERASE(pHeadList->pPrev);
		return true;
	}
	return false;
}