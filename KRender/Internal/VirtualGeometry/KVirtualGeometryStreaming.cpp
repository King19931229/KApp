#include "KVirtualGeometryStreaming.h"

KVirtualGeometryStreamingManager::KVirtualGeometryStreamingManager()
	: m_FreeStreamingPageHead(nullptr)
	, m_UsedStreamingPageHead(nullptr)
	, m_FreeRootPageHead(nullptr)
	, m_UsedRootPageHead(nullptr)
	, m_MaxStramingPages(0)
	, m_MaxRootPages(0)
	, m_CurretRootPages(0)
	, m_NumPages(0)
{
}

KVirtualGeometryStreamingManager::~KVirtualGeometryStreamingManager()
{
}

void KVirtualGeometryStreamingManager::Init(uint32_t maxStramingPages, uint32_t maxRootPages)
{
	m_MaxStramingPages = maxStramingPages;
	m_MaxRootPages = maxRootPages;
	m_NumPages = m_MaxStramingPages + m_MaxRootPages;
	m_CurretRootPages = 0;

	m_PageStates.resize(m_NumPages);
	m_Pages.resize(m_NumPages);

	for (uint32_t i = 0; i < m_NumPages; ++i)
	{
		m_Pages[i] = new KVirtualGeometryActivePage();
		m_Pages[i]->index = i;
	}

	for (uint32_t index = 0; index < m_MaxStramingPages; ++index)
	{
		if (m_FreeStreamingPageHead)
		{
			m_Pages[index]->next = m_FreeStreamingPageHead;
			m_Pages[index]->prev = m_FreeStreamingPageHead->prev;
			m_FreeStreamingPageHead->prev = m_Pages[index];
		}
		m_FreeStreamingPageHead = m_Pages[index];
	}

	for (uint32_t index = m_MaxStramingPages; index < m_NumPages; ++index)
	{
		if (m_FreeRootPageHead)
		{
			m_Pages[index]->next = m_FreeRootPageHead;
			m_Pages[index]->prev = m_FreeRootPageHead->prev;
			m_FreeRootPageHead->prev = m_Pages[index];
		}
		m_FreeRootPageHead = m_Pages[index];
	}

	m_UsedStreamingPageHead = nullptr;
	m_UsedRootPageHead = nullptr;
}

void KVirtualGeometryStreamingManager::UnInit()
{
	SAFE_UNINIT(m_PageStorageBuffer);
	m_PageStates.clear();
	for (KVirtualGeometryActivePage* page : m_Pages)
	{
		SAFE_DELETE(page);
	}
	m_Pages.clear();
	m_ResourcePages.clear();
	m_ResourcePageStorages.clear();
	m_ResourcePageFixups.clear();
	m_ResourcePageDependencies.clear();
	m_FreeStreamingPageHead = nullptr;
	m_FreeRootPageHead = nullptr;
	m_UsedStreamingPageHead = nullptr;
	m_UsedRootPageHead = nullptr;
	m_MaxStramingPages = 0;
	m_MaxRootPages = 0;
	m_NumPages = 0;
	m_CurretRootPages = 0;
}

uint32_t KVirtualGeometryStreamingManager::AddGeometry(const KVirtualGeometryPages& pages, const KVirtualGeometryPageStorages& storages,
	const KVirtualGeomertyFixup& fixup, const KVirtualGeomertyPageDependencies& dependencies)
{
	uint32_t numRootPage = pages.numRootPage;
	if (m_CurretRootPages + pages.numRootPage > m_MaxRootPages)
	{
		uint32_t addedPages = m_CurretRootPages + numRootPage - m_MaxRootPages;
		m_PageStates.resize(m_NumPages + addedPages);
		m_Pages.resize(m_NumPages + addedPages);
		for (uint32_t i = 0; i < addedPages; ++i)
		{
			uint32_t index = m_NumPages + i;
			m_Pages[index] = new KVirtualGeometryActivePage();
			m_Pages[index]->index = index;
			if (m_FreeRootPageHead)
			{
				m_Pages[index]->next = m_FreeRootPageHead;
				m_Pages[index]->prev = m_FreeRootPageHead->prev;
				m_FreeRootPageHead->prev = m_Pages[index];
			}
			m_FreeRootPageHead = m_Pages[index];
		}
		m_NumPages += addedPages;
		m_MaxRootPages += addedPages;
	}

	uint32_t resourceIndex = (uint32_t)m_ResourcePages.size();

	m_ResourcePages.push_back(pages);
	m_ResourcePageStorages.push_back(storages);
	m_ResourcePageFixups.push_back(fixup);
	m_ResourcePageDependencies.push_back(dependencies);

	for (uint32_t i = 0; i < numRootPage; ++i)
	{
		KVirtualGeometryStreamingPageDesc desc;
		desc.resourceIndex = resourceIndex;
		desc.pageIndex = i;
		KVirtualGeometryActivePage* page = m_FreeRootPageHead;
		assert(page && page->prev == nullptr);

		m_FreeRootPageHead = m_FreeRootPageHead->next;
		m_FreeRootPageHead->prev = nullptr;

		if (m_UsedRootPageHead)
		{
			assert(m_UsedRootPageHead->prev == nullptr);
			m_UsedRootPageHead->prev = page;
		}
		page->next = m_UsedRootPageHead;
		m_UsedRootPageHead = page;

		m_ActivedPages[desc] = page;
		m_PendingUploadPages.push_back(desc);
	}

	m_CurretRootPages += numRootPage;

	return resourceIndex;
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
			auto it = m_ActivedPages.find(desc);
			assert(it != m_ActivedPages.end() && "should never reach");
			if (it != m_ActivedPages.end())
			{
				KVirtualGeometryActivePage* resourceRootPage = it->second;
				resourceRootPage->next = m_FreeRootPageHead;
				resourceRootPage->prev = m_FreeRootPageHead->prev;
				m_FreeRootPageHead = resourceRootPage;
			}

			m_PendingUploadPages.erase(std::remove(m_PendingUploadPages.begin(), m_PendingUploadPages.end(), desc), m_PendingUploadPages.end());
		}

		std::unordered_map<KVirtualGeometryStreamingPageDesc, KVirtualGeometryActivePage*> newActivedPages;
		for (auto& pair : m_ActivedPages)
		{
			KVirtualGeometryStreamingPageDesc desc = pair.first;
			if (desc.resourceIndex != resourceIndex)
			{
				if (desc.resourceIndex > resourceIndex)
				{
					desc.resourceIndex -= 1;
				}
				newActivedPages[desc] = pair.second;
			}
		}

		m_ResourcePages.erase(m_ResourcePages.begin() + resourceIndex);
		m_ResourcePageStorages.erase(m_ResourcePageStorages.begin() + resourceIndex);
		m_ResourcePageFixups.erase(m_ResourcePageFixups.begin() + resourceIndex);
		m_ResourcePageDependencies.erase(m_ResourcePageDependencies.begin() + resourceIndex);

		m_ActivedPages = newActivedPages;
		m_CurretRootPages -= numRootPage;
	}
}