#include "KInstanceBufferManager.h"
#include "Internal/KRenderGlobal.h"

KInstanceBufferManager::KInstanceBufferManager()
	: m_Device(nullptr),
	m_VertexSize(64),
	m_BlockCount(65536)
{
}

KInstanceBufferManager::~KInstanceBufferManager()
{
	ASSERT_RESULT(m_Device == nullptr);
	ASSERT_RESULT(m_InstanceBlocks.empty());
}

bool KInstanceBufferManager::Init(IKRenderDevice* device, size_t vertexSize, size_t blockSize)
{
	UnInit();

	ASSERT_RESULT(device);

	m_Device = device;
	m_VertexSize = vertexSize;
	m_BlockCount = blockSize;

	m_InstanceBlocks.resize(KRenderGlobal::NumFramesInFlight);
	return true;
}

bool KInstanceBufferManager::UnInit()
{
	for (FrameInstanceBlock& frameBlock : m_InstanceBlocks)
	{
		for (InstanceBlock& block : frameBlock.blocks)
		{
			SAFE_UNINIT(block.buffer);
		}
		frameBlock.blocks.clear();
	}
	m_InstanceBlocks.clear();
	m_Device = nullptr;
	return true;
}

bool KInstanceBufferManager::Alloc(size_t count, const void* data, std::vector<AllocResultBlock>& results)
{
	results.clear();
	if (InternalAlloc(count, KRenderGlobal::CurrentInFlightFrameIndex, KRenderGlobal::CurrentFrameNum, results))
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		ASSERT_RESULT(data);

		size_t assignedCount = 0;
		for (AllocResultBlock& result : results)
		{
			void* pBufferData = nullptr;
			ASSERT_RESULT(result.buffer->Map(&pBufferData));
			memcpy(POINTER_OFFSET(pBufferData, result.start * m_VertexSize), POINTER_OFFSET(data, assignedCount * m_VertexSize), result.count * m_VertexSize);
			ASSERT_RESULT(result.buffer->UnMap());
			assignedCount += result.count;
		}
		ASSERT_RESULT(assignedCount == count);

		return true;
	}
	return false;
}

bool KInstanceBufferManager::InternalAlloc(size_t count,
	size_t frameIndex, size_t frameNum,
	std::vector<AllocResultBlock>& results)
{
	if (frameIndex < m_InstanceBlocks.size())
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

		FrameInstanceBlock& frameBlock = m_InstanceBlocks[frameIndex];

		if (frameBlock.frameNum != frameNum)
		{
			for (auto it = frameBlock.blocks.begin(); it != frameBlock.blocks.end();)
			{
				if (it->useCount == 0 && it != frameBlock.blocks.begin())
				{
					SAFE_UNINIT(it->buffer);
					it = frameBlock.blocks.erase(it);
					continue;
				}
				it->useCount = 0;
				++it;
			}
			frameBlock.frameNum = frameNum;
		}

		size_t restCount = count;

		{
			for (InstanceBlock& block : frameBlock.blocks)
			{
				size_t blockRestCount = m_BlockCount - block.useCount;
				if (blockRestCount > 0)
				{
					size_t useCount = restCount > blockRestCount ? blockRestCount : restCount;
					restCount -= useCount;

					AllocResultBlock resultBlock;
					resultBlock.buffer = block.buffer;
					resultBlock.count = useCount;
					resultBlock.start = block.useCount;
					resultBlock.offset = block.useCount * m_VertexSize;
					results.push_back(resultBlock);

					block.useCount += useCount;
				}
			}

			while (restCount)
			{
				InstanceBlock newBlock;

				ASSERT_RESULT(m_Device->CreateVertexBuffer(newBlock.buffer));
				newBlock.useCount = 0;

				ASSERT_RESULT(newBlock.buffer->InitMemory(m_BlockCount, m_VertexSize, nullptr));
				ASSERT_RESULT(newBlock.buffer->InitDevice(true));

				{
					size_t useCount = restCount > m_BlockCount ? m_BlockCount : restCount;
					restCount -= useCount;

					AllocResultBlock resultBlock;
					resultBlock.buffer = newBlock.buffer;
					resultBlock.count = useCount;
					resultBlock.start = newBlock.useCount;
					resultBlock.offset = newBlock.useCount * m_VertexSize;
					results.push_back(resultBlock);

					newBlock.useCount += useCount;
				}

				frameBlock.blocks.push_back(newBlock);
			}
		}

		return true;
	}
	return false;
}