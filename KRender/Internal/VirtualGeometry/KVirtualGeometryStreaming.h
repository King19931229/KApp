#pragma once
#include "KVirtualGeomerty.h"
#include "KRender/Interface/IKBuffer.h"
#include <queue>

struct KVirtualGeometryStreamingPageDesc
{
	uint32_t resourceIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t pageIndex = KVirtualGeometryDefine::INVALID_INDEX;
	friend bool operator<(const KVirtualGeometryStreamingPageDesc& lhs, const KVirtualGeometryStreamingPageDesc& rhs);
	friend bool operator==(const KVirtualGeometryStreamingPageDesc& lhs, const KVirtualGeometryStreamingPageDesc& rhs);
};

inline bool operator<(const KVirtualGeometryStreamingPageDesc& lhs, const KVirtualGeometryStreamingPageDesc& rhs)
{
	if (lhs.resourceIndex != rhs.resourceIndex)
	{
		return lhs.resourceIndex < rhs.resourceIndex;
	}
	return lhs.pageIndex < rhs.pageIndex;
}

inline bool operator==(const KVirtualGeometryStreamingPageDesc& lhs, const KVirtualGeometryStreamingPageDesc& rhs)
{
	if (lhs.resourceIndex != rhs.resourceIndex)
	{
		return false;
	}
	return lhs.pageIndex == rhs.pageIndex;
}

struct KVirtualGeometryActivePage
{
	uint32_t index = KVirtualGeometryDefine::INVALID_INDEX;
	KVirtualGeometryActivePage* prev = nullptr;
	KVirtualGeometryActivePage* next = nullptr;
};

struct KVirtualGeometryActivePageState
{
	uint32_t resourceIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t pageIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t refCount = 0;
};

template<>
struct std::hash<KVirtualGeometryStreamingPageDesc>
{
	inline std::size_t operator()(const KVirtualGeometryStreamingPageDesc& desc) const
	{
		std::size_t hash = 0;
		KHash::HashCombine(hash, desc.pageIndex);
		KHash::HashCombine(hash, desc.resourceIndex);
		return hash;
	}
};

class KVirtualGeometryStreamingManager
{
protected:
	IKStorageBufferPtr m_PageStorageBuffer;
	std::vector<KVirtualGeometryActivePageState> m_PageStates;
	std::vector<KVirtualGeometryActivePage*> m_Pages;

	std::vector<KVirtualGeometryPages> m_ResourcePages;
	std::vector<KVirtualGeometryPageStorages> m_ResourcePageStorages;
	std::vector<KVirtualGeomertyFixup> m_ResourcePageFixups;
	std::vector<KVirtualGeomertyPageDependencies> m_ResourcePageDependencies;

	KVirtualGeometryActivePage* m_FreeStreamingPageHead;
	KVirtualGeometryActivePage* m_UsedStreamingPageHead;
	KVirtualGeometryActivePage* m_FreeRootPageHead;
	KVirtualGeometryActivePage* m_UsedRootPageHead;
	uint32_t m_MaxStramingPages;
	uint32_t m_MaxRootPages;
	uint32_t m_CurretRootPages;
	uint32_t m_NumPages;
	std::unordered_map<KVirtualGeometryStreamingPageDesc, KVirtualGeometryActivePage*> m_ActivedPages;
	std::vector<KVirtualGeometryStreamingPageDesc> m_PendingUploadPages;
public:
	KVirtualGeometryStreamingManager();
	~KVirtualGeometryStreamingManager();

	void Init(uint32_t maxStramingPages, uint32_t minRootPages);
	void UnInit();

	uint32_t AddGeometry(const KVirtualGeometryPages& pages, const KVirtualGeometryPageStorages& storages, const KVirtualGeomertyFixup& fixup, const KVirtualGeomertyPageDependencies& dependencies);
	void RemoveGeometry(uint32_t resourceIndex);
};