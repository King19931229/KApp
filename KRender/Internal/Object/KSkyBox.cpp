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
	ASSERT_RESULT(KRenderGlobal::TextureManager.Acquire(cubeTexPath, m_CubeTexture, false));

	m_CubeSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_CubeSampler->Init(0, m_CubeTexture->GetMipmaps() - 1);

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "others/skybox.vert", m_VertexShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "others/skybox.frag", m_FragmentShader, false));

	m_VertexBuffer->InitMemory(ARRAY_SIZE(ms_Positions), sizeof(ms_Positions[0]), ms_Positions);
	m_VertexBuffer->InitDevice(false);

	m_IndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_Indices), ms_Indices);
	m_IndexBuffer->InitDevice(false);
}

void KSkyBox::PreparePipeline()
{
	IKPipelinePtr& pipeline = m_Pipeline;
	pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
	pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	pipeline->SetBlendEnable(false);
	pipeline->SetCullMode(CM_NONE);
	pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	pipeline->SetPolygonMode(PM_FILL);
	pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);
	pipeline->SetShader(ST_VERTEX, *m_VertexShader);
	pipeline->SetShader(ST_FRAGMENT, *m_FragmentShader);

	//IKUniformBufferPtr objectBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_OBJECT);
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);

	pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);
	pipeline->SetSampler(SHADER_BINDING_TEXTURE0, (*m_CubeTexture)->GetFrameBuffer(), m_CubeSampler);

	pipeline->SetDebugName("SkyBoxPipeline");
	ASSERT_RESULT(pipeline->Init());
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

bool KSkyBox::Init(IKRenderDevice* renderDevice, const char* cubeTexPath)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(cubeTexPath != nullptr);

	ASSERT_RESULT(UnInit());

	renderDevice->CreateSampler(m_CubeSampler);
	renderDevice->CreateVertexBuffer(m_VertexBuffer);
	renderDevice->CreateIndexBuffer(m_IndexBuffer);

	KRenderGlobal::RenderDevice->CreatePipeline(m_Pipeline);

	LoadResource(cubeTexPath);
	PreparePipeline();
	InitRenderData();

	return true;
}

bool KSkyBox::UnInit()
{
	SAFE_UNINIT(m_Pipeline);
	SAFE_UNINIT(m_VertexBuffer);
	SAFE_UNINIT(m_IndexBuffer);

	m_VertexShader.Release();
	m_FragmentShader.Release();

	m_CubeTexture.Release();

	SAFE_UNINIT(m_CubeSampler);

	m_VertexData.Reset();
	m_IndexData.Reset();

	return true;
}

bool KSkyBox::Render(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	KRenderCommand command;
	command.vertexData = &m_VertexData;
	command.indexData = &m_IndexData;
	command.pipeline = m_Pipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	commandList.Render(command);

	return true;
}