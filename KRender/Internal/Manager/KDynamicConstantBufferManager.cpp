#include "KDynamicConstantBufferManager.h"
#include "Internal/KRenderGlobal.h"

KDynamicConstantBufferManager::KDynamicConstantBufferManager()
	: m_BlockSize(65536)
	, m_Alignment(256)
{
}

KDynamicConstantBufferManager::~KDynamicConstantBufferManager()
{
	ASSERT_RESULT(m_ConstantBlocks.empty());
}

bool KDynamicConstantBufferManager::Init(size_t alignment, size_t blockSize)
{
	UnInit();

	m_ConstantBlocks.resize(KRenderGlobal::NumFramesInFlight);
	m_Alignment = alignment;
	m_BlockSize = blockSize;

	for (FrameConstantBlock& frameBlock : m_ConstantBlocks)
	{
		frameBlock.frameNum = 0;
	}

	return true;
}

bool KDynamicConstantBufferManager::UnInit()
{
	for (FrameConstantBlock& frameBlock : m_ConstantBlocks)
	{
		for (ConstantBlock& block : frameBlock.blocks)
		{
			SAFE_UNINIT(block.buffer);
		}
		frameBlock.blocks.clear();
	}
	m_ConstantBlocks.clear();

	return true;
}

bool KDynamicConstantBufferManager::Alloc(const void* data, KDynamicConstantBufferUsage& usage)
{
	size_t alignment = (usage.range + m_Alignment - 1) & ~(m_Alignment - 1);
	if (InternalAlloc(alignment, KRenderGlobal::CurrentInFlightFrameIndex, KRenderGlobal::CurrentFrameNum, usage.buffer, usage.offset))
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		ASSERT_RESULT(data);
		void* pBufferData = nullptr;
		ASSERT_RESULT(usage.buffer->Map(&pBufferData));
		memcpy(POINTER_OFFSET(pBufferData, usage.offset), data, usage.range);
		ASSERT_RESULT(usage.buffer->UnMap());
		return true;
	}
	return false;
}

bool KDynamicConstantBufferManager::InternalAlloc(size_t size,
	size_t frameIndex, size_t frameNum,
	IKUniformBufferPtr& buffer, size_t& offset)
{
	ASSERT_RESULT(size <= m_BlockSize);

	if (size <= m_BlockSize && frameIndex < m_ConstantBlocks.size())
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

		FrameConstantBlock& frameBlock = m_ConstantBlocks[frameIndex];

		if (frameBlock.frameNum != frameNum)
		{
			for (auto it = frameBlock.blocks.begin(); it != frameBlock.blocks.end();)
			{
				if (it->useSize == 0 && it != frameBlock.blocks.begin())
				{
					SAFE_UNINIT(it->buffer);
					it = frameBlock.blocks.erase(it);
					continue;
				}
				it->useSize = 0;
				++it;
			}
			frameBlock.frameNum = frameNum;
		}

		for (ConstantBlock& block : frameBlock.blocks)
		{
			if (block.useSize + size <= m_BlockSize)
			{
				buffer = block.buffer;
				offset = block.useSize;
				block.useSize += size;
				return true;
			}
		}

		ConstantBlock newBlock;
		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateUniformBuffer(newBlock.buffer));
		newBlock.useSize = 0;

		buffer = newBlock.buffer;
		ASSERT_RESULT(buffer->InitMemory(m_BlockSize, nullptr));
		ASSERT_RESULT(buffer->InitDevice());
		offset = 0;

		newBlock.useSize += size;

		frameBlock.blocks.push_back(newBlock);
		return true;
	}
	return false;
}