#include "KVulkanHeapAllocator.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KBase/Publish/KNumerical.h"

#include <mutex>

#define SAFE_DELETE(p)\
do\
{\
	if(p) { delete p; p = NULL; }\
}while(false);

#define KVUALKAN_HEAP_TRUELY_ALLOC
//#define KVUALKAN_HEAP_BRUTE_CHECK

namespace KVulkanHeapAllocator
{
	static VkDevice DEVICE = nullptr;
	static VkPhysicalDevice PHYSICAL_DEVICE = nullptr;

	static uint32_t MEMORY_TYPE_COUNT = 0;
	static std::vector<uint32_t> MEMORY_TYPE_TO_HEAP_IDX;
	static std::vector<VkDeviceSize> HEAP_REMAIN_SIZE;

	static VkDeviceSize ALLOC_FACTOR = 1024;
	static VkDeviceSize MAX_ALLOC_COUNT = 0;

	static std::mutex ALLOC_FREE_LOCK;

	struct PageInfo;
	struct BlockInfo
	{
		// ��block��page��ƫ����
		VkDeviceSize offset;

		// ��block��С
		VkDeviceSize size;
		int isFree;

		BlockInfo* pNext;
		BlockInfo* pPre;
		// ������Ϣ �����ͷ�ʱ����
		PageInfo* pParent;

		BlockInfo(PageInfo* _pParent)
		{
			assert(_pParent);
			isFree = true;
			offset = 0;
			size = 0;
			pNext = pPre = nullptr;
			pParent = _pParent;
		}
	};

	struct MemoryHeap;
	struct PageInfo
	{
		VkDevice vkDevice;
#ifdef KVUALKAN_HEAP_TRUELY_ALLOC
		VkDeviceMemory vkMemroy;
#else
		void* vkMemroy;
#endif
		VkDeviceSize size;
		uint32_t memoryTypeIndex;

		BlockInfo* pHead;

		PageInfo* pPre;
		PageInfo* pNext;
		
		// ������Ϣ �����ͷ�ʱ����
		MemoryHeap* pParent;
		int noShare;

		PageInfo(MemoryHeap* _pParent, VkDevice _vkDevice, VkDeviceSize _size, uint32_t _memoryTypeIndex, int _noShare)
		{
			vkDevice = _vkDevice;
			size = _size;
			memoryTypeIndex = _memoryTypeIndex;

			vkMemroy = nullptr;
			pHead = nullptr;
			pPre = pNext = nullptr;	

			pParent = _pParent;
			noShare = _noShare;
		}

		~PageInfo()
		{
			assert(vkMemroy == nullptr);
		}

		void Check()
		{
#ifdef KVUALKAN_HEAP_BRUTE_CHECK
			if(pHead)
			{
				VkDeviceSize sum = 0;
				for(BlockInfo* p = pHead; p; p = p->pNext)
				{
					sum += p->size;
				}
				assert(sum == size);
			}
#endif
		}

		BlockInfo* Alloc(VkDeviceSize sizeToFit)
		{
			if(sizeToFit > size)
			{
				return nullptr;
			}
			if(vkMemroy == nullptr)
			{
				assert(pHead == nullptr);

#ifdef KVUALKAN_HEAP_TRUELY_ALLOC
				{
					VkMemoryAllocateInfo allocInfo = {};

					allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					allocInfo.allocationSize = size;
					allocInfo.pNext = nullptr;
					allocInfo.memoryTypeIndex = memoryTypeIndex;

					VK_ASSERT_RESULT(vkAllocateMemory(DEVICE, &allocInfo, nullptr, &vkMemroy));
				}
#else
				{
					vkMemroy = malloc((size_t)size);
				}
#endif

				pHead = new BlockInfo(this);
				pHead->isFree = true;
				pHead->offset = 0;
				pHead->pPre = pHead->pNext = nullptr;
				pHead->size = size;
				pHead->pParent = this;

				// �Ѷ���Ŀռ���ѳ���
				Split(pHead, sizeToFit);
				// �ѱ�ռ��
				pHead->isFree = false;

				Check();
				return pHead;
			}
			else
			{
				assert(pHead);
				BlockInfo* pTemp = Find(sizeToFit);
				if(pTemp)
				{
					// �Ѷ���Ŀռ���ѳ���
					Split(pTemp, sizeToFit);
					// �ѱ�ռ��
					pTemp->isFree = false;

					Check();

					return pTemp;
				}
				Check();
				return nullptr;
			}
		}

		void Free(BlockInfo* pBlock)
		{
			assert(pBlock->pParent == this);
			assert(!pBlock->isFree);
			assert(pHead);
			// �ѱ��ͷ�
			pBlock->isFree = true;
			// ��ǰ���freeblock�ϲ�
			Trim(pBlock);
			// ֻʣ�����һ���ڵ� �ͷ��ڴ�
			if(pHead->pNext == nullptr)
			{
				SAFE_DELETE(pHead);
				assert(vkMemroy != nullptr);
#ifdef KVUALKAN_HEAP_TRUELY_ALLOC
				vkFreeMemory(vkDevice, vkMemroy, nullptr);
#else
				free(vkMemroy);
#endif
				vkMemroy = nullptr;
			}
		}

		// �ͷŵ�freeblock
		void Trim()
		{
			BlockInfo* pTemp = pHead;
			while(pTemp)
			{
				Trim(pTemp);
				pTemp = pTemp->pNext;
			}
		}

		void Clear()
		{
			if(vkMemroy)
			{
#ifdef KVUALKAN_HEAP_TRUELY_ALLOC
				vkFreeMemory(vkDevice, vkMemroy, nullptr);
#else
				free(vkMemroy);
#endif
				vkMemroy = nullptr;
			}

			for(BlockInfo* p = pHead; p != nullptr;)
			{
				BlockInfo* pNext = p->pNext;
				SAFE_DELETE(p);
				p = pNext;
			}
			pHead = nullptr;
		}

		BlockInfo* Find(VkDeviceSize sizeToFit)
		{
			BlockInfo* pTemp = pHead;
			while(pTemp)
			{
				if(pTemp->isFree && pTemp->size >= sizeToFit)
				{
					return pTemp;
				}
				pTemp = pTemp->pNext;
			}
			return nullptr;
		}

		bool HasSpace(VkDeviceSize sizeToFit)
		{
			if(pHead)
			{
				return Find(sizeToFit) != nullptr;
			}
			else
			{
				return sizeToFit <= size;
			}
		}

		static void Split(BlockInfo* pBlock, VkDeviceSize sizeToFit)
		{
			assert(pBlock->isFree && pBlock->size >= sizeToFit);
			if(pBlock->isFree && pBlock->size >= sizeToFit)
			{
				VkDeviceSize remainSize = pBlock->size - sizeToFit;

				BlockInfo* pNext = pBlock->pNext;
				// �����һ��block�����õ�ʣ��ռ�
				if(pNext && pNext->isFree)
				{
					pNext->offset -= remainSize;
					pNext->size += remainSize;
				}
				// ������Ѷ�һ��block����¼ʣ��ռ�
				else if(remainSize > 0)
				{
					// ��ʣ��Ŀռ���䵽�½ڵ���
					BlockInfo* pNewBlock = new BlockInfo(pBlock->pParent);
					pNewBlock->isFree = true;
					pNewBlock->size = remainSize;
					pNewBlock->offset = pBlock->offset + sizeToFit;

					pNewBlock->pNext = pNext;
					pNewBlock->pPre = pBlock;

					if(pNext)
						pNext->pPre = pNewBlock;
					pBlock->pNext = pNewBlock;
				}

				// ���·��䱾block�ռ�
				pBlock->size = sizeToFit;
			}
		}

		static void Trim(BlockInfo* pBlock)
		{
			assert(pBlock->isFree);
			BlockInfo* pTemp = nullptr;
			if(pBlock->isFree)
			{
				// ������freeblock�ϲ�
				while(pBlock->pNext && pBlock->pNext->isFree)
				{
					pBlock->size += pBlock->pNext->size;

					pTemp = pBlock->pNext;
					pBlock->pNext = pTemp->pNext;

					if(pTemp->pNext)
					{
						pTemp->pNext->pPre = pBlock;
					}
					SAFE_DELETE(pTemp);
				}
				// ��ǰ���freeblock�ϲ�
				while(pBlock->pPre && pBlock->pPre->isFree)
				{
					pBlock->size += pBlock->pPre->size;
					pBlock->offset = pBlock->pPre->offset;

					pTemp = pBlock->pPre;
					pBlock->pPre = pTemp->pPre;

					if(pTemp->pPre)
					{
						pTemp->pPre->pNext = pBlock;
					}
					SAFE_DELETE(pTemp);
				}
				// ��Ϊͷ���
				if(pBlock->pPre == nullptr)
				{
					assert(pBlock->pParent);
					pBlock->pParent->pHead = pBlock;
				}
			}
		}
	};

	struct MemoryHeap
	{
		VkDevice vkDevice;
		uint32_t memoryTypeIndex;
		PageInfo* pHead;
		PageInfo* pNoShareHead;

		// ��ǰheap���ܴ�С
		VkDeviceSize size;

		MemoryHeap(VkDevice _device, uint32_t _memoryTypeIndex)
		{
			vkDevice = _device;
			memoryTypeIndex = _memoryTypeIndex;
			pHead = nullptr;
			pNoShareHead = nullptr;
			size = 0;
		}

		void Check()
		{
#ifdef KVUALKAN_HEAP_BRUTE_CHECK
			if(pHead)
			{
				VkDeviceSize sum = 0;
				for(PageInfo* p = pHead; p; p = p->pNext)
				{
					p->Check();
					sum += p->size;
				}
				assert(sum == size);
			}
#endif
		}

		void Clear()
		{
			PageInfo* pTemp = nullptr;
			
			pTemp = pHead;
			while(pTemp)
			{
				PageInfo* pNext = pTemp->pNext;
				pTemp->Clear();
				SAFE_DELETE(pTemp);
				pTemp = pNext;
			}

			pTemp = pNoShareHead;
			while(pTemp)
			{
				PageInfo* pNext = pTemp->pNext;
				pTemp->Clear();
				SAFE_DELETE(pTemp);
				pTemp = pNext;
			}
		}

		BlockInfo* Alloc(VkDeviceSize sizeToFit, VkMemoryPropertyFlags usage)
		{
			std::lock_guard<decltype(ALLOC_FREE_LOCK)> guard(ALLOC_FREE_LOCK);

#if 0
			// �������ֻ�ܶ�ռһ��vkAllocateMemory���⴦��
			if(usage & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			{
				PageInfo* pPage = new PageInfo(this, vkDevice, sizeToFit, memoryTypeIndex, true);
				BlockInfo* pBlock = pPage->Alloc(sizeToFit);

				pPage->pNext = pNoShareHead;
				if(pNoShareHead)
				{
					pNoShareHead->pPre = pPage;
				}
				pNoShareHead = pPage;

				assert(pBlock && !pBlock->isFree);
				return pBlock;
			}
			else
#endif
			{
				// �����С������ALLOC_FACTOR��������
				// ��ÿ�η��䶼��ALLOC_FACTOR��������ʱ�� ���ܱ�֤ͬһ��page���offsetҲ��ALLOC_FACTOR��������
				sizeToFit = ((sizeToFit + ALLOC_FACTOR - 1) / ALLOC_FACTOR) * ALLOC_FACTOR;
				assert(sizeToFit % ALLOC_FACTOR == 0);
				if(pHead == nullptr)
				{
					pHead = new PageInfo(this, vkDevice, sizeToFit, memoryTypeIndex, false);
					size = sizeToFit;
					BlockInfo* pBlock = pHead->Alloc(sizeToFit);
					assert(pBlock && !pBlock->isFree);

					Check();
					return pBlock;
				}
				else
				{
					PageInfo* pPage = Find(sizeToFit);
					if(pPage)
					{
						// �Ѷ���Ŀռ���ѳ��� ������ʡʵ�ʷ�����ڴ�
						if(pPage->vkMemroy == nullptr)
						{
							Split(pPage, sizeToFit);
						}
						BlockInfo* pBlock = pPage->Alloc(sizeToFit);
						assert(pBlock && !pBlock->isFree);

						Check();
						return pBlock;
					}

					// ��ǰ����page���Ҳ����㹻�ռ� ����һ���µĲ��뵽��� ��֤heap�ܿռ�2������
					while (true)
					{
						pPage = Nail();
						VkDeviceSize newSize = KNumerical::Pow2LessEqual(size) << 1;
						size += newSize;

						PageInfo* pNewPage = new PageInfo(this, vkDevice, newSize, memoryTypeIndex, false);

						pPage->pNext = pNewPage;
						pNewPage->pPre = pPage;
						pNewPage->pNext = nullptr;

						if(pNewPage->size >= sizeToFit)
						{
							pPage = pNewPage;
							break;
						}
					}
					BlockInfo* pBlock = pPage->Alloc(sizeToFit);
					assert(pBlock && !pBlock->isFree);

					Check();
					return pBlock;
				}
			}
		}

		void Free(BlockInfo* pBlock)
		{
			std::lock_guard<decltype(ALLOC_FREE_LOCK)> guard(ALLOC_FREE_LOCK);

			assert(pBlock->pParent != nullptr);
			PageInfo* pPage = pBlock->pParent;
			assert(pPage->pParent == this);

			// �������ֻ�ܶ�ռһ��vkAllocateMemory���⴦�� ������ͬpageͬʱɾ��
			if(pPage->noShare)
			{
				if(pPage == pNoShareHead)
				{
					pNoShareHead = pPage->pNext;
				}
				if(pPage->pPre)
				{
					pPage->pPre->pNext = pPage->pNext;
				}
				if(pPage->pNext)
				{
					pPage->pNext->pPre = pPage->pPre;
				}
				pPage->Clear();
				SAFE_DELETE(pPage);
				Check();
			}
			else
			{
				pPage->Free(pBlock);
				Check();
				// �ռ�Ϊ�� ���Ժϲ��ٽ�page
				if(pPage->vkMemroy == nullptr)
				{
					Trim(pPage);
				}
				Check();
			}
		}

		PageInfo* Find(VkDeviceSize sizeToFit)
		{
			PageInfo* pTemp = pHead;
			while(pTemp)
			{
				if(pTemp->size >= sizeToFit)
				{
					if(pTemp->HasSpace(sizeToFit))
					{
						return pTemp;
					}
				}
				pTemp = pTemp->pNext;
			}
			return nullptr;
		}

		PageInfo* Nail()
		{
			PageInfo* pTemp = pHead;
			while(pTemp->pNext)
			{
				pTemp = pTemp->pNext;
			}
			return pTemp;
		}

		static void Trim(PageInfo* pPage)
		{
			assert(pPage->vkMemroy == nullptr);
			PageInfo* pTemp = nullptr;
			if(pPage->vkMemroy == nullptr)
			{
				// ������freepage�ϲ�
				while(pPage->pNext && pPage->pNext->vkMemroy == nullptr)
				{
					pPage->size += pPage->pNext->size;

					pTemp = pPage->pNext;
					pPage->pNext = pTemp->pNext;

					if(pTemp->pNext)
					{
						pTemp->pNext->pPre = pPage;
					}
					SAFE_DELETE(pTemp);
					pPage->Check();
				}
				// ��ǰ���freepage�ϲ�
				while(pPage->pPre && pPage->pPre->vkMemroy == nullptr)
				{
					pPage->size += pPage->pPre->size;

					pTemp = pPage->pPre;
					pPage->pPre = pTemp->pPre;

					if(pTemp->pPre)
					{
						pTemp->pPre->pNext = pPage;
					}
					SAFE_DELETE(pTemp);
					pPage->Check();
				}
				// ��Ϊͷ���
				if(pPage->pPre == nullptr)
				{
					assert(pPage->pParent);
					pPage->pParent->pHead = pPage;
				}
			}
		}

		static void Split(PageInfo* pPage, VkDeviceSize sizeToFit)
		{
			assert(pPage->vkMemroy == nullptr && pPage->size >= sizeToFit);
			if(pPage->vkMemroy == nullptr && pPage->size >= sizeToFit)
			{
				// page���´�С����ALLOC_FACTOR�������� �޷�����
				if(sizeToFit % ALLOC_FACTOR != 0)
				{
					return;
				}

				VkDeviceSize remainSize = pPage->size - sizeToFit;

				PageInfo* pNext = pPage->pNext;
				// �����һ��page�����õ�ʣ��ռ�
				if(pNext && pNext->vkMemroy == nullptr)
				{
					pNext->size += remainSize;
				}
				// ������Ѷ�һ��page����¼ʣ��ռ�
				else if(remainSize > 0)
				{
					// ��ʣ��Ŀռ���䵽�½ڵ���
					PageInfo* pNewPage = new PageInfo(pPage->pParent, pPage->vkDevice, remainSize, pPage->memoryTypeIndex, false);
					pNewPage->vkMemroy = nullptr;
					pNewPage->size = remainSize;

					pNewPage->pNext = pNext;
					pNewPage->pPre = pPage;

					if(pNext)
						pNext->pPre = pNewPage;
					pPage->pNext = pNewPage;
				}

				// ���·��䱾page�ռ�
				pPage->size = sizeToFit;
			}
		}
	};

	static std::vector<MemoryHeap*> MEMORY_TYPE_TO_HEAP;

	bool Init()
	{
		if(KVulkanGlobal::deviceReady)
		{
			DEVICE = KVulkanGlobal::device;
			PHYSICAL_DEVICE = KVulkanGlobal::physicalDevice;

			VkPhysicalDeviceProperties deviceProperties = {};

			vkGetPhysicalDeviceProperties(PHYSICAL_DEVICE, &deviceProperties);

			MAX_ALLOC_COUNT = deviceProperties.limits.maxMemoryAllocationCount;
			/*
			Linear buffer 0xXX is aliased with non-linear image 0xXX which may indicate a bug.
			For further info refer to the Buffer-Image Granularity section of the Vulkan specification. >
			(https://www.khronos.org/registry/vulkan/specs/1.0-extensions/xhtml/vkspec.html#resources-bufferimagegranularity)
			*/
			// ��������image��buffer����ͬһ��VkDeviceMemory ���ÿ�η���ռ�õĿռ��С�����Ǹ�factor��������
			ALLOC_FACTOR = deviceProperties.limits.bufferImageGranularity;

			VkPhysicalDeviceMemoryProperties memoryProperties = {};
			vkGetPhysicalDeviceMemoryProperties(PHYSICAL_DEVICE, &memoryProperties);

			MEMORY_TYPE_COUNT = memoryProperties.memoryTypeCount;
			MEMORY_TYPE_TO_HEAP_IDX.resize(memoryProperties.memoryTypeCount);
			MEMORY_TYPE_TO_HEAP.resize(memoryProperties.memoryTypeCount);
			for(uint32_t memTypeIdx = 0; memTypeIdx < memoryProperties.memoryTypeCount; ++memTypeIdx)
			{
				MEMORY_TYPE_TO_HEAP_IDX[memTypeIdx] = memoryProperties.memoryTypes[memTypeIdx].heapIndex;
				MEMORY_TYPE_TO_HEAP[memTypeIdx] = new MemoryHeap(KVulkanGlobal::device, memTypeIdx);
			}

			HEAP_REMAIN_SIZE.resize(memoryProperties.memoryHeapCount);
			for(uint32_t memHeapIdx = 0; memHeapIdx < memoryProperties.memoryHeapCount; ++memHeapIdx)
			{
				HEAP_REMAIN_SIZE[memHeapIdx] = memoryProperties.memoryHeaps[memHeapIdx].size;
			}

			return true;
		}
		else
		{
#ifdef KVUALKAN_HEAP_TRUELY_ALLOC
			return false;
#else
			MEMORY_TYPE_COUNT = 1;
			MEMORY_TYPE_TO_HEAP.resize(1);
			MEMORY_TYPE_TO_HEAP[0] = new MemoryHeap(KVulkanGlobal::device, 0);
			return true;
#endif
		}
	}

	bool UnInit()
	{
		for(uint32_t memoryTypeIndex = 0; memoryTypeIndex < MEMORY_TYPE_COUNT; ++memoryTypeIndex)
		{
			MemoryHeap* pHeap = MEMORY_TYPE_TO_HEAP[memoryTypeIndex];
			if(pHeap)
			{
				pHeap->Clear();
				SAFE_DELETE(pHeap);
			}
		}

		MEMORY_TYPE_COUNT = 0;
		MEMORY_TYPE_TO_HEAP.clear();
		MEMORY_TYPE_TO_HEAP_IDX.clear();
		HEAP_REMAIN_SIZE.clear();

		return true;
	}

	bool Alloc(VkDeviceSize size, uint32_t memoryTypeIndex, VkMemoryPropertyFlags usage, AllocInfo& info)
	{
		if(memoryTypeIndex < MEMORY_TYPE_COUNT)
		{
			MemoryHeap* pHeap = MEMORY_TYPE_TO_HEAP[memoryTypeIndex];

			info.internalData = pHeap->Alloc(size, usage);
			if(info.internalData)
			{
				BlockInfo* pBlock = (BlockInfo*)info.internalData;
				PageInfo* pPage = (PageInfo*)pBlock->pParent;

				info.vkMemroy = pPage->vkMemroy;
				info.vkOffset = pBlock->offset;

				return true;
			}
		}
		return false;
	}

	bool Free(const AllocInfo& data)
	{
		BlockInfo* pBlock = (BlockInfo*)data.internalData;
		if(pBlock)
		{
			MemoryHeap* pHeap = pBlock->pParent->pParent;
			pHeap->Free(pBlock);
			return true;
		}
		return false;
	}
}