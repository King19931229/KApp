#include "KQuadDataProvider.h"
#include "Internal/KRenderGlobal.h"

const VertexFormat KQuadDataProvider::ms_QuadFormats[] = { VF_SCREENQUAD_POS };

const KVertexDefinition::SCREENQUAD_POS_2F KQuadDataProvider::ms_QuadVertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KQuadDataProvider::ms_QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

KQuadDataProvider::KQuadDataProvider()
{
}

KQuadDataProvider::~KQuadDataProvider()
{
	ASSERT_RESULT(!m_QuadVertexBuffer);
	ASSERT_RESULT(!m_QuadIndexBuffer);
}

bool KQuadDataProvider::Init()
{
	UnInit();

	KRenderGlobal::RenderDevice->CreateVertexBuffer(m_QuadVertexBuffer);
	m_QuadVertexBuffer->InitMemory(ARRAY_SIZE(ms_QuadVertices), sizeof(ms_QuadVertices[0]), ms_QuadVertices);
	m_QuadVertexBuffer->InitDevice(false);
	m_QuadVertexBuffer->SetDebugName("QuadVertexBuffer");

	KRenderGlobal::RenderDevice->CreateIndexBuffer(m_QuadIndexBuffer);
	m_QuadIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_QuadIndices), ms_QuadIndices);
	m_QuadIndexBuffer->InitDevice(false);
	m_QuadIndexBuffer->SetDebugName("QuadIndexBuffer");

	m_QuadVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_QuadVertexBuffer);
	m_QuadVertexData.vertexFormats = std::vector<VertexFormat>(ms_QuadFormats, ms_QuadFormats + ARRAY_SIZE(ms_QuadFormats));
	m_QuadVertexData.vertexCount = ARRAY_SIZE(ms_QuadVertices);
	m_QuadVertexData.vertexStart = 0;

	m_QuadIndexData.indexBuffer = m_QuadIndexBuffer;
	m_QuadIndexData.indexCount = ARRAY_SIZE(ms_QuadIndices);
	m_QuadIndexData.indexStart = 0;

	return true;
}

bool KQuadDataProvider::UnInit()
{
	SAFE_UNINIT(m_QuadVertexBuffer);
	SAFE_UNINIT(m_QuadIndexBuffer);
	m_QuadVertexData.Reset();
	m_QuadIndexData.Reset();

	return true;
}

const KVertexData& KQuadDataProvider::GetVertexData()
{
	return m_QuadVertexData;
}

const KIndexData& KQuadDataProvider::GetIndexData()
{
	return m_QuadIndexData;
}