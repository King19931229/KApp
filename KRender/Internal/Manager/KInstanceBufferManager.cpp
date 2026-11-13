#include "KInstanceBufferManager.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KInteger.h"

KInstanceBufferManager::KInstanceBufferManager()
	: m_VertexSize(64)
	, m_BlockCount(65536)
{
}

KInstanceBufferManager::~KInstanceBufferManager()
{
	ASSERT_RESULT(m_InstanceBlocks.empty());
}

bool KInstanceBufferManager::Init(uint32_t vertexSize, uint32_t blockSize)
{
	if (!KInteger::FitsMemorySize(uint64_t(vertexSize) * blockSize)) return false;
	UnInit();

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
	return true;
}

bool KInstanceBufferManager::Alloc(uint32_t count, const void* data, std::vector<AllocResultBlock>& results)
{
	results.clear();
	if (!KInteger::FitsMemorySize(uint64_t(count) * m_VertexSize)) return false;
	if (InternalAlloc(count, KRenderGlobal::CurrentInFlightFrameIndex, KRenderGlobal::CurrentFrameNum, results))
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		ASSERT_RESULT(data);

		uint32_t assignedCount = 0;
		for (AllocResultBlock& result : results)
		{
			void* pBufferData = nullptr;
			ASSERT_RESULT(result.buffer->Map(&pBufferData));
			memcpy(POINTER_OFFSET(pBufferData, uint64_t(result.start) * m_VertexSize), POINTER_OFFSET(data, uint64_t(assignedCount) * m_VertexSize), static_cast<size_t>(uint64_t(result.count) * m_VertexSize));
			ASSERT_RESULT(result.buffer->UnMap());
			assignedCount += result.count;
		}
		ASSERT_RESULT(assignedCount == count);

		return true;
	}
	return false;
}

bool KInstanceBufferManager::InternalAlloc(uint32_t count,
	uint32_t frameIndex, uint32_t frameNum,
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

		uint32_t restCount = count;

		{
			for (InstanceBlock& block : frameBlock.blocks)
			{
				uint32_t blockRestCount = m_BlockCount - block.useCount;
				if (blockRestCount > 0)
				{
					uint32_t useCount = restCount > blockRestCount ? blockRestCount : restCount;
					restCount -= useCount;

					AllocResultBlock resultBlock;
					resultBlock.buffer = block.buffer;
					resultBlock.count = useCount;
					resultBlock.start = block.useCount;
					resultBlock.offset = uint64_t(block.useCount) * m_VertexSize;
					results.push_back(resultBlock);

					block.useCount += useCount;
				}
			}

			while (restCount)
			{
				InstanceBlock newBlock;

				ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateVertexBuffer(newBlock.buffer));
				newBlock.useCount = 0;

				ASSERT_RESULT(newBlock.buffer->InitMemory(m_BlockCount, m_VertexSize, nullptr));
				ASSERT_RESULT(newBlock.buffer->InitDevice(true));

				{
					uint32_t useCount = restCount > m_BlockCount ? m_BlockCount : restCount;
					restCount -= useCount;

					AllocResultBlock resultBlock;
					resultBlock.buffer = newBlock.buffer;
					resultBlock.count = useCount;
					resultBlock.start = newBlock.useCount;
					resultBlock.offset = uint64_t(newBlock.useCount) * m_VertexSize;
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