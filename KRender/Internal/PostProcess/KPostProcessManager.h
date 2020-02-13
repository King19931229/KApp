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
#include <unordered_set>

class KPostProcessManager : public IKPostProcessManager
{
	friend class KPostProcessPass;
protected:
	IKRenderDevice* m_Device;
	IKCommandPoolPtr m_CommandPool;
	size_t m_FrameInFlight;

	size_t m_Width;
	size_t m_Height;

	std::unordered_map<IKPostProcessNode::IDType, IKPostProcessNodePtr> m_AllNodes;
	IKPostProcessNodePtr m_StartPointPass;
	std::unordered_set<IKPostProcessNodePtr> m_DeletedNodes;

	std::unordered_map<KPostProcessConnection::IDType, IKPostProcessConnectionPtr> m_AllConnections;
	std::unordered_set<IKPostProcessConnectionPtr> m_DeletedConnections;

	static const KVertexDefinition::SCREENQUAD_POS_2F ms_vertices[4];
	static const uint32_t ms_Indices[6];

	KVertexData m_SharedVertexData;
	KIndexData m_SharedIndexData;

	IKVertexBufferPtr m_SharedVertexBuffer;
	IKIndexBufferPtr m_SharedIndexBuffer;

	IKShaderPtr m_ScreenDrawVS;
	IKShaderPtr m_ScreenDrawFS;

	IKSamplerPtr m_Sampler;
	bool m_bConstructed;

	static const char* msTypeKey;
	static const char* msIDKey;

	static const char* msStartPointKey;

	static const char* msNodesKey;
	static const char* msConnectionsKey;

	void ClearDeletedPassConnection();
	void ClearCreatedPassConnection();
	void IterPostProcessGraph(std::function<void(IKPostProcessNode*)> func);
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

	IKPostProcessNodePtr CreatePass() override;
	IKPostProcessNodePtr CreateTextrue() override;

	void DeleteNode(IKPostProcessNodePtr node) override;
	IKPostProcessNodePtr GetNode(IKPostProcessNode::IDType id) override;
	bool GetAllNodes(KPostProcessNodeSet& set) override;

	IKPostProcessConnectionPtr CreateConnection(IKPostProcessNodePtr outNode, int16_t outSlot, IKPostProcessNodePtr inNode, int16_t inSlot) override;
	void DeleteConnection(IKPostProcessConnectionPtr conn) override;
	IKPostProcessConnectionPtr GetConnection(KPostProcessConnection::IDType id);

	IKPostProcessNodePtr GetStartPointPass() override;

	bool Construct() override;
	bool Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChainPtr& swapChain, IKUIOverlayPtr& ui, IKCommandBufferPtr primaryCommandBuffer);

	inline IKRenderDevice* GetDevice() { return m_Device; }	
};