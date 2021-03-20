#include "KPrefiterCubeMap.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

const KVertexDefinition::SCREENQUAD_POS_2F KPrefilerCubeMap::ms_Vertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint32_t KPrefilerCubeMap::ms_Indices[] = { 0, 1, 2, 2, 3, 0 };

KPrefilerCubeMap::KPrefilerCubeMap()
	: m_Pipeline(nullptr)
	, m_CommandPool(nullptr)
	, m_VertexBuffer(nullptr)
	, m_IndexBuffer(nullptr)
	, m_SharedVertexBuffer(nullptr)
	, m_SharedIndexBuffer(nullptr)
	, m_Material(nullptr)
{
	ZERO_ARRAY_MEMORY(m_RenderTargets);
	ZERO_ARRAY_MEMORY(m_RenderPass);

	ZERO_MEMORY(m_SharedVertexData);
	ZERO_MEMORY(m_SharedIndexData);
}

KPrefilerCubeMap::~KPrefilerCubeMap()
{
}

bool KPrefilerCubeMap::Init(IKRenderDevice* renderDevice,
	size_t frameInFlight,
	uint32_t width, uint32_t height,
	size_t mipmaps,
	const char* materialPath)
{
	VertexFormat formats[] = { VF_SCREENQUAD_POS };

	{
		renderDevice->CreateVertexBuffer(m_SharedVertexBuffer);
		m_SharedVertexBuffer->InitMemory(ARRAY_SIZE(ms_Vertices), sizeof(ms_Vertices[0]), ms_Vertices);
		m_SharedVertexBuffer->InitDevice(false);

		renderDevice->CreateIndexBuffer(m_SharedIndexBuffer);
		m_SharedIndexBuffer->InitMemory(IT_32, ARRAY_SIZE(ms_Indices), ms_Indices);
		m_SharedIndexBuffer->InitDevice(false);

		m_SharedVertexData.vertexStart = 0;
		m_SharedVertexData.vertexCount = ARRAY_SIZE(ms_Vertices);
		m_SharedVertexData.vertexFormats = std::vector<VertexFormat>(1, VF_SCREENQUAD_POS);
		m_SharedVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_SharedVertexBuffer);

		m_SharedIndexData.indexStart = 0;
		m_SharedIndexData.indexCount = ARRAY_SIZE(ms_Indices);
		m_SharedIndexData.indexBuffer = m_SharedIndexBuffer;
	}

	{
		KRenderGlobal::MaterialManager.Acquire(materialPath, m_Material, false);
		ASSERT_RESULT(m_Material);

		IKShaderPtr vertexShader = m_Material->GetVSShader(formats, 1);
		IKShaderPtr fragmentShader = m_Material->GetFSShader(formats, 1, &m_Texture);

		renderDevice->CreatePipeline(m_Pipeline);

		IKPipelinePtr& pipeline = m_Pipeline;

		pipeline->SetVertexBinding(formats, 1);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, vertexShader);
		pipeline->SetShader(ST_FRAGMENT, fragmentShader);

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->Init();
	}

	for(int i = 0; i < 6; ++i)
	{
		renderDevice->CreateRenderPass(m_RenderPass[i]);
		renderDevice->CreateRenderTarget(m_RenderTargets[i]);
		m_RenderTargets[i]->InitFromColor(width, height, 1, EF_R16G16B16A16_FLOAT);
		m_RenderPass[i]->SetColorAttachment(0, m_RenderTargets[i]->GetFrameBuffer());
		m_RenderPass[i]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_RenderPass[i]->Init();
	}

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	m_CommandBuffers.resize(frameInFlight);
	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		for (IKCommandBufferPtr& buffer : m_CommandBuffers[i])
		{
			renderDevice->CreateCommandBuffer(buffer);
			buffer->Init(m_CommandPool, CBL_SECONDARY);
		}
	}

	return true;
}

bool KPrefilerCubeMap::UnInit()
{
	SAFE_UNINIT(m_SharedVertexBuffer);
	SAFE_UNINIT(m_SharedIndexBuffer);
	SAFE_UNINIT(m_Pipeline);

	for (int i = 0; i < 6; ++i)
	{
		SAFE_UNINIT(m_RenderTargets[i]);
		SAFE_UNINIT(m_RenderPass[i]);
	}

	for (auto& buffers : m_CommandBuffers)
	{
		for (IKCommandBufferPtr& buffer : buffers)
		{
			buffer->UnInit();
			buffer = nullptr;
		}
	}
	m_CommandBuffers.clear();

	SAFE_UNINIT(m_CommandPool);

	return true;
}

bool KPrefilerCubeMap::Render(size_t frameIndex, IKCommandBufferPtr primaryCommandBuffer)
{
	for (int i = 0; i < 6; ++i)
	{
		primaryCommandBuffer->BeginRenderPass(m_RenderPass[i], SUBPASS_CONTENTS_SECONDARY);
		{
			m_CommandBuffers[frameIndex][i]->BeginSecondary(m_RenderPass[i]);
			m_CommandBuffers[frameIndex][i]->SetViewport(m_RenderPass[i]->GetViewPort());

			KRenderCommand command;

			/*if (PopulateRenderCommand(command, m_Pipeline, m_RenderPass[i]))
			{
				m_CommandBuffers[frameIndex]->Render(command);
			}*/
			m_CommandBuffers[frameIndex][i]->End();
		}
		primaryCommandBuffer->Execute(m_CommandBuffers[frameIndex][i]);
		primaryCommandBuffer->EndRenderPass();
	}

	return true;
}