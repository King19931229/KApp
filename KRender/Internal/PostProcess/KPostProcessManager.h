#pragma once
#include "Interface/IKRenderDevice.h"
#include <set>

class KPostProcessPass;

class KPostProcessManager
{
	IKRenderDevice* m_Device;
	IKCommandPoolPtr m_CommandPool;

	std::set<KPostProcessPass*> m_AllPasses;
	KPostProcessPass* m_StartPointPass;
public:
	KPostProcessManager();
	~KPostProcessManager();

	bool Init(IKRenderDevice* device,
		size_t width, size_t height,
		ElementFormat startFormat,
		size_t frameInFlight);
	bool UnInit();

	bool Resize(size_t width, size_t height);

	inline IKRenderDevice* GetDevice() { return m_Device; }
	inline IKCommandPoolPtr GetCommandPool() { return m_CommandPool; }
};