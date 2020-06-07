#include "KDynamicConstantBufferManager.h"

KDynamicConstantBufferManager::KDynamicConstantBufferManager()
	: m_Device(nullptr),
	m_BlockSize(512 * 1024)
{
}

KDynamicConstantBufferManager::~KDynamicConstantBufferManager()
{
	ASSERT_RESULT(m_ConstantBlocks.empty());
}

bool KDynamicConstantBufferManager::Init(IKRenderDevice* device, size_t frameInFlight, size_t blockSize)
{
	UnInit();

	m_Device = device;
	m_ConstantBlocks.resize(frameInFlight);
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
	m_Device = nullptr;

	return true;
}

bool KDynamicConstantBufferManager::Alloc(size_t size,
	size_t frameIndex, size_t frameNum,
	IKUniformBufferPtr& buffer, size_t& offset)
{
	ASSERT_RESULT(m_Device);
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
		ASSERT_RESULT(m_Device->CreateUniformBuffer(newBlock.buffer));
		newBlock.useSize = 0;

		buffer = newBlock.buffer;
		offset = 0;

		newBlock.useSize += size;

		frameBlock.blocks.push_back(newBlock);
		return true;
	}
	return false;
}