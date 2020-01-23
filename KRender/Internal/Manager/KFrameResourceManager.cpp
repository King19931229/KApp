#include "KFrameResourceManager.h"
#include "Internal/KConstantGlobal.h"

KFrameResourceManager::KFrameResourceManager()
	: m_Device(nullptr),
	m_FrameInFlight(0)
{

}

KFrameResourceManager::~KFrameResourceManager()
{

}

bool KFrameResourceManager::Init(IKRenderDevice* device, size_t frameInFlight)
{
	ASSERT_RESULT(UnInit());

	m_Device = device;
	m_FrameInFlight = frameInFlight;

	for (size_t cbtIdx = 0; cbtIdx < CBT_COUNT; ++cbtIdx)
	{
		ConstantBufferType bufferType = (ConstantBufferType)cbtIdx;
		FrameBufferList& frameBuffers = m_FrameContantBuffer[cbtIdx];
		frameBuffers.resize(frameInFlight);
		for (IKUniformBufferPtr& buffer : frameBuffers)
		{
			ASSERT_RESULT(device->CreateUniformBuffer(buffer));
			auto& detail = KConstantDefinition::GetConstantBufferDetail(bufferType);
			void* initData = KConstantGlobal::GetGlobalConstantData(bufferType);
			ASSERT_RESULT(buffer->InitMemory(detail.bufferSize, initData));
			ASSERT_RESULT(buffer->InitDevice());
		}
	}

	return true;
}

bool KFrameResourceManager::UnInit()
{
	for (size_t cbtIdx = 0; cbtIdx < CBT_COUNT; ++cbtIdx)
	{
		ConstantBufferType bufferType = (ConstantBufferType)cbtIdx;
		FrameBufferList& frameBuffers = m_FrameContantBuffer[cbtIdx];
		for (IKUniformBufferPtr& buffer : frameBuffers)
		{
			if (buffer)
			{
				buffer->UnInit();
				buffer = nullptr;
			}
		}
		frameBuffers.clear();
	}

	m_Device = nullptr;
	m_FrameInFlight = 0;

	return true;
}

IKUniformBufferPtr KFrameResourceManager::GetConstantBuffer(size_t frameIndex, ConstantBufferType type)
{
	if(frameIndex > m_FrameInFlight)
	{
		assert(false && "frame index out of bound");
		return nullptr;
	}

	if(type >= CBT_COUNT)
	{
		assert(false && "constant type out of bound");
		return nullptr;
	}

	FrameBufferList& frameBuffers = m_FrameContantBuffer[type];
	assert(frameIndex < frameBuffers.size());

	return frameBuffers[frameIndex];
}