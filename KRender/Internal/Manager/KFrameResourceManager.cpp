#include "KFrameResourceManager.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

KFrameResourceManager::KFrameResourceManager()
	: m_Device(nullptr)
{
}

KFrameResourceManager::~KFrameResourceManager()
{
}

bool KFrameResourceManager::Init()
{
	ASSERT_RESULT(UnInit());

	for (size_t cbtIdx = CBT_STATIC_BEGIN; cbtIdx <= CBT_STATIC_END; ++cbtIdx)
	{
		ConstantBufferType bufferType = (ConstantBufferType)cbtIdx;
		IKUniformBufferPtr& buffer = m_ContantBuffers[cbtIdx];
		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateUniformBuffer(buffer));
		auto& detail = KConstantDefinition::GetConstantBufferDetail(bufferType);
		void* initData = KConstantGlobal::GetGlobalConstantData(bufferType);
		ASSERT_RESULT(buffer->InitMemory(detail.bufferSize, initData));
		ASSERT_RESULT(buffer->InitDevice());
	}

	return true;
}

bool KFrameResourceManager::UnInit()
{
	for (size_t cbtIdx = CBT_STATIC_BEGIN; cbtIdx <= CBT_STATIC_END; ++cbtIdx)
	{
		ConstantBufferType bufferType = (ConstantBufferType)cbtIdx;
		IKUniformBufferPtr& buffer = m_ContantBuffers[cbtIdx];
		if (buffer)
		{
			buffer->UnInit();
			buffer = nullptr;
		}
	}

	return true;
}

IKUniformBufferPtr KFrameResourceManager::GetConstantBuffer(ConstantBufferType type)
{
	if (type >= CBT_COUNT)
	{
		assert(false && "constant type out of bound");
		return nullptr;
	}

	return m_ContantBuffers[type];
}