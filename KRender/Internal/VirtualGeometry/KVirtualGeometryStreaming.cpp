#include "KVirtualGeometryStreaming.h"
#include "Internal/KRenderGlobal.h"

KVirtualGeometryStreamingManager::KVirtualGeometryStreamingManager()
	: m_FreeStreamingPageHead(nullptr)
	, m_UsedStreamingPageHead(nullptr)
	, m_FreeRootPageHead(nullptr)
	, m_UsedRootPageHead(nullptr)
	, m_MaxStreamingPages(0)
	, m_MaxRootPages(0)
	, m_CurretRootPages(0)
	, m_NumPages(0)
{
}

KVirtualGeometryStreamingManager::~KVirtualGeometryStreamingManager()
{
}

void KVirtualGeometryStreamingManager::Init(uint32_t maxStreamingPages, uint32_t maxRootPages)
{
	m_MaxStreamingPages = maxStreamingPages;
	m_MaxRootPages = maxRootPages;
	m_NumPages = m_MaxStreamingPages + m_MaxRootPages;
	m_CurretRootPages = 0;

	KRenderGlobal::RenderDevice->CreateUniformBuffer(m_StreamingDataBuffer);
	m_StreamingDataBuffer->InitMemory(sizeof(KVirtualGeometryStreaming), nullptr);
	m_StreamingDataBuffer->InitDevice();
	m_StreamingDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_STREAMING_DATA);

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_PageDataBuffer);
	m_PageDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_PAGE_STORAGE);
	m_PageDataBuffer->InitMemory(KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE * m_MaxStreamingPages + KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE * m_MaxRootPages, nullptr);
	m_PageDataBuffer->InitDevice(false, false);

	m_Pages.resize(m_NumPages);

	for (uint32_t index = 0; index < m_NumPages; ++index)
	{
		m_Pages[index] = new KVirtualGeometryActivePage();
		m_Pages[index]->index = index;
		m_Pages[index]->prev = m_Pages[index]->next = m_Pages[index];
	}

	for (uint32_t i = 0; i < m_MaxStreamingPages; ++i)
	{
		uint32_t index = m_MaxStreamingPages - 1 - i;
		if (m_FreeStreamingPageHead)
		{
			m_Pages[index]->next = m_FreeStreamingPageHead;
			m_Pages[index]->prev = m_FreeStreamingPageHead->prev;
			m_FreeStreamingPageHead->prev = m_Pages[index];
			m_Pages[index]->next = m_FreeStreamingPageHead;
		}
		m_FreeStreamingPageHead = m_Pages[index];
	}

	for (uint32_t i = m_MaxStreamingPages; i < m_NumPages; ++i)
	{
		uint32_t index = m_NumPages - 1 - i + m_MaxStreamingPages;
		if (m_FreeRootPageHead)
		{
			m_Pages[index]->next = m_FreeRootPageHead;
			m_Pages[index]->prev = m_FreeRootPageHead->prev;
			m_FreeRootPageHead->prev = m_Pages[index];
			m_Pages[index]->next = m_FreeRootPageHead;
		}
		m_FreeRootPageHead = m_Pages[index];
	}

	m_UsedStreamingPageHead = nullptr;
	m_UsedRootPageHead = nullptr;

	uint32_t frameCount = KRenderGlobal::NumFramesInFlight;

	m_StreamingRequestBuffers.resize(frameCount);
	m_PageUploadBuffers.resize(frameCount);
	m_ClusterFixupUploadBuffers.resize(frameCount);
	m_HierarchyFixupUploadBuffers.resize(frameCount);
	m_StreamingRequestClearPipelines.resize(frameCount);
	m_PageUploadPipelines.resize(frameCount);
	m_ClusterFixupUploadPipelines.resize(frameCount);
	m_HierarchyFixupUploadPipelines.resize(frameCount);

	for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_StreamingRequestBuffers[frameIndex]);
		m_StreamingRequestBuffers[frameIndex]->SetDebugName((std::string(VIRTUAL_GEOMETRY_STREAMING_REQUEST) + "_" + std::to_string(frameIndex)).c_str());
		m_StreamingRequestBuffers[frameIndex]->InitMemory(MAX_STREAMING_REQUEST * sizeof(KVirtualGeometryStreamingRequest), nullptr);
		m_StreamingRequestBuffers[frameIndex]->InitDevice(false, true);

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_PageUploadBuffers[frameIndex]);
		m_PageUploadBuffers[frameIndex]->SetDebugName((std::string(VIRTUAL_GEOMETRY_PAGE_UPLOAD) + "_" + std::to_string(frameIndex)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_ClusterFixupUploadBuffers[frameIndex]);
		m_ClusterFixupUploadBuffers[frameIndex]->SetDebugName((std::string(VIRTUAL_GEOMETRY_CLUSTER_FIXUP_UPLOAD) + "_" + std::to_string(frameIndex)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_HierarchyFixupUploadBuffers[frameIndex]);
		m_HierarchyFixupUploadBuffers[frameIndex]->SetDebugName((std::string(VIRTUAL_GEOMETRY_HIERARCHY_FIXUP_UPLOAD) + "_" + std::to_string(frameIndex)).c_str());

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_StreamingRequestClearPipelines[frameIndex]);
		m_StreamingRequestClearPipelines[frameIndex]->BindStorageBuffer(BINDING_STREAMING_REQUEST, m_StreamingRequestBuffers[frameIndex], COMPUTE_RESOURCE_OUT, true);
		m_StreamingRequestClearPipelines[frameIndex]->Init("virtualgeometry/streaming_request_clear.comp", KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv());

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_PageUploadPipelines[frameIndex]);
		m_PageUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_PAGE_DATA, m_PageDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_PageUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_PAGE_UPLOAD, m_PageUploadBuffers[frameIndex], COMPUTE_RESOURCE_IN, true);
		m_PageUploadPipelines[frameIndex]->BindUniformBuffer(BINDING_STREAMING_DATA, m_StreamingDataBuffer);
		m_PageUploadPipelines[frameIndex]->Init("virtualgeometry/page_upload.comp", KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv());

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_ClusterFixupUploadPipelines[frameIndex]);
		m_ClusterFixupUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_HIERARCHY_DATA, KRenderGlobal::VirtualGeometryManager.GetPackedHierarchyBuffer(), COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_ClusterFixupUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_PAGE_DATA, m_PageDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_ClusterFixupUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_CLUSTER_FIXUP_UPLOAD, m_ClusterFixupUploadBuffers[frameIndex], COMPUTE_RESOURCE_IN, true);
		m_ClusterFixupUploadPipelines[frameIndex]->BindUniformBuffer(BINDING_STREAMING_DATA, m_StreamingDataBuffer);
		m_ClusterFixupUploadPipelines[frameIndex]->Init("virtualgeometry/cluster_fixup_upload.comp", KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv());

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_HierarchyFixupUploadPipelines[frameIndex]);
		m_HierarchyFixupUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_HIERARCHY_DATA, KRenderGlobal::VirtualGeometryManager.GetPackedHierarchyBuffer(), COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_HierarchyFixupUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_PAGE_DATA, m_PageDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_HierarchyFixupUploadPipelines[frameIndex]->BindStorageBuffer(BINDING_HIERARCHY_FIXUP_UPLOAD, m_HierarchyFixupUploadBuffers[frameIndex], COMPUTE_RESOURCE_IN, true);
		m_HierarchyFixupUploadPipelines[frameIndex]->BindUniformBuffer(BINDING_STREAMING_DATA, m_StreamingDataBuffer);
		m_HierarchyFixupUploadPipelines[frameIndex]->Init("virtualgeometry/hierarchy_fixup_upload.comp", KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv());
	}
}

void KVirtualGeometryStreamingManager::UnInit()
{
	SAFE_UNINIT(m_StreamingDataBuffer);
	SAFE_UNINIT(m_PageDataBuffer);
	SAFE_UNINIT_CONTAINER(m_StreamingRequestBuffers);
	SAFE_UNINIT_CONTAINER(m_PageUploadBuffers);
	SAFE_UNINIT_CONTAINER(m_ClusterFixupUploadBuffers);
	SAFE_UNINIT_CONTAINER(m_HierarchyFixupUploadBuffers);
	SAFE_UNINIT_CONTAINER(m_StreamingRequestClearPipelines);
	SAFE_UNINIT_CONTAINER(m_PageUploadPipelines);
	SAFE_UNINIT_CONTAINER(m_ClusterFixupUploadPipelines);
	SAFE_UNINIT_CONTAINER(m_HierarchyFixupUploadPipelines);

	for (KVirtualGeometryActivePage* page : m_Pages)
	{
		SAFE_DELETE(page);
	}
	m_Pages.clear();

	m_CommitedPages.clear();
	m_CommitingPages.clear();

	m_ResourcePages.clear();
	m_ResourcePageStorages.clear();
	m_ResourcePageFixups.clear();
	m_ResourcePageDependencies.clear();

	m_FreeStreamingPageHead = nullptr;
	m_FreeRootPageHead = nullptr;
	m_UsedStreamingPageHead = nullptr;
	m_UsedRootPageHead = nullptr;
	m_MaxStreamingPages = 0;
	m_MaxRootPages = 0;
	m_NumPages = 0;
	m_CurretRootPages = 0;
}

bool KVirtualGeometryStreamingManager::GetGPUPageIndex(const KVirtualGeometryStreamingPageDesc& page, uint32_t& gpuPageIndex)
{
	auto it = m_CommitedPages.find(page);
	if (it != m_CommitedPages.end())
	{
		KVirtualGeometryActivePage* page = it->second;
		gpuPageIndex = page->index;
		return true;
	}
	else
	{
		gpuPageIndex = -1;
		return false;
	}
}

bool KVirtualGeometryStreamingManager::IsPageCommited(uint32_t resourceIndex, uint32_t pageStartIndex, uint32_t pageNum)
{
	for (uint32_t i = 0; i < pageNum; ++i)
	{
		KVirtualGeometryStreamingPageDesc page;
		page.resourceIndex = resourceIndex;
		page.pageIndex = pageStartIndex + i;
		uint32_t gpuPageIndex = -1;
		if (!GetGPUPageIndex(page, gpuPageIndex))
		{
			return false;
		}
	}
	return true;
}

void KVirtualGeometryStreamingManager::ApplyFixup(KVirtualGeometryActivePage* page, bool install)
{
	KVirtualGeometryStreamingPageDesc residentPage = page->residentPage;
	assert(residentPage.IsValid());
	if (residentPage.IsValid())
	{
		uint32_t resourceIndex = residentPage.resourceIndex;
		uint32_t pageIndex = residentPage.pageIndex;
		const KVirtualGeomertyFixup& fixup = m_ResourcePageFixups[resourceIndex];
		const std::vector<KVirtualGeomertyClusterFixup>& clusterFixups = fixup.clusterFixups[pageIndex];
		const std::vector<KVirtualGeomertyHierarchyFixup>& hierarchyFixups = fixup.hierarchyFixups[pageIndex];

		for (const KVirtualGeomertyClusterFixup& clusterFixup : clusterFixups)
		{
			bool dependencyFit = !install || IsPageCommited(resourceIndex, clusterFixup.dependencyPageStart, clusterFixup.dependencyPageEnd - clusterFixup.dependencyPageStart + 1);
			if (!dependencyFit)
			{
				continue;
			}

			KVirtualGeometryStreamingPageDesc fixupDesc;
			fixupDesc.resourceIndex = resourceIndex;
			fixupDesc.pageIndex = clusterFixup.fixupPage;

			uint32_t fixupGPUPageIndex = -1;
			if (GetGPUPageIndex(fixupDesc, fixupGPUPageIndex))
			{
				KVirtualGeometryClusterFixupUpdate newUpdate;
				newUpdate.clusterIndexInPage = clusterFixup.clusterIndexInPage;
				newUpdate.isLeaf = !install;
				newUpdate.gpuPageIndex = fixupGPUPageIndex;
				m_ClusterFixupUpdates.push_back(newUpdate);
			}
			else
			{
				assert(false && "shuold not reach");
			}
		}

		for (const KVirtualGeomertyHierarchyFixup& hierarchyFixup : hierarchyFixups)
		{
			bool dependencyFit = !install || IsPageCommited(resourceIndex, hierarchyFixup.dependencyPageStart, hierarchyFixup.dependencyPageEnd - hierarchyFixup.dependencyPageStart + 1);
			if (!dependencyFit)
			{
				continue;
			}

			KVirtualGeometryStreamingPageDesc fixupDesc;
			fixupDesc.resourceIndex = resourceIndex;
			fixupDesc.pageIndex = hierarchyFixup.fixupPage;

			uint32_t fixupGPUPageIndex = -1;
			if (GetGPUPageIndex(fixupDesc, fixupGPUPageIndex))
			{
				KVirtualGeometryHierarchyFixupUpdate newUpdate;
				newUpdate.partIndex = hierarchyFixup.partIndex;
				newUpdate.clusterPageIndex = install ? fixupGPUPageIndex : KVirtualGeometryDefine::INVALID_INDEX;
				newUpdate.gpuPageIndex = fixupGPUPageIndex;
				m_HierarchyFixupUpdates.push_back(newUpdate);
			}
			// Running during uninstall can occur. Because the pages belonging to other parts of the same group may be already be stream in.
			// Even thought that group can not be rendered yet.
			else if (install)
			{
				assert(false && "shuold not reach");
			}
		}
	}
}

void KVirtualGeometryStreamingManager::UpdatePageRefCount(KVirtualGeometryActivePage* page, bool install)
{
	KVirtualGeometryStreamingPageDesc residentPage = page->residentPage;
	assert(residentPage.IsValid());
	if (residentPage.IsValid())
	{
		const KVirtualGeomertyPageDependencies& dependencies = m_ResourcePageDependencies[residentPage.resourceIndex];
		const KVirtualGeomertyPageDependency& dependency = dependencies.pageDependencies[residentPage.pageIndex];
		for (uint32_t pageIndex : dependency.dependencies)
		{
			KVirtualGeometryStreamingPageDesc dependencyDesc;
			dependencyDesc.resourceIndex = residentPage.resourceIndex;
			dependencyDesc.pageIndex = pageIndex;
			auto it = m_CommitedPages.find(dependencyDesc);
			assert(it != m_CommitedPages.end());
			if (it != m_CommitedPages.end())
			{
				KVirtualGeometryActivePage* dependencyPage = it->second;
				if (install)
				{
					++dependencyPage->refCount;
				}
				else
				{
					--dependencyPage->refCount;
				}
			}
		}
	}
}

uint32_t KVirtualGeometryStreamingManager::GPUPageIndexToGPUOffset(uint32_t gpuPageIndex) const
{
	if (gpuPageIndex < m_MaxStreamingPages)
	{
		return gpuPageIndex * KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE;
	}
	else
	{
		return m_MaxStreamingPages * KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE + (gpuPageIndex - m_MaxStreamingPages) * KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE;
	}
}

uint32_t KVirtualGeometryStreamingManager::GPUPageIndexToGPUSize(uint32_t gpuPageIndex) const
{
	if (gpuPageIndex < m_MaxStreamingPages)
	{
		return KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE;
	}
	else
	{
		return KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE;
	}
}

void KVirtualGeometryStreamingManager::ApplyStreamingUpdate(IKCommandBufferPtr primaryBuffer)
{
	uint32_t currentFrame = KRenderGlobal::CurrentInFlightFrameIndex;

	uint32_t pageSize = (uint32_t)m_PageDataBuffer->GetBufferSize();
	IKStorageBufferPtr hierarchyBuffer = KRenderGlobal::VirtualGeometryManager.GetPackedHierarchyBuffer();
	uint32_t hierarchySize = (uint32_t)hierarchyBuffer->GetBufferSize();
	uint32_t hierarchyNum = hierarchySize / sizeof(KMeshClusterHierarchyPackedNode);
	uint32_t clusterFixupNum = (uint32_t)m_ClusterFixupUpdates.size();
	uint32_t hierarchyFixupNum = (uint32_t)m_HierarchyFixupUpdates.size();

	// 1.Update streaming data UBO
	{
		KVirtualGeometryStreaming streamingData;
		streamingData.misc4.x = pageSize;
		streamingData.misc4.y = m_MaxStreamingPages;
		streamingData.misc4.z = m_MaxRootPages;
		streamingData.misc4.w = (uint32_t)KRenderGlobal::VirtualGeometryManager.GetPackedHierarchyBuffer()->GetBufferSize() / sizeof(KMeshClusterHierarchyPackedNode);

		streamingData.misc5.x = clusterFixupNum;
		streamingData.misc5.y = hierarchyFixupNum;

		void* pWrite = nullptr;
		m_StreamingDataBuffer->Map(&pWrite);
		memcpy(pWrite, &streamingData, sizeof(streamingData));
		m_StreamingDataBuffer->UnMap();
		pWrite = nullptr;
	}

	// 2.Release previous upload buffer
	m_PageUploadBuffers[currentFrame]->UnInit();
	m_ClusterFixupUploadBuffers[currentFrame]->UnInit();
	m_HierarchyFixupUploadBuffers[currentFrame]->UnInit();

	// 3.Upload new pages
	if (m_ActualStreamInPages.size() > 0)
	{
		IKStorageBufferPtr pageUploadContentBuffer = m_PageUploadBuffers[currentFrame];

		std::vector<unsigned char> pageUploadContents;
		pageUploadContents.resize(pageSize);
		memset(pageUploadContents.data(), -1, pageUploadContents.size());

		for (KVirtualGeometryActivePage* page : m_ActualStreamInPages)
		{
			uint32_t resourceIndex = page->residentPage.resourceIndex;
			uint32_t pageIndex = page->residentPage.pageIndex;
			uint32_t gpuPageIndex = page->index;
			const KVirtualGeometryPageStorage& pageStorage = m_ResourcePageStorages[resourceIndex].storages[pageIndex];
			const KVirtualGeometryPage& pageDescription = m_ResourcePages[resourceIndex].pages[pageIndex];

			uint32_t gpuOffset = GPUPageIndexToGPUOffset(gpuPageIndex);
			uint32_t gpuSize = GPUPageIndexToGPUSize(gpuPageIndex);
			uint32_t currentOffset = 0;

			// Refer to KVirtualGeometryBuilder::BuildPageStorage()
			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, &pageStorage.vertexStorageByteOffset, sizeof(uint32_t));
			currentOffset += sizeof(uint32_t);

			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, &pageStorage.indexStorageByteOffset, sizeof(uint32_t));
			currentOffset += sizeof(uint32_t);

			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, &pageStorage.materialStorageByteOffset, sizeof(uint32_t));
			currentOffset += sizeof(uint32_t);

			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, &pageStorage.batchStorageByteOffset, sizeof(uint32_t));
			currentOffset += sizeof(uint32_t);

			uint32_t vertexSize = (uint32_t)pageStorage.vertexStorage.vertices.size() * sizeof(float);
			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, pageStorage.vertexStorage.vertices.data(), vertexSize);
			currentOffset += vertexSize;

			uint32_t indexSize = (uint32_t)pageStorage.indexStorage.indices.size() * sizeof(uint32_t);
			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, pageStorage.indexStorage.indices.data(), indexSize);
			currentOffset += indexSize;

			uint32_t materialSize = (uint32_t)pageStorage.materialStorage.materials.size() * sizeof(uint32_t);
			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, pageStorage.materialStorage.materials.data(), materialSize);
			currentOffset += materialSize;

			uint32_t batchSize = (uint32_t)pageStorage.batchStorage.batches.size() * sizeof(KMeshClusterBatch);
			memcpy(pageUploadContents.data() + gpuOffset + currentOffset, pageStorage.batchStorage.batches.data(), batchSize);
			currentOffset += batchSize;

			assert(currentOffset == pageDescription.dataByteSize);
		}

		pageUploadContentBuffer->InitMemory(pageSize, pageUploadContents.data());
		pageUploadContentBuffer->InitDevice(false, false);

		uint32_t numDispatch = (((pageSize + 3) / 4) + VG_GROUP_SIZE - 1) / VG_GROUP_SIZE;
		m_PageUploadPipelines[currentFrame]->Execute(primaryBuffer, numDispatch, 1, 1, nullptr);
	}

	// 4.Update fixup
	if (m_ClusterFixupUpdates.size() > 0)
	{
		IKStorageBufferPtr fixupUploadContentBuffer = m_ClusterFixupUploadBuffers[currentFrame];
		std::vector<uint32_t> fixupUploadContents;
		fixupUploadContents.resize(clusterFixupNum * 4);
		size_t bufferSize = sizeof(uint32_t) * fixupUploadContents.size();
		memset(fixupUploadContents.data(), -1, bufferSize);

		for (uint32_t index = 0; index < clusterFixupNum; ++index)
		{
			const KVirtualGeometryClusterFixupUpdate& fixup = m_ClusterFixupUpdates[index];
			uint32_t resourceIndex = m_Pages[fixup.gpuPageIndex]->residentPage.resourceIndex;
			fixupUploadContents[4 * index] = resourceIndex;
			fixupUploadContents[4 * index + 1] = fixup.gpuPageIndex;
			fixupUploadContents[4 * index + 2] = fixup.clusterIndexInPage;
			fixupUploadContents[4 * index + 3] = fixup.isLeaf;
		}

		fixupUploadContentBuffer->InitMemory(bufferSize, fixupUploadContents.data());
		fixupUploadContentBuffer->InitDevice(false, false);

		uint32_t numDispatch = (clusterFixupNum + VG_GROUP_SIZE - 1) / VG_GROUP_SIZE;
		m_ClusterFixupUploadPipelines[currentFrame]->Execute(primaryBuffer, numDispatch, 1, 1, nullptr);
	}

	if (m_HierarchyFixupUpdates.size() > 0)
	{
		IKStorageBufferPtr fixupUploadContentBuffer = m_HierarchyFixupUploadBuffers[currentFrame];
		std::vector<uint32_t> fixupUploadContents;
		fixupUploadContents.resize(2 * hierarchyFixupNum);
		size_t bufferSize = sizeof(uint32_t) * fixupUploadContents.size();
		memset(fixupUploadContents.data(), -1, bufferSize);

		for (uint32_t index = 0; index < hierarchyFixupNum; ++index)
		{
			const KVirtualGeometryHierarchyFixupUpdate& fixup = m_HierarchyFixupUpdates[index];
			uint32_t resourceIndex = m_Pages[fixup.gpuPageIndex]->residentPage.resourceIndex;
			uint32_t hierarchyStartOffset = KRenderGlobal::VirtualGeometryManager.GetResource(resourceIndex)->hierarchyPackedOffset / sizeof(KMeshClusterHierarchyPackedNode);
			const KVirtualGeomertyPageClustersData& clusterData = m_ResourcePageClustersDatas[resourceIndex];
			uint32_t hierarchyIndex = clusterData.parts[fixup.partIndex].hierarchyIndex;
			fixupUploadContents[2 * index] = hierarchyStartOffset + hierarchyIndex;
			fixupUploadContents[2 * index + 1] = fixup.clusterPageIndex;
		}

		fixupUploadContentBuffer->InitMemory(bufferSize, fixupUploadContents.data());
		fixupUploadContentBuffer->InitDevice(false, false);

		uint32_t numDispatch = (hierarchyFixupNum + VG_GROUP_SIZE - 1) / VG_GROUP_SIZE;
		m_HierarchyFixupUploadPipelines[currentFrame]->Execute(primaryBuffer, numDispatch, 1, 1, nullptr);
	}

	m_ActualStreamInPages.clear();
	m_ClusterFixupUpdates.clear();
	m_HierarchyFixupUpdates.clear();
}

void KVirtualGeometryStreamingManager::UpdateStreamingRequests(IKCommandBufferPtr primaryBuffer)
{
	uint32_t currentFrame = KRenderGlobal::CurrentInFlightFrameIndex;

	static KVirtualGeometryStreamingRequest requests[MAX_STREAMING_REQUEST];
	m_StreamingRequestBuffers[currentFrame]->Read(requests);

	std::unordered_set<KVirtualGeometryStreamingRequest> uniqueStreamingRequests;
	uint32_t numRequest = requests[0].priority;
	for (uint32_t i = 0; i < numRequest; ++i)
	{
		uniqueStreamingRequests.insert(requests[i + 1]);
	}

	std::unordered_map<KVirtualGeometryStreamingPageDesc, uint32_t> pageRequestPrioritys;
	for (const KVirtualGeometryStreamingRequest& streamingRequest : uniqueStreamingRequests)
	{
		for (uint32_t i = 0; i < streamingRequest.pageNum; ++i)
		{
			KVirtualGeometryStreamingPageDesc desc;
			desc.resourceIndex = streamingRequest.resourceIndex;
			desc.pageIndex = streamingRequest.pageStart + i;
			auto it = pageRequestPrioritys.find(desc);
			if (it == pageRequestPrioritys.end())
			{
				pageRequestPrioritys.insert({ desc, streamingRequest.priority });
			}
			else
			{
				it->second = std::max(it->second, streamingRequest.priority);
			}
		}
	}

	struct PageRequest
	{
		KVirtualGeometryStreamingPageDesc pageDesc;
		uint32_t priority;
	};
	std::vector<PageRequest> pageRequests;
	pageRequests.reserve(pageRequestPrioritys.size());

	for (auto& pair : pageRequestPrioritys)
	{
		PageRequest request;
		request.pageDesc = pair.first;
		request.priority = pair.second;
		pageRequests.push_back(request);
	}

	std::sort(pageRequests.begin(), pageRequests.end(), [](const PageRequest& lhs, const PageRequest& rhs)
	{
		return lhs.priority < rhs.priority;
	});

	m_RequestedPages.clear();
	m_RequestedPages.reserve(pageRequests.size());
	for (size_t i = 0; i < pageRequests.size(); ++i)
	{
		if (!IsRootPage(pageRequests[i].pageDesc))
		{
			m_RequestedPages.push_back(pageRequests[i].pageDesc);
		}
	}

	primaryBuffer->BeginDebugMarker("VirtualGeometry_StreamingRequestClear", glm::vec4(1.0f));
	{
		m_StreamingRequestClearPipelines[currentFrame]->Execute(primaryBuffer, 1, 1, 1);
	}
	primaryBuffer->EndDebugMarker();
}

bool KVirtualGeometryStreamingManager::IsRootPage(const KVirtualGeometryStreamingPageDesc& pageDesc)
{
	if (pageDesc.resourceIndex < m_ResourcePages.size())
	{
		const KVirtualGeometryPages& geometryPages = m_ResourcePages[pageDesc.resourceIndex];
		return pageDesc.pageIndex < geometryPages.numRootPage;
	}
	assert(false && "should not reach");
	return false;
}

bool KVirtualGeometryStreamingManager::GetPageLevel(const KVirtualGeometryStreamingPageDesc& pageDesc, uint32_t& minLevel, uint32_t& maxLevel)
{
	if (pageDesc.resourceIndex < m_ResourcePages.size())
	{
		const KVirtualGeometryPages& geometryPages = m_ResourcePages[pageDesc.resourceIndex];
		const KVirtualGeometryPage& geometryPage = geometryPages.pages[pageDesc.pageIndex];
		const KVirtualGeomertyPageClustersData& clustersData = m_ResourcePageClustersDatas[pageDesc.resourceIndex];
		maxLevel = clustersData.parts[geometryPage.clusterGroupPartStart].level;
		minLevel = clustersData.parts[geometryPage.clusterGroupPartStart + geometryPage.clusterGroupPartNum - 1].level;
		assert(minLevel <= maxLevel);
		return true;
	}
	assert(false && "should not reach");
	minLevel = maxLevel = -1;
	return false;
}

void KVirtualGeometryStreamingManager::UpdateStreamingPages(IKCommandBufferPtr primaryBuffer)
{
	// 1.LRU sort streaming page and generate pending stream in.
	LRUSortPagesByRequests();

	// 2.Pend stream in page commit and generate pending stream out.
	for (const KVirtualGeometryStreamingPageDesc& pendingStreamIn : m_PendingStreamInPages)
	{
		if (PendPageCommit(pendingStreamIn))
		{
			m_PendingUploadPages.push_back(pendingStreamIn);
		}
	}

	// 3.Do stream out page unload
	for (const KVirtualGeometryStreamingPageDesc& pendingStreamOut : m_PendingStreamOutPages)
	{
		auto it = m_CommitedPages.find(pendingStreamOut);
		if (it != m_CommitedPages.end())
		{
			KVirtualGeometryActivePage* page = it->second;
			UpdatePageRefCount(page, false);
			ApplyFixup(page, false);
			page->refCount = 0;
			page->residentPage.Invalidate();
			m_CommitedPages.erase(it);
		}
	}

	// 4.Do stream in page or root page upload.
	m_ActualStreamInPages.clear();
	for (const KVirtualGeometryStreamingPageDesc& pendingUpload : m_PendingUploadPages)
	{
		auto it = m_CommitingPages.find(pendingUpload);
		assert(!IsRootPage(pendingUpload) || it != m_CommitingPages.end());
		uint32_t minLevel = 0, maxLevel = 0;
		GetPageLevel(pendingUpload, minLevel, maxLevel);
		if (it != m_CommitingPages.end())
		{
			KVirtualGeometryActivePage* page = it->second;
			page->pendingPage.Invalidate();
			page->residentPage = pendingUpload;
			m_ActualStreamInPages.push_back(page);
			m_CommitedPages.insert({ pendingUpload ,page });
			m_CommitingPages.erase(it);
		}
	}

	// 5.Update stream in refcount and fixup
	for (KVirtualGeometryActivePage* page : m_ActualStreamInPages)
	{
		page->refCount = 0;
		UpdatePageRefCount(page, true);
		ApplyFixup(page, true);
	}

	m_PendingStreamInPages.clear();
	m_PendingStreamOutPages.clear();
	m_PendingUploadPages.clear();
	m_RequestedPages.clear();
	m_RequestedCommitedPages.clear();
}

void KVirtualGeometryStreamingManager::LRUSortPagesByRequests()
{
	for (KVirtualGeometryStreamingPageDesc& requestPage : m_RequestedPages)
	{
		auto it = m_CommitedPages.find(requestPage);
		if (it == m_CommitedPages.end())
		{
			m_PendingStreamInPages.push_back(requestPage);
		}
		else
		{
			m_RequestedCommitedPages.insert(requestPage);
			bool isRootPage = IsRootPage(requestPage);
			if (!isRootPage)
			{
				KVirtualGeometryActivePage* page = it->second;
				assert(page);
				assert(m_UsedStreamingPageHead);
				if (page == m_UsedStreamingPageHead)
				{
					continue;
				}
				// assert(ListLength(m_UsedStreamingPageHead) + ListLength(m_FreeStreamingPageHead) == m_MaxStreamingPages);
				page->prev->next = page->next;
				page->next->prev = page->prev;
				page->next = m_UsedStreamingPageHead;
				page->prev = m_UsedStreamingPageHead->prev;
				m_UsedStreamingPageHead->prev->next = page;
				m_UsedStreamingPageHead->prev = page;
				m_UsedStreamingPageHead = page;
				// assert(ListLength(m_UsedStreamingPageHead) + ListLength(m_FreeStreamingPageHead) == m_MaxStreamingPages);
			}
		}
	}
}

bool KVirtualGeometryStreamingManager::Update(IKCommandBufferPtr primaryBuffer)
{
	UpdateStreamingRequests(primaryBuffer);
	UpdateStreamingPages(primaryBuffer);
	ApplyStreamingUpdate(primaryBuffer);
	return true;
}

IKStorageBufferPtr KVirtualGeometryStreamingManager::GetStreamingRequestPipeline(uint32_t frameIndex)
{
	if (frameIndex < m_StreamingRequestBuffers.size())
	{
		return m_StreamingRequestBuffers[frameIndex];
	}
	return nullptr;
}

IKStorageBufferPtr KVirtualGeometryStreamingManager::GetPageDataBuffer()
{
	return m_PageDataBuffer;
}

IKUniformBufferPtr KVirtualGeometryStreamingManager::GetStreamingDataBuffer()
{
	return m_StreamingDataBuffer;
}

bool KVirtualGeometryStreamingManager::ReloadShader()
{
	for (IKComputePipelinePtr pipeline : m_StreamingRequestClearPipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr pipeline : m_PageUploadPipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr pipeline : m_ClusterFixupUploadPipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr pipeline : m_HierarchyFixupUploadPipelines)
	{
		pipeline->Reload();
	}
	return true;
}

uint32_t KVirtualGeometryStreamingManager::AddGeometry(const KVirtualGeometryPages& pages, const KVirtualGeometryPageStorages& storages,
	const KVirtualGeomertyFixup& fixup, const KVirtualGeomertyPageDependencies& dependencies, const KVirtualGeomertyPageClustersData& data)
{
	uint32_t numRootPage = pages.numRootPage;
	if (m_CurretRootPages + pages.numRootPage > m_MaxRootPages)
	{
		uint32_t addedPages = m_CurretRootPages + numRootPage - m_MaxRootPages;
		m_Pages.resize(m_NumPages + addedPages);
		for (uint32_t i = 0; i < addedPages; ++i)
		{
			uint32_t index = addedPages - 1 - i + m_NumPages;
			m_Pages[index] = new KVirtualGeometryActivePage();
			m_Pages[index]->index = index;
			m_Pages[index]->prev = m_Pages[index]->next = m_Pages[index];
			if (m_FreeRootPageHead)
			{
				m_Pages[index]->next = m_FreeRootPageHead;
				m_Pages[index]->prev = m_FreeRootPageHead->prev;
				m_FreeRootPageHead->prev = m_Pages[index];
				m_Pages[index]->next = m_FreeRootPageHead;
			}
			m_FreeRootPageHead = m_Pages[index];
		}
		m_NumPages += addedPages;
		m_MaxRootPages += addedPages;

		assert(m_PageDataBuffer);

		std::vector<unsigned char> oldPageStorageData;
		oldPageStorageData.resize(KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE * m_MaxStreamingPages + KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE * m_MaxRootPages);
		m_PageDataBuffer->Read(oldPageStorageData.data());

		m_PageDataBuffer->UnInit();
		m_PageDataBuffer->InitMemory(KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE * m_MaxStreamingPages + KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE * m_MaxRootPages, nullptr);
		m_PageDataBuffer->InitDevice(false, false);

		m_PageDataBuffer->Write(oldPageStorageData.data());
		// assert(ListLength(m_UsedRootPageHead) + ListLength(m_FreeRootPageHead) == m_MaxRootPages);
	}

	uint32_t resourceIndex = (uint32_t)m_ResourcePages.size();

	m_ResourcePages.push_back(pages);
	m_ResourcePageStorages.push_back(storages);
	m_ResourcePageFixups.push_back(fixup);
	m_ResourcePageDependencies.push_back(dependencies);
	m_ResourcePageClustersDatas.push_back(data);

	for (uint32_t i = 0; i < numRootPage; ++i)
	{
		KVirtualGeometryStreamingPageDesc newPage;
		newPage.resourceIndex = resourceIndex;
		newPage.pageIndex = i;
		PendPageCommit(newPage);
		m_PendingUploadPages.push_back(newPage);
	}

	m_CurretRootPages += numRootPage;

	return resourceIndex;
}

bool KVirtualGeometryStreamingManager::DependencyPageCommited(const KVirtualGeometryStreamingPageDesc& page)
{
	const KVirtualGeomertyPageDependencies& dependencies = m_ResourcePageDependencies[page.resourceIndex];
	const KVirtualGeomertyPageDependency& dependency = dependencies.pageDependencies[page.pageIndex];
	for (uint32_t pageIndex : dependency.dependencies)
	{
		KVirtualGeometryStreamingPageDesc dependencyDesc;
		dependencyDesc.resourceIndex = page.resourceIndex;
		dependencyDesc.pageIndex = pageIndex;
		auto it = m_CommitedPages.find(dependencyDesc);
		if (it == m_CommitedPages.end())
		{
			return false;
		}
	}
	return true;
}

uint32_t KVirtualGeometryStreamingManager::ListLength(KVirtualGeometryActivePage* head) const
{	
	if (head)
	{
		uint32_t length = 1;
		KVirtualGeometryActivePage* current = head;
		while (current->next != head)
		{
			++length;
			current = current->next;
		}
		return length;
	}
	return 0;
}

bool KVirtualGeometryStreamingManager::PendPageCommit(const KVirtualGeometryStreamingPageDesc& newPage)
{
	uint32_t resourceIndex = newPage.resourceIndex;
	uint32_t numRootPage = m_ResourcePages[resourceIndex].numRootPage;
	bool isRootPage = newPage.pageIndex < numRootPage;

	KVirtualGeometryActivePage* page = nullptr;
	if (isRootPage)
	{
		page = m_FreeRootPageHead;
		// Root space ensure
		assert(page);

		m_FreeRootPageHead->next->prev = m_FreeRootPageHead->prev;
		m_FreeRootPageHead->prev->next = m_FreeRootPageHead->next;

		// List length longer than 1
		if (m_FreeRootPageHead->next != m_FreeRootPageHead)
		{
			m_FreeRootPageHead = m_FreeRootPageHead->next;
		}
		else
		{
			assert(m_FreeRootPageHead->prev == m_FreeRootPageHead);
			m_FreeRootPageHead = nullptr;
		}

		if (m_UsedRootPageHead)
		{
			m_UsedRootPageHead->prev->next = page;
			page->prev = m_UsedRootPageHead->prev;
			m_UsedRootPageHead->prev = page;
			page->next = m_UsedRootPageHead;
		}
		else
		{
			page->prev = page->next = page;
		}
		m_UsedRootPageHead = page;
		// assert(ListLength(m_UsedRootPageHead) + ListLength(m_FreeRootPageHead) == m_MaxRootPages);
	}
	else if (DependencyPageCommited(newPage))
	{
		if (m_FreeStreamingPageHead)
		{
			page = m_FreeStreamingPageHead;

			m_FreeStreamingPageHead->next->prev = m_FreeStreamingPageHead->prev;
			m_FreeStreamingPageHead->prev->next = m_FreeStreamingPageHead->next;

			// List length longer than 1
			if (m_FreeStreamingPageHead->next != m_FreeStreamingPageHead)
			{
				m_FreeStreamingPageHead = m_FreeStreamingPageHead->next;
			}
			else
			{
				m_FreeStreamingPageHead = nullptr;
			}

			if (m_UsedStreamingPageHead)
			{
				page->next = m_UsedStreamingPageHead;
				page->prev = m_UsedStreamingPageHead->prev;
				m_UsedStreamingPageHead->prev->next = page;
				m_UsedStreamingPageHead->prev = page;
			}
			else
			{
				page->prev = page->next = page;
			}
			m_UsedStreamingPageHead = page;
			// assert(ListLength(m_UsedStreamingPageHead) + ListLength(m_FreeStreamingPageHead) == m_MaxStreamingPages);
		}
		else
		{
			// assert(ListLength(m_FreeStreamingPageHead) == 0);
			// assert(ListLength(m_UsedStreamingPageHead) == m_MaxStreamingPages);
			KVirtualGeometryActivePage* current = m_UsedStreamingPageHead;
			do
			{
				if (current->refCount == 0)
				{
					bool isPageRequested = m_RequestedCommitedPages.find(current->residentPage) != m_RequestedCommitedPages.end();
					if (!isPageRequested)
					{
						page = current;
						break;
					}
				}
				current = current->prev;
			} while (current != m_UsedStreamingPageHead);
		}
	}

	if (page)
	{
		if (page->residentPage.IsValid())
		{
			// Ensure
			assert(!isRootPage);
			if (!isRootPage)
			{
				m_PendingStreamOutPages.push_back(page->residentPage);
			}
		}
		if (page->pendingPage.IsValid())
		{
			m_CommitingPages.erase(page->pendingPage);
		}
		page->pendingPage = newPage;
		m_CommitingPages.insert({ newPage, page});
		return true;
	}
	else
	{
		return false;
	}
}

void KVirtualGeometryStreamingManager::RemoveGeometry(uint32_t resourceIndex)
{
	if (resourceIndex < m_ResourcePages.size())
	{
		KVirtualGeometryPages pages = m_ResourcePages[resourceIndex];
		uint32_t numRootPage = pages.numRootPage;

		for (uint32_t i = 0; i < numRootPage; ++i)
		{
			KVirtualGeometryStreamingPageDesc desc;
			desc.resourceIndex = resourceIndex;
			desc.pageIndex = i;
			auto it = m_CommitedPages.find(desc);
			if (it != m_CommitedPages.end())
			{
				KVirtualGeometryActivePage* rootPage = it->second;
				if (m_FreeRootPageHead)
				{
					rootPage->next = m_FreeRootPageHead;
					rootPage->prev = m_FreeRootPageHead->prev;
				}
				else
				{
					rootPage->prev = rootPage->next = rootPage;
				}
				// UpdatePageRefCount(rootPage, false);
				// ApplyFixup(rootPage, false);
				assert(!rootPage->pendingPage.IsValid());
				rootPage->residentPage.Invalidate();
				rootPage->refCount = 0;
				m_FreeRootPageHead = rootPage;
				m_CommitedPages.erase(it);
			}
			m_CommitingPages.erase(desc);
			m_PendingUploadPages.erase(std::remove(m_PendingUploadPages.begin(), m_PendingUploadPages.end(), desc), m_PendingUploadPages.end());
		}
		// assert(ListLength(m_UsedRootPageHead) + ListLength(m_FreeRootPageHead) == m_MaxRootPages);

		for (uint32_t i = numRootPage; i < (uint32_t)pages.pages.size(); ++i)
		{
			KVirtualGeometryStreamingPageDesc desc;
			desc.resourceIndex = resourceIndex;
			desc.pageIndex = i;
			auto it = m_CommitedPages.find(desc);
			if (it != m_CommitedPages.end())
			{
				KVirtualGeometryActivePage* streamingPage = it->second;
				if (m_FreeStreamingPageHead)
				{
					streamingPage->next = m_FreeStreamingPageHead;
					streamingPage->prev = m_FreeStreamingPageHead->prev;
				}
				else
				{
					streamingPage->prev = streamingPage->next = streamingPage;
				}
				// UpdatePageRefCount(streamingPage, false);
				// ApplyFixup(streamingPage, false);
				streamingPage->residentPage.Invalidate();
				streamingPage->refCount = 0;
				m_FreeStreamingPageHead = streamingPage;
				m_CommitedPages.erase(it);
			}
			m_CommitingPages.erase(desc);
			m_PendingUploadPages.erase(std::remove(m_PendingUploadPages.begin(), m_PendingUploadPages.end(), desc), m_PendingUploadPages.end());
			m_PendingStreamInPages.erase(std::remove(m_PendingStreamInPages.begin(), m_PendingStreamInPages.end(), desc), m_PendingStreamInPages.end());
			m_PendingStreamOutPages.erase(std::remove(m_PendingStreamOutPages.begin(), m_PendingStreamOutPages.end(), desc), m_PendingStreamOutPages.end());
		}
		// assert(ListLength(m_UsedStreamingPageHead) + ListLength(m_FreeStreamingPageHead) == m_MaxStreamingPages);

		auto AdjustPageMap = [](std::unordered_map<KVirtualGeometryStreamingPageDesc, KVirtualGeometryActivePage*>& mapToAdjust, uint32_t resourceIndex)
		{
			std::unordered_map<KVirtualGeometryStreamingPageDesc, KVirtualGeometryActivePage*> newMap;
			for (auto& pair : mapToAdjust)
			{
				KVirtualGeometryStreamingPageDesc desc = pair.first;
				if (desc.resourceIndex != resourceIndex)
				{
					if (desc.resourceIndex > resourceIndex)
					{
						desc.resourceIndex -= 1;
					}
					newMap[desc] = pair.second;
				}
			}
			newMap.swap(mapToAdjust);
		};
		AdjustPageMap(m_CommitedPages, resourceIndex);
		AdjustPageMap(m_CommitingPages, resourceIndex);

		auto AdjustPageList = [](std::vector<KVirtualGeometryStreamingPageDesc>& list, uint32_t resourceIndex)
		{
			for (KVirtualGeometryStreamingPageDesc& desc : list)
			{
				if (desc.resourceIndex > resourceIndex)
				{
					desc.resourceIndex -= 1;
				}
			}
		};
		AdjustPageList(m_RequestedPages, resourceIndex);
		AdjustPageList(m_PendingUploadPages, resourceIndex);
		AdjustPageList(m_PendingStreamInPages, resourceIndex);
		AdjustPageList(m_PendingStreamOutPages, resourceIndex);

		m_ResourcePages.erase(m_ResourcePages.begin() + resourceIndex);
		m_ResourcePageStorages.erase(m_ResourcePageStorages.begin() + resourceIndex);
		m_ResourcePageFixups.erase(m_ResourcePageFixups.begin() + resourceIndex);
		m_ResourcePageDependencies.erase(m_ResourcePageDependencies.begin() + resourceIndex);
		m_ResourcePageClustersDatas.erase(m_ResourcePageClustersDatas.begin() + resourceIndex);
		m_CurretRootPages -= numRootPage;
	}
}