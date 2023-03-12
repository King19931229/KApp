#include "KVulkanHeapAllocator.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KBase/Publish/KNumerical.h"
#include "KBase/Interface/IKLog.h"
#include <algorithm>

#include <mutex>

#define KVUALKAN_HEAP_TRUELY_ALLOC
//#define KVUALKAN_HEAP_BRUTE_CHECK

namespace KVulkanHeapAllocator
{
	struct BlockInfo;
	struct PageInfo;
	struct MemoryHeap;

	static VkDevice DEVICE = nullptr;
	static VkPhysicalDevice PHYSICAL_DEVICE = nullptr;

	static uint32_t MEMORY_TYPE_COUNT = 0;

	static std::vector<VkDeviceSize> MIN_PAGE_SIZE;
	static std::vector<VkDeviceSize> HEAP_REMAIN_SIZE;
	static std::vector<MemoryHeap*> MEMORY_TYPE_TO_HEAP;

	static VkDeviceSize ALLOC_FACTOR = 1024;
	static VkDeviceSize MAX_ALLOC_COUNT = 0;
	static VkDeviceSize BLOCK_SIZE_FACTOR = 4;

	static std::mutex ALLOC_FREE_LOCK;

	enum MemoryAllocateType
	{
		MAT_DEFAULT,
		MAT_ACCELERATION_STRUCTURE,
		MAT_DEVICE_ADDRESS,
		MAT_COUNT
	};

	struct MemoryAllocateTypeProperty
	{
		bool needDeviceAddress;
	};

	static MemoryAllocateTypeProperty MEMORY_ALLOCATE_TYPE_PROPERITES[MAT_COUNT] =
	{
		// MAT_DEFAULT
		{ false },
		// MAT_ACCELERATION_STRUCTURE
		{ true },
		// MAT_DEVICE_ADDRESS
		{ true },
	};

	struct BlockInfo
	{
		// 本block在page的偏移量
		VkDeviceSize offset;

		// 本block大小
		VkDeviceSize size;
		int isFree;

		BlockInfo* pNext;
		BlockInfo* pPre;
		// 额外信息 用于释放时索引
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

		MemoryAllocateType type;

		uint32_t memoryTypeIndex;
		uint32_t memoryHeapIndex;

		BlockInfo* pHead;

		PageInfo* pPre;
		PageInfo* pNext;
		
		// 额外信息 用于释放时索引
		MemoryHeap* pParent;
		int noShare;

		PageInfo(MemoryHeap* _pParent, VkDevice _vkDevice, VkDeviceSize _size,
			MemoryAllocateType _type, uint32_t _memoryTypeIndex, uint32_t _memoryHeapIndex, int _noShare)
		{
			vkDevice = _vkDevice;
			size = _size;
			type = _type;

			memoryTypeIndex = _memoryTypeIndex;
			memoryHeapIndex = _memoryHeapIndex;

			vkMemroy = VK_NULL_HANDLE;
			pHead = nullptr;
			pPre = pNext = nullptr;	

			pParent = _pParent;
			noShare = _noShare;
		}

		~PageInfo()
		{
			assert(vkMemroy == VK_NULL_HANDLE);
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

		BlockInfo* Alloc(VkDeviceSize sizeToFit, VkDeviceSize alignment)
		{
			if(sizeToFit > size)
			{
				return nullptr;
			}
			if(vkMemroy == VK_NULL_HANDLE)
			{
				assert(pHead == nullptr);

#ifdef KVUALKAN_HEAP_TRUELY_ALLOC
				{
					VkMemoryAllocateInfo allocInfo = {};

					allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					allocInfo.allocationSize = size;
					allocInfo.pNext = nullptr;
					allocInfo.memoryTypeIndex = memoryTypeIndex;

					if (MEMORY_ALLOCATE_TYPE_PROPERITES[type].needDeviceAddress)
					{
						static VkMemoryAllocateFlagsInfo allocFlagsInfo = {};
						// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
						allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
						allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
						allocInfo.pNext = &allocFlagsInfo;
					}

					VK_ASSERT_RESULT(vkAllocateMemory(DEVICE, &allocInfo, nullptr, &vkMemroy));
					HEAP_REMAIN_SIZE[memoryHeapIndex] -= size;
				}
#else
				{
					vkMemroy = malloc((size_t)size);
				}
#endif

				pHead = KNEW BlockInfo(this);
				pHead->isFree = true;
				pHead->offset = 0;
				pHead->pPre = pHead->pNext = nullptr;
				pHead->size = size;
				pHead->pParent = this;

				// 把多余的空间分裂出来
				Split(pHead, 0, sizeToFit);
				// 已被占用
				pHead->isFree = false;

				Check();
				return pHead;
			}
			else
			{
				assert(pHead);

				VkDeviceSize offset = 0;
				VkDeviceSize extraSize = 0;
				BlockInfo* pTemp = Find(sizeToFit, alignment, &offset, &extraSize);
				if(pTemp)
				{
					// 把多余的空间分裂出来
					Split(pTemp, offset, sizeToFit);
					// 已被占用
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
			// 已被释放
			pBlock->isFree = true;
			// 与前后的freeblock合并
			Trim(pBlock);
			// 只剩下最后一个节点 释放内存
			if(pHead->pNext == nullptr)
			{
				SAFE_DELETE(pHead);
				assert(vkMemroy != VK_NULL_HANDLE);
#ifdef KVUALKAN_HEAP_TRUELY_ALLOC
				vkFreeMemory(vkDevice, vkMemroy, nullptr);
				HEAP_REMAIN_SIZE[memoryHeapIndex] += size;
#else
				free(vkMemroy);
#endif
				vkMemroy = VK_NULL_HANDLE;
			}
		}

		// 释放掉freeblock
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
				HEAP_REMAIN_SIZE[memoryHeapIndex] += size;
#else
				free(vkMemroy);
#endif
				vkMemroy = VK_NULL_HANDLE;
			}

			for(BlockInfo* p = pHead; p != nullptr;)
			{
				BlockInfo* pNext = p->pNext;
				SAFE_DELETE(p);
				p = pNext;
			}
			pHead = nullptr;
		}

		BlockInfo* Find(VkDeviceSize sizeToFit, VkDeviceSize alignment, VkDeviceSize* pOffset, VkDeviceSize* pExtraSize)
		{
			BlockInfo* pTemp = pHead;
			while(pTemp)
			{
				if(pTemp->isFree && pTemp->size >= sizeToFit)
				{
					VkDeviceSize offset = (pTemp->offset % size) ? pTemp->offset + alignment - (pTemp->offset % alignment) : pTemp->offset;
					VkDeviceSize extraSize = offset - pTemp->offset + sizeToFit;
					assert(offset % alignment == 0);
					if(extraSize <= pTemp->size)
					{
						if(pOffset)
						{
							*pOffset = offset;
						}
						if(pExtraSize)
						{
							*pExtraSize = extraSize;
						}
						return pTemp;
					}
				}
				pTemp = pTemp->pNext;
			}
			return nullptr;
		}

		bool HasSpace(VkDeviceSize sizeToFit, VkDeviceSize alignment)
		{
			if(pHead)
			{
				return Find(sizeToFit, alignment, nullptr, nullptr) != nullptr;
			}
			else
			{
				return sizeToFit <= size;
			}
		}

		static void Split(BlockInfo* pBlock, VkDeviceSize offset, VkDeviceSize sizeToFit)
		{
			assert(pBlock->isFree && pBlock->size >= sizeToFit);
			if(pBlock->isFree && pBlock->size >= sizeToFit)
			{
				if(offset > 0)
				{
					BlockInfo* pPre = KNEW BlockInfo(pBlock->pParent);

					pPre->pPre = pBlock->pPre;
					if(pPre->pPre)
						pPre->pPre->pNext = pPre;

					pPre->pNext = pBlock;
					pBlock->pPre = pPre;

					if(pBlock == pBlock->pParent->pHead)
					{
						pBlock->pParent->pHead = pPre;
					}

					pPre->isFree = true;
					pPre->offset = pBlock->offset;
					pPre->size = offset - pBlock->offset;

					pBlock->offset += pPre->size;
					pBlock->size -= pPre->size;
				}

				VkDeviceSize remainSize = pBlock->size - sizeToFit;

				BlockInfo* pNext = pBlock->pNext;
				// 如果下一个block可以拿掉剩余空间
				if(pNext && pNext->isFree)
				{
					pNext->offset -= remainSize;
					pNext->size += remainSize;
				}
				// 否则分裂多一个block来记录剩余空间
				else if(remainSize > 0)
				{
					// 把剩余的空间分配到新节点上
					BlockInfo* pNewBlock = KNEW BlockInfo(pBlock->pParent);
					pNewBlock->isFree = true;
					pNewBlock->size = remainSize;
					pNewBlock->offset = pBlock->offset + sizeToFit;

					pNewBlock->pNext = pNext;
					pNewBlock->pPre = pBlock;

					if(pNext)
						pNext->pPre = pNewBlock;
					pBlock->pNext = pNewBlock;
				}

				// 重新分配本block空间
				pBlock->size = sizeToFit;
			}
		}

		static void Trim(BlockInfo* pBlock)
		{
			assert(pBlock->isFree);
			BlockInfo* pTemp = nullptr;
			if(pBlock->isFree)
			{
				// 与后面的freeblock合并
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
				// 与前面的freeblock合并
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
				// 成为头结点
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
		uint32_t memoryHeapIndex;

		PageInfo* pHead[MAT_COUNT];
		PageInfo* pNoShareHead[MAT_COUNT];
		VkDeviceSize lastPageSize[MAT_COUNT];
		VkDeviceSize totalPageSize[MAT_COUNT];

		MemoryHeap(VkDevice _device, uint32_t _memoryTypeIndex, uint32_t _memoryHeapIndex)
		{
			vkDevice = _device;
			memoryTypeIndex = _memoryTypeIndex;
			memoryHeapIndex = _memoryHeapIndex;
			for (uint32_t i = 0; i < MAT_COUNT; ++i)
			{
				pHead[i] = nullptr;
				pNoShareHead[i] = nullptr;
				lastPageSize[i] = 0;
				totalPageSize[i] = 0;
			}
		}

		void Check()
		{
#ifdef KVUALKAN_HEAP_BRUTE_CHECK
			for (uint32_t i = 0; i < MAT_COUNT; ++i)
			{
				if (pHead[i])
				{
					VkDeviceSize sum = 0;
					for (PageInfo* p = pHead[i]; p; p = p->pNext)
					{
						p->Check();
						sum += p->size;
					}
					assert(sum == totalPageSize[i]);
				}
			}
#endif
		}

		void Clear()
		{
			PageInfo* pTemp = nullptr;

			for (uint32_t i = 0; i < MAT_COUNT; ++i)
			{
				lastPageSize[i] = totalPageSize[i] = 0;

				pTemp = pHead[i];
				while (pTemp)
				{
					PageInfo* pNext = pTemp->pNext;
					pTemp->Clear();
					SAFE_DELETE(pTemp);
					pTemp = pNext;
				}

				pTemp = pNoShareHead[i];
				while (pTemp)
				{
					PageInfo* pNext = pTemp->pNext;
					pTemp->Clear();
					SAFE_DELETE(pTemp);
					pTemp = pNext;
				}
			}
		}

		VkDeviceSize NewPageSize(MemoryAllocateType type)
		{
			VkDeviceSize newSize = lastPageSize[type] ? lastPageSize[type] << 1 : MIN_PAGE_SIZE[memoryHeapIndex];
			newSize = std::max(MIN_PAGE_SIZE[memoryHeapIndex], newSize);
			return newSize;
		}

		VkDeviceSize FindPageFitSize(VkDeviceSize pageSize, VkDeviceSize sizeToFit)
		{
			VkDeviceSize newPageSize = KNumerical::Factor2GreaterEqual(sizeToFit);
			newPageSize = std::max(newPageSize, MIN_PAGE_SIZE[memoryHeapIndex]);
			newPageSize = std::min(newPageSize, pageSize);
			return newPageSize;
		}

		BlockInfo* Alloc(VkDeviceSize sizeToFit, VkDeviceSize alignment, bool noShared, MemoryAllocateType type)
		{
			std::lock_guard<decltype(ALLOC_FREE_LOCK)> guard(ALLOC_FREE_LOCK);

			if(noShared)
			{
				PageInfo* pPage = KNEW PageInfo(this, vkDevice, sizeToFit, type, memoryTypeIndex, memoryHeapIndex, true);
				BlockInfo* pBlock = pPage->Alloc(sizeToFit, alignment);

				pPage->pNext = pNoShareHead[type];
				if(pNoShareHead[type])
				{
					pNoShareHead[type]->pPre = pPage;
				}
				pNoShareHead[type] = pPage;

				assert(pBlock && !pBlock->isFree);
				return pBlock;
			}
			else
			{
				VkDeviceSize allocFactor = ALLOC_FACTOR;

				// 分配大小必须是allocFactor的整数倍
				// 当每次分配都是allocFactor的整数倍时候 就能保证同一个page里的offset也是allocFactor的整数倍
				// sizeToFit = ((sizeToFit + ALLOC_FACTOR - 1) / ALLOC_FACTOR) * ALLOC_FACTOR;
				// assert(sizeToFit % ALLOC_FACTOR == 0);

				alignment = KNumerical::LCM(alignment, ALLOC_FACTOR);

				PageInfo* pPage = Find(sizeToFit, alignment, type);
				if(!pPage)
				{
					// 当前所有page里找不到足够空间 分配一个新的插入到最后 保证heap总空间2倍递增
					while (true)
					{
						pPage = Nail(type);
						VkDeviceSize newSize = NewPageSize(type);

						lastPageSize[type] = newSize;
						totalPageSize[type] += newSize;

						PageInfo* pNewPage = KNEW PageInfo(this, vkDevice, newSize, type, memoryTypeIndex, memoryHeapIndex, false);

						if(pPage)
							pPage->pNext = pNewPage;
						pNewPage->pPre = pPage;
						pNewPage->pNext = nullptr;

						// 头结点
						if(pHead[type] == nullptr)
						{
							pHead[type] = pNewPage;
						}

						if(pNewPage->size >= sizeToFit)
						{
							pPage = pNewPage;
							break;
						}
					}
				}

				// 把多余的空间分裂出来 尽量节省实际分配的内存
				if(pPage->vkMemroy == VK_NULL_HANDLE)
				{
					VkDeviceSize newPageSize = FindPageFitSize(pPage->size, sizeToFit);
					Split(pPage, newPageSize);
				}
				BlockInfo* pBlock = pPage->Alloc(sizeToFit, alignment);
				assert(pBlock && !pBlock->isFree);

				Check();
				return pBlock;
			}
		}

		void Free(BlockInfo* pBlock)
		{
			std::lock_guard<decltype(ALLOC_FREE_LOCK)> guard(ALLOC_FREE_LOCK);

			assert(pBlock->pParent != nullptr);
			PageInfo* pPage = pBlock->pParent;
			assert(pPage->pParent == this);

			MemoryAllocateType type = pPage->type;
			// 特殊情况只能独占一个vkAllocateMemory特殊处理 这里连同page同时删除
			if(pPage->noShare)
			{
				if(pPage == pNoShareHead[type])
				{
					pNoShareHead[type] = pPage->pNext;
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
				// 空间为空 尝试合并临近page
				if(pPage->vkMemroy == VK_NULL_HANDLE)
				{
					Trim(pPage);
				}
				Check();
			}
		}

		PageInfo* Find(VkDeviceSize sizeToFit, VkDeviceSize alignment, MemoryAllocateType type)
		{
			PageInfo* pTemp = pHead[type];
			while(pTemp)
			{
				if(pTemp->size >= sizeToFit)
				{
					if(pTemp->HasSpace(sizeToFit, alignment))
					{
						return pTemp;
					}
				}
				pTemp = pTemp->pNext;
			}
			return nullptr;
		}

		PageInfo* Nail(MemoryAllocateType type)
		{
			PageInfo* pTemp = pHead[type];
			while(pTemp && pTemp->pNext)
			{
				pTemp = pTemp->pNext;
			}
			return pTemp;
		}

		static void Trim(PageInfo* pPage)
		{
			assert(pPage->vkMemroy == VK_NULL_HANDLE);
			PageInfo* pTemp = nullptr;
			if(pPage->vkMemroy == VK_NULL_HANDLE)
			{
				// 与后面的freepage合并
				while(pPage->pNext && pPage->pNext->vkMemroy == VK_NULL_HANDLE)
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
				// 与前面的freepage合并
				while(pPage->pPre && pPage->pPre->vkMemroy == VK_NULL_HANDLE)
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
				// 成为头结点
				if(pPage->pPre == nullptr)
				{
					assert(pPage->pParent);
					MemoryAllocateType type = pPage->type;
					pPage->pParent->pHead[type] = pPage;
				}
			}
		}

		static void Split(PageInfo* pPage, VkDeviceSize sizeToFit)
		{
			assert(pPage->vkMemroy == VK_NULL_HANDLE && pPage->size >= sizeToFit);
			if(pPage->vkMemroy == VK_NULL_HANDLE && pPage->size >= sizeToFit)
			{
				// page的新大小不是ALLOC_FACTOR的整数倍 无法分裂
				if(sizeToFit % ALLOC_FACTOR != 0)
				{
					return;
				}

				VkDeviceSize remainSize = pPage->size - sizeToFit;

				PageInfo* pNext = pPage->pNext;
				// 如果下一个page可以拿掉剩余空间
				if(pNext && pNext->vkMemroy == VK_NULL_HANDLE)
				{
					pNext->size += remainSize;
				}
				// 否则分裂多一个page来记录剩余空间
				else if(remainSize > 0)
				{
					// 把剩余的空间分配到新节点上
					PageInfo* pNewPage = KNEW PageInfo(pPage->pParent, pPage->vkDevice, remainSize, pPage->type, pPage->memoryTypeIndex, pPage->memoryHeapIndex, false);
					pNewPage->vkMemroy = VK_NULL_HANDLE;
					pNewPage->size = remainSize;

					pNewPage->pNext = pNext;
					pNewPage->pPre = pPage;

					if(pNext)
						pNext->pPre = pNewPage;
					pPage->pNext = pNewPage;
				}

				// 重新分配本page空间
				pPage->size = sizeToFit;
			}
		}
	};

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
			// 由于这里image与buffer共享同一份VkDeviceMemory 因此每次分配占用的空间大小必须是该factor的整数倍
			ALLOC_FACTOR = deviceProperties.limits.bufferImageGranularity;

			VkPhysicalDeviceMemoryProperties memoryProperties = {};
			vkGetPhysicalDeviceMemoryProperties(PHYSICAL_DEVICE, &memoryProperties);

			MEMORY_TYPE_COUNT = memoryProperties.memoryTypeCount;
			MEMORY_TYPE_TO_HEAP.resize(memoryProperties.memoryTypeCount);
			for(uint32_t memTypeIdx = 0; memTypeIdx < memoryProperties.memoryTypeCount; ++memTypeIdx)
			{
				uint32_t memHeapIndex = memoryProperties.memoryTypes[memTypeIdx].heapIndex;
				MEMORY_TYPE_TO_HEAP[memTypeIdx] = KNEW MemoryHeap(KVulkanGlobal::device, memTypeIdx, memHeapIndex);
			}

			HEAP_REMAIN_SIZE.resize(memoryProperties.memoryHeapCount);
			MIN_PAGE_SIZE.resize(memoryProperties.memoryHeapCount);

			for(uint32_t memHeapIdx = 0; memHeapIdx < memoryProperties.memoryHeapCount; ++memHeapIdx)
			{
				HEAP_REMAIN_SIZE[memHeapIdx] = memoryProperties.memoryHeaps[memHeapIdx].size;
				MIN_PAGE_SIZE[memHeapIdx] = std::min
				(
					HEAP_REMAIN_SIZE[memHeapIdx],
					BLOCK_SIZE_FACTOR * KNumerical::Pow2GreaterEqual(memoryProperties.memoryHeaps[memHeapIdx].size * memoryProperties.memoryHeapCount / MAX_ALLOC_COUNT)
				);
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
			MEMORY_TYPE_TO_HEAP[0] = KNEW MemoryHeap(KVulkanGlobal::device, 0, 0);
			HEAP_REMAIN_SIZE.resize(1);
			HEAP_REMAIN_SIZE[0] = static_cast<VkDeviceSize>(512U * 1024U * 1024U);
			MIN_PAGE_SIZE.resize(1);
			MIN_PAGE_SIZE[0] = static_cast<VkDeviceSize>(1);
			MAX_PAGE_SIZE.resize(1);
			MAX_PAGE_SIZE[0] = static_cast<VkDeviceSize>(512U * 1024U * 1024U);
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
		HEAP_REMAIN_SIZE.clear();
		MIN_PAGE_SIZE.clear();

		return true;
	}

	bool Alloc(VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryTypeIndex, VkMemoryPropertyFlags memoryUsage, VkBufferUsageFlags bufferUsage, bool noShared, AllocInfo& info)
	{
		if(memoryTypeIndex < MEMORY_TYPE_COUNT)
		{
			MemoryHeap* pHeap = MEMORY_TYPE_TO_HEAP[memoryTypeIndex];

			MemoryAllocateType type = MAT_DEFAULT;

			// TODO
			if (memoryUsage & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			{
				noShared = true;
			}

			if (bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
			{
				type = MAT_DEVICE_ADDRESS;
			}

			// 光追加速结构不能够与其他资源共享VkDeviceMemory
			if (bufferUsage & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)
			{
				type = MAT_ACCELERATION_STRUCTURE;
			}

			info.internalData = pHeap->Alloc(size, alignment, noShared, type);

			if(info.internalData)
			{
				BlockInfo* pBlock = (BlockInfo*)info.internalData;
				PageInfo* pPage = (PageInfo*)pBlock->pParent;

				info.vkMemroy = static_cast<VkDeviceMemory>(pPage->vkMemroy);
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