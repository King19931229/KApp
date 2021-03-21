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
	renderDevice->CreateTexture(m_CubeMap);
	m_CubeMap->InitMemoryFromFile("Textures/uffizi_cube.ktx", true, false);
	m_CubeMap->InitDevice(false);

	AllocateTempResource(renderDevice, width, height, mipmaps, materialPath);
	Draw();
	FreeTempResource();

	return true;
}

bool KPrefilerCubeMap::UnInit()
{
	FreeTempResource();
	SAFE_UNINIT(m_CubeMap);
	return true;
}

bool KPrefilerCubeMap::PopulateRenderCommand(KRenderCommand& command, IKPipelinePtr pipeline, IKRenderPassPtr renderPass)
{
	IKPipelineHandlePtr pipeHandle = nullptr;
	if (pipeline->GetHandle(renderPass, pipeHandle))
	{
		command.vertexData = &m_SharedVertexData;
		command.indexData = &m_SharedIndexData;
		command.pipeline = pipeline;
		command.pipelineHandle = pipeHandle;

		command.indexDraw = true;

		return true;
	}
	return false;
}

bool KPrefilerCubeMap::AllocateTempResource(IKRenderDevice* renderDevice,
	uint32_t width, uint32_t height,
	size_t mipmaps,
	const char* materialPath)
{
	VertexFormat formats[] = { VF_SCREENQUAD_POS };

	// VertexData
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

	// Material
	{
		KRenderGlobal::MaterialManager.Acquire(materialPath, m_Material, false);
		ASSERT_RESULT(m_Material);

		IKShaderPtr vertexShader = m_Material->GetVSShader(formats, 1);
		IKShaderPtr fragmentShader = m_Material->GetFSShader(formats, 1, &m_TextureBinding);

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

	m_MipmapTargets.resize(m_CubeMap->GetMipmaps());
	for (uint32_t mipLevel = 0; mipLevel < (uint32_t)m_MipmapTargets.size(); ++mipLevel)
	{
		MipmapTarget& mipTarget = m_MipmapTargets[mipLevel];

		IKRenderPassPtr& pass = mipTarget.pass;
		IKRenderTargetPtr& target = mipTarget.target;

		renderDevice->CreateRenderPass(pass);
		renderDevice->CreateRenderTarget(target);
		target->InitFromColor((uint32_t)m_CubeMap->GetWidth() >> mipLevel, (uint32_t)m_CubeMap->GetHeight() >> mipLevel, 1, m_CubeMap->GetTextureFormat());
		pass->SetColorAttachment(0, target->GetFrameBuffer());
		pass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		pass->Init();
	}

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
	renderDevice->CreateCommandBuffer(m_CommandBuffer);
	m_CommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	return true;
}

bool KPrefilerCubeMap::FreeTempResource()
{
	SAFE_UNINIT(m_SharedVertexBuffer);
	SAFE_UNINIT(m_SharedIndexBuffer);
	SAFE_UNINIT(m_Pipeline);

	for (size_t mipLevel = 0; mipLevel < m_MipmapTargets.size(); ++mipLevel)
	{
		MipmapTarget& mipTarget = m_MipmapTargets[mipLevel];
		SAFE_UNINIT(mipTarget.pass);
		SAFE_UNINIT(mipTarget.target);
	}
	m_MipmapTargets.clear();

	SAFE_UNINIT(m_CommandBuffer);
	SAFE_UNINIT(m_CommandPool);
	return true;
}

bool KPrefilerCubeMap::Draw()
{
	for (uint32_t mipLevel = 0; mipLevel < (uint32_t)m_MipmapTargets.size(); ++mipLevel)
	{
		MipmapTarget& mipTarget = m_MipmapTargets[mipLevel];
		IKRenderTargetPtr& target = mipTarget.target;
		IKRenderPassPtr& pass = mipTarget.pass;

		m_CommandBuffer->BeginPrimary();
		m_CommandBuffer->BeginRenderPass(pass, SUBPASS_CONTENTS_INLINE);
		m_CommandBuffer->SetViewport(pass->GetViewPort());

		KRenderCommand command;
		if (PopulateRenderCommand(command, m_Pipeline, pass))
		{
			m_CommandBuffer->Render(command);
		}

		m_CommandBuffer->EndRenderPass();
		m_CommandBuffer->End();

		m_CommandBuffer->Flush();

		for (int i = 0; i < 6; ++i)
		{
			m_CubeMap->CopyFromFrameBuffer(target->GetFrameBuffer(), i, mipLevel);
		}
	}
	return true;
}