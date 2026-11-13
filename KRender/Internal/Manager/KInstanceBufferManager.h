#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKBuffer.h"
#include <mutex>

class KInstanceBufferManager
{
public:
	struct AllocResultBlock
	{
		IKVertexBufferPtr buffer;
		uint32_t start;
		uint32_t count;
		uint64_t offset;
	};
protected:
	struct InstanceBlock
	{
		IKVertexBufferPtr buffer;
		uint32_t useCount;

		InstanceBlock()
		{
			buffer = nullptr;
			useCount = 0;
		}
	};

	struct FrameInstanceBlock
	{
		uint32_t frameNum;
		std::vector<InstanceBlock> blocks;
		FrameInstanceBlock()
		{
			frameNum = 0;
		}
	};

	std::vector<FrameInstanceBlock> m_InstanceBlocks;
	std::mutex m_Lock;
	uint32_t m_VertexSize;
	uint32_t m_BlockCount;

	bool InternalAlloc(uint32_t count,
		uint32_t frameIndex, uint32_t frameNum,
		std::vector<AllocResultBlock>& results);
public:
	KInstanceBufferManager();
	~KInstanceBufferManager();

	bool Init(uint32_t vertexSize, uint32_t blockSize);
	bool UnInit();

	bool Alloc(uint32_t count, const void* data, std::vector<AllocResultBlock>& results);

	inline uint32_t GetVertexSize() const { return m_VertexSize; }
};