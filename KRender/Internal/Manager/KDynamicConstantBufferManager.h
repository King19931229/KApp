#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKBuffer.h"
#include <mutex>

class KDynamicConstantBufferManager
{
protected:
	struct ConstantBlock
	{
		IKUniformBufferPtr buffer;
		uint32_t useSize;

		ConstantBlock()
		{
			buffer = nullptr;
			useSize = 0;
		}
	};

	struct FrameConstantBlock
	{
		uint32_t frameNum;
		bool occupied;
		std::vector<ConstantBlock> blocks;

		FrameConstantBlock()
		{
			frameNum = 0;
			occupied = false;
		}
	};

	std::vector<FrameConstantBlock> m_ConstantBlocks;
	std::mutex m_Lock;
	uint32_t m_BlockSize;
	uint64_t m_Alignment;

	bool InternalAlloc(uint32_t size,
		IKUniformBufferPtr& buffer, uint32_t& offset);
public:
	KDynamicConstantBufferManager();
	~KDynamicConstantBufferManager();

	bool Init(uint64_t aligment, uint32_t blockSize);
	bool UnInit();
	void Release(uint32_t frameNum);

	bool Alloc(const void* data, KDynamicConstantBufferUsage& usage);
};
