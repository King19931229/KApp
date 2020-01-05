#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKCommandBuffer.h"

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
		unsigned short massCount,
		ElementFormat startFormat,
		size_t frameInFlight);
	bool UnInit();

	bool Resize(size_t width, size_t height);

	IKRenderTargetPtr GetOffscreenTarget(size_t frameIndex);
	IKTexturePtr GetOffscreenTextrue(size_t frameIndex);

	inline IKRenderDevice* GetDevice() { return m_Device; }	
	inline IKCommandPoolPtr GetCommandPool() { return m_CommandPool; }
};