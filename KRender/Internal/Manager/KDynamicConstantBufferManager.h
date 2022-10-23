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
		size_t useSize;

		ConstantBlock()
		{
			buffer = nullptr;
			useSize = 0;
		}
	};

	struct FrameConstantBlock
	{
		size_t frameNum;
		std::vector<ConstantBlock> blocks;

		FrameConstantBlock()
		{
			frameNum = 0;
		}
	};

	std::vector<FrameConstantBlock> m_ConstantBlocks;
	std::mutex m_Lock;
	IKRenderDevice* m_Device;
	size_t m_BlockSize;
	size_t m_Alignment;

	bool InternalAlloc(size_t size,
		size_t frameIndex, size_t frameNum,
		IKUniformBufferPtr& buffer, size_t& offset);
public:
	KDynamicConstantBufferManager();
	~KDynamicConstantBufferManager();

	bool Init(IKRenderDevice* device, size_t aligment, size_t blockSize);
	bool UnInit();

	bool Alloc(const void* data, KDynamicConstantBufferUsage& usage);
};
