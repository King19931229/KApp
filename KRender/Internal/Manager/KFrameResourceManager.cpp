#include "KFrameResourceManager.h"
#include "Internal/KConstantGlobal.h"

KFrameResourceManager::KFrameResourceManager()
	: m_Device(nullptr),
	m_FrameInFlight(0),
	m_RenderThreadNum(0)
{

}

KFrameResourceManager::~KFrameResourceManager()
{

}

bool KFrameResourceManager::Init(IKRenderDevice* device, size_t frameInFlight, size_t renderThreadNum)
{
	ASSERT_RESULT(UnInit());

	m_Device = device;
	m_FrameInFlight = frameInFlight;
	m_RenderThreadNum = renderThreadNum;

	for(size_t cbtIdx = 0; cbtIdx < CBT_COUNT; ++cbtIdx)
	{
		ConstantBufferType bufferType = (ConstantBufferType)cbtIdx;

		FrameBufferDataList& frameBuffers = m_FrameContantBuffer[cbtIdx];
		frameBuffers.resize(frameInFlight);

		for(FrameBufferData& frameBufferData : frameBuffers)
		{
			// todo hard code for now
			if(cbtIdx == CBT_OBJECT)
			{
				frameBufferData.bufferPreThread = true;
				frameBufferData.buffer = nullptr;
				frameBufferData.threadBuffers.resize(renderThreadNum);

				for(IKUniformBufferPtr& buffer : frameBufferData.threadBuffers)
				{
					ASSERT_RESULT(device->CreateUniformBuffer(buffer));

					auto& detail = KConstantDefinition::GetConstantBufferDetail(bufferType);
					void* initData = KConstantGlobal::GetGlobalConstantData(bufferType);

					ASSERT_RESULT(buffer->InitMemory(detail.bufferSize, initData));
					ASSERT_RESULT(buffer->InitDevice());
				}
			}
			else
			{
				frameBufferData.bufferPreThread = false;
				IKUniformBufferPtr& buffer = frameBufferData.buffer;

				ASSERT_RESULT(device->CreateUniformBuffer(buffer));

				auto& detail = KConstantDefinition::GetConstantBufferDetail(bufferType);
				void* initData = KConstantGlobal::GetGlobalConstantData(bufferType);

				ASSERT_RESULT(buffer->InitMemory(detail.bufferSize, initData));
				ASSERT_RESULT(buffer->InitDevice());
			}
		}
	}

	return true;
}

bool KFrameResourceManager::UnInit()
{
	for(size_t cbtIdx = 0; cbtIdx < CBT_COUNT; ++cbtIdx)
	{
		FrameBufferDataList& frameBuffers = m_FrameContantBuffer[cbtIdx];
		for(FrameBufferData& frameBufferData : frameBuffers)
		{
			for(IKUniformBufferPtr& buffer : frameBufferData.threadBuffers)
			{
				if(buffer)
				{
					buffer->UnInit();
					buffer = nullptr;
				}
			}
			frameBufferData.threadBuffers.clear();

			if(frameBufferData.buffer)
			{
				frameBufferData.buffer->UnInit();
				frameBufferData.buffer = nullptr;
			}
		}

		frameBuffers.clear();
	}

	m_Device = nullptr;
	m_FrameInFlight = 0;
	m_RenderThreadNum = 0;

	return true;
}

IKUniformBufferPtr KFrameResourceManager::GetConstantBuffer(size_t frameIndex, size_t threadIndex, ConstantBufferType type)
{
	if(frameIndex > m_FrameInFlight)
	{
		assert(false && "frame index out of bound");
		return nullptr;
	}

	if(threadIndex > m_RenderThreadNum)
	{
		assert(false && "thraed index out of bound");
		return nullptr;
	}

	if(type >= CBT_COUNT)
	{
		assert(false && "constant type out of bound");
		return nullptr;
	}

	FrameBufferDataList& frameBuffers = m_FrameContantBuffer[type];

	assert(frameIndex < m_FrameContantBuffer[type].size());	
	FrameBufferData& frameBufferData = frameBuffers[frameIndex];

	if(frameBufferData.bufferPreThread)
	{
		assert(threadIndex < frameBufferData.threadBuffers.size());	
		return frameBufferData.threadBuffers[threadIndex];
	}
	else
	{
		assert(frameBufferData.buffer);
		return frameBufferData.buffer;
	}
}