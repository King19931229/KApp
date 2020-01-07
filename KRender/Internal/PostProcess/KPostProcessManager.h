#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKCommandBuffer.h"
#include "Internal/KVertexDefinition.h"

#include <set>

class KPostProcessPass;

class KPostProcessManager
{
	friend class KPostProcessPass;
protected:
	IKRenderDevice* m_Device;
	IKCommandPoolPtr m_CommandPool;

	std::set<KPostProcessPass*> m_AllPasses;
	KPostProcessPass* m_StartPointPass;

	static const KVertexDefinition::SCREENQUAD_POS_2F ms_vertices[4];
	static const uint32_t ms_Indices[6];

	KVertexData m_SharedVertexData;
	KIndexData m_SharedIndexData;

	IKVertexBufferPtr m_SharedVertexBuffer;
	IKIndexBufferPtr m_SharedIndexBuffer;
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