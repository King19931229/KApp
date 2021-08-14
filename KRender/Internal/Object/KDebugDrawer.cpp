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

IKVertexBufferPtr KDebugDrawSharedData::m_BackGroundVertexBuffer = nullptr;
IKIndexBufferPtr KDebugDrawSharedData::m_BackGroundIndexBuffer = nullptr;

IKShaderPtr KDebugDrawSharedData::m_DebugVertexShader = nullptr;
IKShaderPtr KDebugDrawSharedData::m_DebugFragmentShader = nullptr;

KVertexData KDebugDrawSharedData::m_DebugVertexData;
KIndexData KDebugDrawSharedData::m_DebugIndexData;

IKSamplerPtr KDebugDrawSharedData::m_DebugSampler = nullptr;

bool KDebugDrawSharedData::Init()
{
	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	renderDevice->CreateSampler(m_DebugSampler);
	m_DebugSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_DebugSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_DebugSampler->Init(0, 0);

	renderDevice->CreateShader(m_DebugVertexShader);
	renderDevice->CreateShader(m_DebugFragmentShader);

	ASSERT_RESULT(m_DebugVertexShader->InitFromFile(ST_VERTEX, "others/debugquad.vert", false));
	ASSERT_RESULT(m_DebugFragmentShader->InitFromFile(ST_FRAGMENT, "others/debugquadcolor.frag", false));

	renderDevice->CreateVertexBuffer(m_BackGroundVertexBuffer);
	m_BackGroundVertexBuffer->InitMemory(ARRAY_SIZE(ms_BackGroundVertices), sizeof(ms_BackGroundVertices[0]), ms_BackGroundVertices);
	m_BackGroundVertexBuffer->InitDevice(false);

	renderDevice->CreateIndexBuffer(m_BackGroundIndexBuffer);
	m_BackGroundIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_BackGroundIndices), ms_BackGroundIndices);
	m_BackGroundIndexBuffer->InitDevice(false);

	m_DebugVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_BackGroundVertexBuffer);
	m_DebugVertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	m_DebugVertexData.vertexCount = ARRAY_SIZE(ms_BackGroundVertices);
	m_DebugVertexData.vertexStart = 0;

	m_DebugIndexData.indexBuffer = m_BackGroundIndexBuffer;
	m_DebugIndexData.indexCount = ARRAY_SIZE(ms_BackGroundIndices);
	m_DebugIndexData.indexStart = 0;

	return true;
}

bool KDebugDrawSharedData::UnInit()
{
	SAFE_UNINIT(m_DebugSampler);
	SAFE_UNINIT(m_BackGroundVertexBuffer);
	SAFE_UNINIT(m_BackGroundIndexBuffer);
	SAFE_UNINIT(m_DebugVertexShader);
	SAFE_UNINIT(m_DebugFragmentShader);
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

bool KRTDebugDrawer::Init(IKRenderTargetPtr target)
{
	UnInit();

	m_Target = target;

	KRenderGlobal::RenderDevice->CreatePipeline(m_Pipeline);

	m_Pipeline->SetVertexBinding(KDebugDrawSharedData::ms_VertexFormats, ARRAY_SIZE(KDebugDrawSharedData::ms_VertexFormats));
	m_Pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

	m_Pipeline->SetBlendEnable(true);
	m_Pipeline->SetCullMode(CM_BACK);
	m_Pipeline->SetFrontFace(FF_CLOCKWISE);

	m_Pipeline->SetDepthFunc(CF_ALWAYS, false, false);
	m_Pipeline->SetShader(ST_VERTEX, KDebugDrawSharedData::m_DebugVertexShader);
	m_Pipeline->SetShader(ST_FRAGMENT, KDebugDrawSharedData::m_DebugFragmentShader);
	m_Pipeline->SetSampler(SHADER_BINDING_TEXTURE0, target, KDebugDrawSharedData::m_DebugSampler, true);

	ASSERT_RESULT(m_Pipeline->Init());

	return true;
}

bool KRTDebugDrawer::UnInit()
{
	m_Target = nullptr;
	SAFE_UNINIT(m_Pipeline);
	return true;
}

void KRTDebugDrawer::EnableDraw(float x, float y, float width, float height)
{
	m_Rect.x = x;
	m_Rect.y = y;
	m_Rect.w = width;
	m_Rect.h = height;
	m_Enable = true;
}

void KRTDebugDrawer::DisableDraw()
{
	m_Enable = false;
}

bool KRTDebugDrawer::GetDebugRenderCommand(KRenderCommandList& commands)
{
	if (m_Enable)
	{
		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f)) * m_Clip;
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(m_Rect.w, m_Rect.h, 1.0f)) * m_Clip;
		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(m_Rect.x, m_Rect.y, 0.0f)) * m_Clip;
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)) * m_Clip;
		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)) * m_Clip;

		KRenderCommand command;

		command.vertexData = &KDebugDrawSharedData::m_DebugVertexData;
		command.indexData = &KDebugDrawSharedData::m_DebugIndexData;
		command.pipeline = m_Pipeline;
		command.indexDraw = true;

		command.objectUsage.binding = SHADER_BINDING_OBJECT;
		command.objectUsage.range = sizeof(m_Clip);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&m_Clip, command.objectUsage);

		commands.push_back(std::move(command));
	}
	return true;
}