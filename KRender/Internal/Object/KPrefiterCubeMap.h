#pragma once

#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKRenderPass.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/Material/KMaterialTextureBinding.h"

class KPrefilerCubeMap
{
protected:
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_Vertices[4];
	static const uint32_t ms_Indices[6];

	// 预处理相关资源
	KMaterialTextureBinding m_Texture;
	IKRenderTargetPtr m_RenderTargets[6];
	IKRenderPassPtr	m_RenderPass[6];
	IKPipelinePtr m_Pipeline;
	std::vector<std::array<IKCommandBufferPtr, 6>> m_CommandBuffers;
	IKCommandPoolPtr m_CommandPool;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	KVertexData m_SharedVertexData;
	KIndexData m_SharedIndexData;

	IKVertexBufferPtr m_SharedVertexBuffer;
	IKIndexBufferPtr m_SharedIndexBuffer;

	IKMaterialPtr m_Material;
public:
	KPrefilerCubeMap();
	~KPrefilerCubeMap();

	bool Init(IKRenderDevice* renderDevice,
		size_t frameInFlight,
		uint32_t width, uint32_t height,
		size_t mipmaps,
		const char* materialPath);
	bool UnInit();

	bool Render(size_t frameIndex, IKCommandBufferPtr primaryCommandBuffer);
};