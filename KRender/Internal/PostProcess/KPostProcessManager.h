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
class KPostProcessConnection;

class KPostProcessManager
{
	friend class KPostProcessPass;
protected:
	IKRenderDevice* m_Device;
	IKCommandPoolPtr m_CommandPool;
	size_t m_FrameInFlight;

	size_t m_Width;
	size_t m_Height;

	//
	std::set<KPostProcessPass*> m_AllPasses;
	KPostProcessPass* m_StartPointPass;

	std::set<KPostProcessConnection*> m_AllConnections;

	//
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_vertices[4];
	static const uint32_t ms_Indices[6];

	KVertexData m_SharedVertexData;
	KIndexData m_SharedIndexData;

	IKVertexBufferPtr m_SharedVertexBuffer;
	IKIndexBufferPtr m_SharedIndexBuffer;

	//
	IKShaderPtr m_ScreenDrawVS;
	IKShaderPtr m_ScreenDrawFS;

	IKSamplerPtr m_Sampler;

	void IterPostProcessGraph(std::function<void(KPostProcessPass*)> func);
	void PopulateRenderCommand(KRenderCommand& command, IKPipelinePtr pipeline, IKRenderTargetPtr target);
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

	KPostProcessPass* CreatePass(const char* vsFile, const char* fsFile, float scale, ElementFormat format);
	void DeletePass(KPostProcessPass* pass);

	KPostProcessConnection* CreatePassConnection(KPostProcessPass* input, KPostProcessPass* output, size_t slot);
	KPostProcessConnection* CreateTextureConnection(IKTexturePtr inputTexure, KPostProcessPass* output, size_t slot);
	void DeleteConnection(KPostProcessConnection* conn);

	KPostProcessPass* GetStartPointPass();

	bool Construct();
	bool Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChainPtr& swapChain, IKUIOverlayPtr& ui, IKCommandBufferPtr primaryCommandBuffer);

	inline IKRenderDevice* GetDevice() { return m_Device; }	
};