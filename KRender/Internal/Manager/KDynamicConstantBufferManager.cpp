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

bool KDynamicConstantBufferManager::Init(uint64_t alignment, uint32_t blockSize)
{
	UnInit();

	m_Alignment = alignment;
	m_BlockSize = blockSize;

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

void KDynamicConstantBufferManager::Release(uint32_t frameNum)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	for (FrameConstantBlock& frameBlock : m_ConstantBlocks)
	{
		if (!frameBlock.occupied || frameBlock.frameNum != frameNum) continue;
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
		frameBlock.occupied = false;
	}
}

bool KDynamicConstantBufferManager::Alloc(const void* data, KDynamicConstantBufferUsage& usage)
{
	if (usage.range > m_BlockSize) return false;
	uint64_t alignment = (usage.range + m_Alignment - 1) & ~(m_Alignment - 1);
	if (alignment > m_BlockSize) return false;
	if (InternalAlloc(static_cast<uint32_t>(alignment), usage.buffer, usage.offset))
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		ASSERT_RESULT(data);
		void* pBufferData = nullptr;
		ASSERT_RESULT(usage.buffer->Map(&pBufferData));
		memcpy(POINTER_OFFSET(pBufferData, usage.offset), data, static_cast<size_t>(usage.range));
		ASSERT_RESULT(usage.buffer->UnMap());
		return true;
	}
	return false;
}

bool KDynamicConstantBufferManager::InternalAlloc(uint32_t size,
	IKUniformBufferPtr& buffer, uint32_t& offset)
{
	ASSERT_RESULT(size <= m_BlockSize);

	if (size <= m_BlockSize)
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

		const uint32_t frameNum = KRenderGlobal::CurrentFrameNum;
		FrameConstantBlock* selected = nullptr;
		FrameConstantBlock* available = nullptr;
		for (FrameConstantBlock& group : m_ConstantBlocks)
		{
			if (group.occupied && group.frameNum == frameNum)
			{
				selected = &group;
				break;
			}
			if (!group.occupied && !available) available = &group;
		}
		if (!selected)
		{
			if (!available)
			{
				m_ConstantBlocks.emplace_back();
				available = &m_ConstantBlocks.back();
			}
			selected = available;
			selected->frameNum = frameNum;
			selected->occupied = true;
		}
		FrameConstantBlock& frameBlock = *selected;

		for (ConstantBlock& block : frameBlock.blocks)
		{
			if (size <= m_BlockSize - block.useSize)
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