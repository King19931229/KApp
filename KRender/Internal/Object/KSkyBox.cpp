#include "KSkyBox.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

#if 0
      1------2
      /|    /|
     / |   / |
    5-----4  |
    |  0--|--3
    | /   | /
    |/    |/
    6-----7
#endif

const KVertexDefinition::POS_3F_NORM_3F_UV_2F KSkyBox::ms_Positions[] =
{
	// Now position and normal is important. As for uv, we really don't care
	{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
	{glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 1.0f)},

	{glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
	{glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(-1.0, -1.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, -1.0f, 1.0f), glm::vec2(1.0f, 1.0f)}
};

const uint16_t KSkyBox::ms_Indices[] =
{
	// up
	5, 1, 2, 2, 4, 5,
	// down
	6, 3, 0, 3, 6, 7,
	// left
	0, 1, 5, 0, 5, 6,
	// right
	7, 4, 2, 7, 2, 3,
	// front
	6, 5, 4, 6, 4, 7,
	// back
	0, 2, 1, 2, 0, 3,
};

const VertexFormat KSkyBox::ms_VertexFormats[] = {VF_POINT_NORMAL_UV};

KSkyBox::KSkyBox()
{
}

KSkyBox::~KSkyBox()
{
}

void KSkyBox::LoadResource(const char* cubeTexPath)
{
	ASSERT_RESULT(KRenderGlobal::TextrueManager.Acquire(cubeTexPath, m_CubeTexture, false));

	m_CubeSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_CubeSampler->Init(m_CubeTexture, false);

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "cube.vert", m_VertexShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "cube.frag", m_FragmentShader, false));

	m_VertexBuffer->InitMemory(ARRAY_SIZE(ms_Positions), sizeof(ms_Positions[0]), ms_Positions);
	m_VertexBuffer->InitDevice(false);

	m_IndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_Indices), ms_Indices);
	m_IndexBuffer->InitDevice(false);
}

void KSkyBox::PreparePipeline()
{
	for(size_t i = 0; i < m_Pipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_Pipelines[i];
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		//IKUniformBufferPtr objectBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, CBT_OBJECT);
		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, CBT_CAMERA);

		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_CubeTexture, m_CubeSampler);

		ASSERT_RESULT(pipeline->Init());
	}
}

void KSkyBox::InitRenderData()
{
	m_VertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_VertexBuffer);
	m_VertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	m_VertexData.vertexCount = ARRAY_SIZE(ms_Positions);
	m_VertexData.vertexStart = 0;

	m_IndexData.indexBuffer = m_IndexBuffer;
	m_IndexData.indexCount = ARRAY_SIZE(ms_Indices);
	m_IndexData.indexStart = 0;
}

bool KSkyBox::Init(IKRenderDevice* renderDevice, size_t frameInFlight, const char* cubeTexPath)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(frameInFlight > 0);
	ASSERT_RESULT(cubeTexPath != nullptr);

	ASSERT_RESULT(UnInit());

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateSampler(m_CubeSampler);
	renderDevice->CreateVertexBuffer(m_VertexBuffer);
	renderDevice->CreateIndexBuffer(m_IndexBuffer);

	m_Pipelines.resize(frameInFlight);
	m_CommandBuffers.resize(frameInFlight);

	for(size_t i = 0; i < frameInFlight; ++i)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(m_Pipelines[i]);

		IKCommandBufferPtr& buffer = m_CommandBuffers[i];
		ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
		ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
	}

	LoadResource(cubeTexPath);
	PreparePipeline();
	InitRenderData();

	return true;
}

bool KSkyBox::UnInit()
{
	for(IKPipelinePtr pipeline : m_Pipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_Pipelines.clear();

	for (IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_CommandBuffers.clear();

	SAFE_UNINIT(m_VertexBuffer);
	SAFE_UNINIT(m_IndexBuffer);

	if (m_VertexShader)
	{
		KRenderGlobal::ShaderManager.Release(m_VertexShader);
	}
	if (m_FragmentShader)
	{
		KRenderGlobal::ShaderManager.Release(m_FragmentShader);
	}
	if (m_CubeTexture)
	{
		KRenderGlobal::TextrueManager.Release(m_CubeTexture);
	}

	SAFE_UNINIT(m_CubeSampler);
	SAFE_UNINIT(m_CommandPool);

	m_VertexData.Clear();
	m_IndexData.Clear();

	return true;
}

bool KSkyBox::Render(size_t frameIndex, IKRenderTargetPtr target, std::vector<IKCommandBufferPtr>& buffers)
{
	if (frameIndex < m_Pipelines.size())
	{
		KRenderCommand command;
		command.vertexData = &m_VertexData;
		command.indexData = &m_IndexData;
		command.pipeline = m_Pipelines[frameIndex];
		command.pipeline->GetHandle(target, command.pipelineHandle);
		command.indexDraw = true;

		IKCommandBufferPtr commandBuffer = m_CommandBuffers[frameIndex];

		commandBuffer->BeginSecondary(target);
		commandBuffer->SetViewport(target);
		commandBuffer->Render(command);
		commandBuffer->End();

		buffers.push_back(commandBuffer);
		return true;
	}
	return false;
}