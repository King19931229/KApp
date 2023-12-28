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
	bool IsValid() const { return resourceIndex != KVirtualGeometryDefine::INVALID_INDEX && pageIndex != KVirtualGeometryDefine::INVALID_INDEX; }
	void Invalidate() { resourceIndex = pageIndex = KVirtualGeometryDefine::INVALID_INDEX; }
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

struct KVirtualGeometryClusterFixupUpdate
{
	uint32_t gpuPageIndex;
	uint32_t clusterIndex;
	uint32_t flag;
	uint32_t padding;
};
static_assert((sizeof(KVirtualGeometryClusterFixupUpdate) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryHierarchyFixupUpdate
{
	uint32_t gpuPageIndex;
	uint32_t groupIndex;
	uint32_t flag;
	uint32_t padding;
};
static_assert((sizeof(KVirtualGeometryHierarchyFixupUpdate) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryPageUpload
{
	uint32_t gpuPageIndex;
	uint32_t offset;
	uint32_t size;
	uint32_t padding;
};
static_assert((sizeof(KVirtualGeometryPageUpload) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryActivePage
{
	KVirtualGeometryStreamingPageDesc pendingPage;
	KVirtualGeometryStreamingPageDesc residentPage;
	uint32_t index = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t refCount = 0;
	KVirtualGeometryActivePage* prev = nullptr;
	KVirtualGeometryActivePage* next = nullptr;
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

struct KVirtualGeometryStreaming
{
	glm::uvec4 misc4;
};

class KVirtualGeometryStreamingManager
{
protected:
	static constexpr char* VIRTUAL_GEOMETRY_STREAMING_DATA = "VirtualGeometryStreamingData";
	static constexpr char* VIRTUAL_GEOMETRY_STREAMING_REQUEST = "VirtualGeometryStreamingRequest";
	static constexpr char* VIRTUAL_GEOMETRY_PAGE_STORAGE = "VirtualGeometryPageStorage";
	static constexpr char* VIRTUAL_GEOMETRY_PAGE_UPLOAD = "VirtualGeometryPageUpload";

	IKUniformBufferPtr m_StreamingDataBuffer;
	IKStorageBufferPtr m_PageDataBuffer;

	std::vector<IKStorageBufferPtr> m_StreamingRequestBuffers;
	std::vector<IKStorageBufferPtr> m_PageUploadBuffers;
	std::vector<IKComputePipelinePtr> m_StreamingRequestClearPipelines;
	std::vector<IKComputePipelinePtr> m_PageUploadPipelines;

	std::vector<KVirtualGeometryActivePage*> m_Pages;
	std::vector<KVirtualGeometryPages> m_ResourcePages;
	std::vector<KVirtualGeometryPageStorages> m_ResourcePageStorages;
	std::vector<KVirtualGeomertyFixup> m_ResourcePageFixups;
	std::vector<KVirtualGeomertyPageDependencies> m_ResourcePageDependencies;

	KVirtualGeometryActivePage* m_FreeStreamingPageHead;
	KVirtualGeometryActivePage* m_UsedStreamingPageHead;
	KVirtualGeometryActivePage* m_FreeRootPageHead;
	KVirtualGeometryActivePage* m_UsedRootPageHead;
	uint32_t m_MaxStreamingPages;
	uint32_t m_MaxRootPages;
	uint32_t m_CurretRootPages;
	uint32_t m_NumPages;
	std::unordered_map<KVirtualGeometryStreamingPageDesc, KVirtualGeometryActivePage*> m_CommitedPages;
	std::unordered_map<KVirtualGeometryStreamingPageDesc, KVirtualGeometryActivePage*> m_CommitingPages;
	std::unordered_set<KVirtualGeometryStreamingPageDesc> m_RequestedCommitedPages;
	std::vector<KVirtualGeometryActivePage*> m_ActualStreamInPages;

	std::vector<KVirtualGeometryStreamingPageDesc> m_RequestedPages;
	std::vector<KVirtualGeometryStreamingPageDesc> m_PendingStreamInPages;
	std::vector<KVirtualGeometryStreamingPageDesc> m_PendingStreamOutPages;
	std::vector<KVirtualGeometryStreamingPageDesc> m_PendingUploadPages;
	std::vector<KVirtualGeometryClusterFixupUpdate> m_ClusterFixupUpdates;
	std::vector<KVirtualGeometryHierarchyFixupUpdate> m_HierarchyFixupUpdates;

	bool IsRootPage(const KVirtualGeometryStreamingPageDesc& pageDesc);

	void LRUSortPagesByRequests();
	void UpdateStreamingRequests(IKCommandBufferPtr primaryBuffer);
	void UpdateStreamingPages(IKCommandBufferPtr primaryBuffer);
	void ApplyStreamingUpdate(IKCommandBufferPtr primaryBuffer);
	void ApplyFixup(KVirtualGeometryActivePage* page, bool install);
	void UpdatePageRefCount(KVirtualGeometryActivePage* page, bool install);
	bool PendPageCommit(const KVirtualGeometryStreamingPageDesc& newPage);
	bool DependencyPageCommited(const KVirtualGeometryStreamingPageDesc& page);
	bool GetGPUPageIndex(const KVirtualGeometryStreamingPageDesc& page, uint32_t& gpuPageIndex);
	bool IsPageCommited(uint32_t resourceIndex, uint32_t pageStartIndex, uint32_t pageNum);
	uint32_t ListLength(KVirtualGeometryActivePage* head) const;
	uint32_t GPUPageIndexToGPUOffset(uint32_t gpuPageIndex) const;
	uint32_t GPUPageIndexToGPUSize(uint32_t gpuPageIndex) const;
public:
	KVirtualGeometryStreamingManager();
	~KVirtualGeometryStreamingManager();

	void Init(uint32_t maxStreamingPages, uint32_t minRootPages);
	void UnInit();

	bool ReloadShader();
	bool Update(IKCommandBufferPtr primaryBuffer);

	IKStorageBufferPtr GetStreamingRequestPipeline(uint32_t frameIndex);
	IKStorageBufferPtr GetPageDataBuffer();
	IKUniformBufferPtr GetStreamingDataBuffer();

	uint32_t AddGeometry(const KVirtualGeometryPages& pages, const KVirtualGeometryPageStorages& storages, const KVirtualGeomertyFixup& fixup, const KVirtualGeomertyPageDependencies& dependencies);
	void RemoveGeometry(uint32_t resourceIndex);
};