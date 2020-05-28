#pragma once

#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"

class KRenderComponent;

class KOcclusionBox
{
protected:
	static const KVertexDefinition::POS_3F_NORM_3F_UV_2F ms_Positions[8];
	static const uint16_t ms_Indices[36];
	static const VertexFormat ms_VertexFormats[1];

	std::vector<IKPipelinePtr> m_PipelinesFrontFace;
	std::vector<IKPipelinePtr> m_PipelinesBackFace;

	std::vector<IKCommandBufferPtr> m_CommandBuffers;
	IKCommandPoolPtr m_CommandPool;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	KVertexData m_VertexData;
	KIndexData m_IndexData;

	void LoadResource();
	void PreparePipeline();
	void InitRenderData();
public:
	KOcclusionBox();
	~KOcclusionBox();

	bool Init(IKRenderDevice* renderDevice, size_t frameInFlight);
	bool UnInit();

	bool Reset(size_t frameIndex, std::vector<KRenderComponent*>& cullRes, IKCommandBufferPtr primaryCommandBuffer);
	bool Render(size_t frameIndex, IKRenderTargetPtr target, std::vector<KRenderComponent*>& cullRes, std::vector<IKCommandBufferPtr>& buffers);
};