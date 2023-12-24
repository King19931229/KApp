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

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_PageStorageBuffer);
	m_PageStorageBuffer->SetDebugName(VIRTUAL_GEOMETRY_PAGE_STORAGE);
	m_PageStorageBuffer->InitMemory(KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE * m_MaxStreamingPages + KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE * m_MaxRootPages, nullptr);
	m_PageStorageBuffer->InitDevice(false, false);

	m_Pages.resize(m_NumPages);

	for (uint32_t index = 0; index < m_NumPages; ++index)
	{
		m_Pages[index] = new KVirtualGeometryActivePage();
		m_Pages[index]->index = index;
		m_Pages[index]->prev = m_Pages[index]->next = m_Pages[index];
	}

	for (uint32_t index = 0; index < m_MaxStreamingPages; ++index)
	{
		if (m_FreeStreamingPageHead)
		{
			m_Pages[index]->next = m_FreeStreamingPageHead;
			m_Pages[index]->prev = m_FreeStreamingPageHead->prev;
			m_FreeStreamingPageHead->prev = m_Pages[index];
			m_Pages[index]->next = m_FreeStreamingPageHead;
		}
		m_FreeStreamingPageHead = m_Pages[index];
	}

	for (uint32_t index = m_MaxStreamingPages; index < m_NumPages; ++index)
	{
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
	m_PageUploadDescBuffers.resize(frameCount);
	m_PageUploadContentBuffers.resize(frameCount);
	m_StreamingRequestClearPipelines.resize(frameCount);

	for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_StreamingRequestBuffers[frameIndex]);
		m_StreamingRequestBuffers[frameIndex]->SetDebugName((std::string(VIRTUAL_GEOMETRY_STREAMING_REQUEST) + "_" + std::to_string(frameIndex)).c_str());
		m_StreamingRequestBuffers[frameIndex]->InitMemory(MAX_STREAMING_REQUEST * sizeof(KVirtualGeometryStreamingRequest), nullptr);
		m_StreamingRequestBuffers[frameIndex]->InitDevice(false, true);

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_PageUploadDescBuffers[frameIndex]);
		m_PageUploadDescBuffers[frameIndex]->SetDebugName((std::string(VIRTUAL_GEOMETRY_PAGE_UPLOAD_DESC) + "_" + std::to_string(frameIndex)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_PageUploadContentBuffers[frameIndex]);
		m_PageUploadContentBuffers[frameIndex]->SetDebugName((std::string(VIRTUAL_GEOMETRY_PAGE_UPLOAD_CONTENT) + "_" + std::to_string(frameIndex)).c_str());

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_StreamingRequestClearPipelines[frameIndex]);
		m_StreamingRequestClearPipelines[frameIndex]->BindStorageBuffer(BINDING_STREAMING_REQUEST, m_StreamingRequestBuffers[frameIndex], COMPUTE_RESOURCE_OUT, false);
		m_StreamingRequestClearPipelines[frameIndex]->Init("virtualgeometry/streaming_request_clear.comp", KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv());
	}
}

void KVirtualGeometryStreamingManager::UnInit()
{
	SAFE_UNINIT(m_PageStorageBuffer);
	SAFE_UNINIT_CONTAINER(m_StreamingRequestBuffers);
	SAFE_UNINIT_CONTAINER(m_PageUploadDescBuffers);
	SAFE_UNINIT_CONTAINER(m_PageUploadContentBuffers);
	SAFE_UNINIT_CONTAINER(m_StreamingRequestClearPipelines);

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

void KVirtualGeometryStreamingManager::ApplyFixup(KVirtualGeometryActivePage* page, bool install)
{
}

void KVirtualGeometryStreamingManager::UpdatePageRefCount(KVirtualGeometryActivePage* page, bool install)
{
}

void KVirtualGeometryStreamingManager::ApplyPageUpdate(IKCommandBufferPtr primaryBuffer)
{
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
	m_PendingStreamInPages.clear();

	// 3.Do stream out page unload.
	for (const KVirtualGeometryStreamingPageDesc& pendingStreamOut : m_PendingStreamOutPages)
	{
		auto it = m_CommitedPages.find(pendingStreamOut);
		if (it != m_CommitedPages.end())
		{
			KVirtualGeometryActivePage* page = it->second;
			UpdatePageRefCount(page, false);
			ApplyFixup(page, false);
			m_CommitedPages.erase(it);
			page->residentPage.Invalidate();
		}
	}
	m_PendingStreamOutPages.clear();
	
	// 4.Do stream in page or root page upload.
	for (const KVirtualGeometryStreamingPageDesc& pendingUpload : m_PendingUploadPages)
	{
		auto it = m_CommitingPages.find(pendingUpload);
		assert(!IsRootPage(pendingUpload) || it != m_CommitingPages.end());
		if (it != m_CommitingPages.end())
		{
			KVirtualGeometryActivePage* page = it->second;
			UpdatePageRefCount(page, true);
			ApplyFixup(page, true);
			m_CommitingPages.erase(it);
			page->pendingPage.Invalidate();
			page->residentPage = pendingUpload;
			m_CommitedPages.insert({ pendingUpload ,page });
		}
	}
	m_PendingUploadPages.clear();
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

				page->next = m_UsedStreamingPageHead;
				page->prev = m_UsedStreamingPageHead->prev;
				m_UsedStreamingPageHead->prev = page;
				m_UsedStreamingPageHead->prev->next = page;

				m_UsedStreamingPageHead = page;
			}
		}
	}
	m_RequestedPages.clear();
}

bool KVirtualGeometryStreamingManager::Update(IKCommandBufferPtr primaryBuffer)
{
	UpdateStreamingRequests(primaryBuffer);
	UpdateStreamingPages(primaryBuffer);
	ApplyPageUpdate(primaryBuffer);
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

bool KVirtualGeometryStreamingManager::ReloadShader()
{
	for (IKComputePipelinePtr pipeline : m_StreamingRequestClearPipelines)
	{
		pipeline->Reload();
	}
	return true;
}

uint32_t KVirtualGeometryStreamingManager::AddGeometry(const KVirtualGeometryPages& pages, const KVirtualGeometryPageStorages& storages,
	const KVirtualGeomertyFixup& fixup, const KVirtualGeomertyPageDependencies& dependencies)
{
	uint32_t numRootPage = pages.numRootPage;
	if (m_CurretRootPages + pages.numRootPage > m_MaxRootPages)
	{
		uint32_t addedPages = m_CurretRootPages + numRootPage - m_MaxRootPages;
		m_Pages.resize(m_NumPages + addedPages);
		for (uint32_t i = 0; i < addedPages; ++i)
		{
			uint32_t index = m_NumPages + i;
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

		assert(m_PageStorageBuffer);

		std::vector<unsigned char> oldPageStorageData;
		oldPageStorageData.resize(KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE * m_MaxStreamingPages + KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE * m_MaxRootPages);
		m_PageStorageBuffer->Read(oldPageStorageData.data());

		m_PageStorageBuffer->UnInit();
		m_PageStorageBuffer->InitMemory(KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE * m_MaxStreamingPages + KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE * m_MaxRootPages, nullptr);
		m_PageStorageBuffer->InitDevice(false, false);

		m_PageStorageBuffer->Write(oldPageStorageData.data());
	}

	uint32_t resourceIndex = (uint32_t)m_ResourcePages.size();

	m_ResourcePages.push_back(pages);
	m_ResourcePageStorages.push_back(storages);
	m_ResourcePageFixups.push_back(fixup);
	m_ResourcePageDependencies.push_back(dependencies);

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
	}
	else
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
				m_UsedStreamingPageHead->prev = page;
				m_UsedStreamingPageHead->prev->next = page;
			}
			else
			{
				m_UsedStreamingPageHead = page;
				m_UsedStreamingPageHead->prev = m_UsedStreamingPageHead->next = m_UsedStreamingPageHead;
			}
		}
		else if (true)
		{
			page = m_UsedStreamingPageHead->prev;
			m_UsedStreamingPageHead = m_UsedStreamingPageHead->prev;
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
				UpdatePageRefCount(rootPage, false);
				assert(!rootPage->pendingPage.IsValid());
				rootPage->residentPage.Invalidate();
				m_FreeRootPageHead = rootPage;
				m_CommitedPages.erase(it);
			}
			m_CommitingPages.erase(desc);
			m_PendingUploadPages.erase(std::remove(m_PendingUploadPages.begin(), m_PendingUploadPages.end(), desc), m_PendingUploadPages.end());
		}

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
				UpdatePageRefCount(streamingPage, false);
				streamingPage->residentPage.Invalidate();
				m_FreeStreamingPageHead = streamingPage;
				m_CommitedPages.erase(it);
			}
			m_CommitingPages.erase(desc);
			m_PendingUploadPages.erase(std::remove(m_PendingUploadPages.begin(), m_PendingUploadPages.end(), desc), m_PendingUploadPages.end());
			m_PendingStreamInPages.erase(std::remove(m_PendingStreamInPages.begin(), m_PendingStreamInPages.end(), desc), m_PendingStreamInPages.end());
			m_PendingStreamOutPages.erase(std::remove(m_PendingStreamOutPages.begin(), m_PendingStreamOutPages.end(), desc), m_PendingStreamOutPages.end());
		}

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

		m_ResourcePages.erase(m_ResourcePages.begin() + resourceIndex);
		m_ResourcePageStorages.erase(m_ResourcePageStorages.begin() + resourceIndex);
		m_ResourcePageFixups.erase(m_ResourcePageFixups.begin() + resourceIndex);
		m_ResourcePageDependencies.erase(m_ResourcePageDependencies.begin() + resourceIndex);
		m_CurretRootPages -= numRootPage;
	}
}