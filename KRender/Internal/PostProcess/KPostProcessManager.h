#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPostProcess.h"
#include "Internal/KVertexDefinition.h"
#include "KPostProcessPass.h"
#include "KPostProcessConnection.h"

#include <unordered_map>

class KPostProcessManager : public IKPostProcessManager
{
	friend class KPostProcessPass;
protected:
	IKRenderDevice* m_Device;
	IKCommandPoolPtr m_CommandPool;
	size_t m_FrameInFlight;

	size_t m_Width;
	size_t m_Height;

	std::unordered_map<KPostProcessPass::IDType, KPostProcessPass*> m_AllPasses;
	KPostProcessPass* m_StartPointPass;

	std::unordered_map<KPostProcessConnection::IDType, KPostProcessConnection*> m_AllConnections;

	static const KVertexDefinition::SCREENQUAD_POS_2F ms_vertices[4];
	static const uint32_t ms_Indices[6];

	KVertexData m_SharedVertexData;
	KIndexData m_SharedIndexData;

	IKVertexBufferPtr m_SharedVertexBuffer;
	IKIndexBufferPtr m_SharedIndexBuffer;

	IKShaderPtr m_ScreenDrawVS;
	IKShaderPtr m_ScreenDrawFS;

	IKSamplerPtr m_Sampler;

	static const char* msStartPointKey;
	static const char* msPassesKey;
	static const char* msConnectionsKey;

	void ClearCreatedPassConnection();
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

	bool Save(const char* jsonFile);
	bool Load(const char* jsonFile);

	IKPostProcessPass* CreatePass(const char* vsFile, const char* fsFile, float scale, ElementFormat format) override;
	void DeletePass(IKPostProcessPass* pass) override;
	KPostProcessPass* GetPass(KPostProcessPass::IDType id);

	bool GetAllPasses(KPostProcessPassSet& set) override;

	IKPostProcessConnection* CreatePassConnection(IKPostProcessPass* outputPass, int16_t outSlot, IKPostProcessPass* inputPass, int16_t inSlot) override;
	IKPostProcessConnection* CreateTextureConnection(IKTexturePtr outputTexure, int16_t outSlot, IKPostProcessPass* inputPass, int16_t inSlot) override;
	void DeleteConnection(IKPostProcessConnection* conn) override;
	KPostProcessConnection* GetConnection(KPostProcessConnection::IDType id);

	IKPostProcessPass* GetStartPointPass() override;

	bool Construct();
	bool Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChainPtr& swapChain, IKUIOverlayPtr& ui, IKCommandBufferPtr primaryCommandBuffer);

	inline IKRenderDevice* GetDevice() { return m_Device; }	
};