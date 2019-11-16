#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/KConstantDefinition.h"

class KFrameResourceManager
{
protected:	
	IKRenderDevice* m_Device;
	size_t m_FrameInFlight;
	size_t m_RenderThreadNum;

	typedef std::vector<IKUniformBufferPtr> ConstantBufferList;
	struct FrameBufferData
	{
		IKUniformBufferPtr buffer;
		ConstantBufferList threadBuffers;
		bool bufferPreThread;

		FrameBufferData()
		{
			buffer = nullptr;
			threadBuffers.clear();
			bufferPreThread = false;
		}
	};
	typedef std::vector<FrameBufferData> FrameBufferDataList;	
	FrameBufferDataList m_FrameContantBuffer[CBT_COUNT];
public:
	KFrameResourceManager();
	~KFrameResourceManager();

	bool Init(IKRenderDevice* device, size_t frameInFlight, size_t renderThreadNum);
	bool UnInit();

	IKUniformBufferPtr GetConstantBuffer(size_t frameIndex, size_t threadIndex, ConstantBufferType type);
	size_t GetFrameInFight() const { return m_FrameInFlight; }	
};