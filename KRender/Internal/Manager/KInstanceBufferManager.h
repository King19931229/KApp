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
		size_t start;
		size_t count;
		size_t offset;
	};
protected:
	struct InstanceBlock
	{
		IKVertexBufferPtr buffer;
		size_t useCount;

		InstanceBlock()
		{
			buffer = nullptr;
			useCount = 0;
		}
	};

	struct FrameInstanceBlock
	{
		size_t frameNum;
		std::vector<InstanceBlock> blocks;
		FrameInstanceBlock()
		{
			frameNum = 0;
		}
	};

	std::vector<FrameInstanceBlock> m_InstanceBlocks;
	std::mutex m_Lock;
	IKRenderDevice* m_Device;
	size_t m_VertexSize;
	size_t m_BlockCount;

	bool InternalAlloc(size_t count,
		size_t frameIndex, size_t frameNum,
		std::vector<AllocResultBlock>& results);
public:
	KInstanceBufferManager();
	~KInstanceBufferManager();

	bool Init(IKRenderDevice* device, size_t vertexSize, size_t blockSize);
	bool UnInit();

	bool Alloc(size_t count, const void* data, std::vector<AllocResultBlock>& results);

	inline size_t GetVertexSize() const { return m_VertexSize; }
};