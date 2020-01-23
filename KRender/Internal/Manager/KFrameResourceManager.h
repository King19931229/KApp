#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/KConstantDefinition.h"

class KFrameResourceManager
{
protected:	
	IKRenderDevice* m_Device;
	size_t m_FrameInFlight;

	typedef std::vector<IKUniformBufferPtr> FrameBufferList;
	FrameBufferList m_FrameContantBuffer[CBT_COUNT];
public:
	KFrameResourceManager();
	~KFrameResourceManager();

	bool Init(IKRenderDevice* device, size_t frameInFlight);
	bool UnInit();

	IKUniformBufferPtr GetConstantBuffer(size_t frameIndex, ConstantBufferType type);
	size_t GetFrameInFight() const { return m_FrameInFlight; }	
};