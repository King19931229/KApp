#pragma once
#include "Interface/IKRenderDevice.h"
#include "KConstantDefinition.h"

class KFrameResourceManager
{
protected:	
	IKRenderDevice* m_Device;
	size_t m_FrameInFlight;

	typedef std::vector<IKUniformBufferPtr> FrameConstantBufferList;
	FrameConstantBufferList m_FrameContantBuffer[CBT_COUNT];
public:
	KFrameResourceManager();
	~KFrameResourceManager();

	bool Init(IKRenderDevice* device, size_t frameInFlight);
	bool UnInit();

	IKUniformBufferPtr GetConstantBuffer(size_t frameIndex, ConstantBufferType type);
	size_t GetFrameInFight() const { return m_FrameInFlight; }	
};