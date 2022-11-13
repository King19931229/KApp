#include "KDebugDrawer.h"
#include "Internal/KRenderGlobal.h"

const VertexFormat KDebugDrawSharedData::ms_VertexFormats[] = { VF_SCREENQUAD_POS };

const KVertexDefinition::SCREENQUAD_POS_2F KDebugDrawSharedData::ms_BackGroundVertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KDebugDrawSharedData::ms_BackGroundIndices[] = { 0, 1, 2, 2, 3, 0 };

IKVertexBufferPtr KDebugDrawSharedData::ms_BackGroundVertexBuffer = nullptr;
IKIndexBufferPtr KDebugDrawSharedData::ms_BackGroundIndexBuffer = nullptr;

IKShaderPtr KDebugDrawSharedData::ms_DebugVertexShader = nullptr;
IKShaderPtr KDebugDrawSharedData::ms_DebugFragmentShader = nullptr;

KVertexData KDebugDrawSharedData::ms_DebugVertexData;
KIndexData KDebugDrawSharedData::ms_DebugIndexData;

IKSamplerPtr KDebugDrawSharedData::ms_DebugSampler = nullptr;

bool KDebugDrawSharedData::Init()
{
	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	renderDevice->CreateSampler(ms_DebugSampler);
	ms_DebugSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	ms_DebugSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	ms_DebugSampler->Init(0, 0);

	renderDevice->CreateShader(ms_DebugVertexShader);
	renderDevice->CreateShader(ms_DebugFragmentShader);

	ASSERT_RESULT(ms_DebugVertexShader->InitFromFile(ST_VERTEX, "others/debugquad.vert", false));
	ASSERT_RESULT(ms_DebugFragmentShader->InitFromFile(ST_FRAGMENT, "others/debugquadcolor.frag", false));

	renderDevice->CreateVertexBuffer(ms_BackGroundVertexBuffer);
	ms_BackGroundVertexBuffer->InitMemory(ARRAY_SIZE(ms_BackGroundVertices), sizeof(ms_BackGroundVertices[0]), ms_BackGroundVertices);
	ms_BackGroundVertexBuffer->InitDevice(false);

	renderDevice->CreateIndexBuffer(ms_BackGroundIndexBuffer);
	ms_BackGroundIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_BackGroundIndices), ms_BackGroundIndices);
	ms_BackGroundIndexBuffer->InitDevice(false);

	ms_DebugVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, ms_BackGroundVertexBuffer);
	ms_DebugVertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	ms_DebugVertexData.vertexCount = ARRAY_SIZE(ms_BackGroundVertices);
	ms_DebugVertexData.vertexStart = 0;

	ms_DebugIndexData.indexBuffer = ms_BackGroundIndexBuffer;
	ms_DebugIndexData.indexCount = ARRAY_SIZE(ms_BackGroundIndices);
	ms_DebugIndexData.indexStart = 0;

	return true;
}

bool KDebugDrawSharedData::UnInit()
{
	SAFE_UNINIT(ms_DebugSampler);
	SAFE_UNINIT(ms_BackGroundVertexBuffer);
	SAFE_UNINIT(ms_BackGroundIndexBuffer);
	SAFE_UNINIT(ms_DebugVertexShader);
	SAFE_UNINIT(ms_DebugFragmentShader);
	return true;
}

KRTDebugDrawer::KRTDebugDrawer()
	: m_Enable(false)
{
}

KRTDebugDrawer::~KRTDebugDrawer()
{
	ASSERT_RESULT(!m_Pipeline);
	ASSERT_RESULT(!m_Target);
}

bool KRTDebugDrawer::Init(IKRenderTargetPtr target, float x, float y, float width, float height)
{
	UnInit();

	m_Target = target;

	m_Rect.x = x;
	m_Rect.y = y;
	m_Rect.w = width;
	m_Rect.h = height;

	KRenderGlobal::RenderDevice->CreatePipeline(m_Pipeline);

	m_Pipeline->SetVertexBinding(KDebugDrawSharedData::ms_VertexFormats, ARRAY_SIZE(KDebugDrawSharedData::ms_VertexFormats));
	m_Pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

	m_Pipeline->SetBlendEnable(true);
	m_Pipeline->SetCullMode(CM_BACK);
	m_Pipeline->SetFrontFace(FF_CLOCKWISE);

	m_Pipeline->SetDepthFunc(CF_ALWAYS, false, false);
	m_Pipeline->SetShader(ST_VERTEX, KDebugDrawSharedData::ms_DebugVertexShader);
	m_Pipeline->SetShader(ST_FRAGMENT, KDebugDrawSharedData::ms_DebugFragmentShader);
	m_Pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_Target->GetFrameBuffer(), KDebugDrawSharedData::ms_DebugSampler, true);

	ASSERT_RESULT(m_Pipeline->Init());

	KRenderGlobal::RenderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
	KRenderGlobal::RenderDevice->CreateCommandBuffer(m_SecondaryBuffer);
	m_SecondaryBuffer->Init(m_CommandPool, CBL_SECONDARY);

	return true;
}

bool KRTDebugDrawer::UnInit()
{
	m_Target = nullptr;
	SAFE_UNINIT(m_Pipeline);

	SAFE_UNINIT(m_SecondaryBuffer);
	SAFE_UNINIT(m_CommandPool);

	return true;
}

void KRTDebugDrawer::EnableDraw()
{
	m_Enable = true;
}

void KRTDebugDrawer::DisableDraw()
{
	m_Enable = false;
}

bool KRTDebugDrawer::Render(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	if (m_Enable)
	{
		ASSERT_RESULT(m_Pipeline);

		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f)) * m_Clip;
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(m_Rect.w, m_Rect.h, 1.0f)) * m_Clip;
		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(m_Rect.x, m_Rect.y, 0.0f)) * m_Clip;
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)) * m_Clip;
		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)) * m_Clip;

		KRenderCommand command;

		command.vertexData = &KDebugDrawSharedData::ms_DebugVertexData;
		command.indexData = &KDebugDrawSharedData::ms_DebugIndexData;
		command.pipeline = m_Pipeline;
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = true;

		command.objectUsage.binding = SHADER_BINDING_OBJECT;
		command.objectUsage.range = sizeof(m_Clip);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&m_Clip, command.objectUsage);

		m_SecondaryBuffer->BeginSecondary(renderPass);
		m_SecondaryBuffer->SetViewport(renderPass->GetViewPort());
		m_SecondaryBuffer->Render(command);
		m_SecondaryBuffer->End();

		primaryBuffer->Execute(m_SecondaryBuffer);
	}
	return true;
}