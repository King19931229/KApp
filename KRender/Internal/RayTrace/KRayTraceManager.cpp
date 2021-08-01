#include "KRayTraceManager.h"
#include "KRayTraceScene.h"
#include "Internal/KRenderGlobal.h"

KRayTraceManager::KRayTraceManager()
{
}

KRayTraceManager::~KRayTraceManager()
{
}

const VertexFormat KRayTraceManager::ms_VertexFormats[] = { VF_SCREENQUAD_POS };

const KVertexDefinition::SCREENQUAD_POS_2F KRayTraceManager::ms_BackGroundVertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KRayTraceManager::ms_BackGroundIndices[] = { 0, 1, 2, 2, 3, 0 };

bool KRayTraceManager::Init()
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

bool KRayTraceManager::UnInit()
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->UnInit();
	}
	m_Scenes.clear();
	SAFE_UNINIT(m_DebugSampler);
	SAFE_UNINIT(m_BackGroundVertexBuffer);
	SAFE_UNINIT(m_BackGroundIndexBuffer);
	SAFE_UNINIT(m_DebugVertexShader);
	SAFE_UNINIT(m_DebugFragmentShader);
	return true;
}

bool KRayTraceManager::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->Execute(primaryBuffer, frameIndex);
	}
	return true;
}

bool KRayTraceManager::UpdateCamera(uint32_t frameIndex)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->UpdateCamera(frameIndex);
	}
	return true;
}

bool KRayTraceManager::Resize(size_t width, size_t height)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		KRayTraceScene* traceScene = (KRayTraceScene*)scene.get();
		traceScene->UpdateSize();
	}
	return true;
}

bool KRayTraceManager::ReloadShader()
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		KRayTraceScene* traceScene = (KRayTraceScene*)scene.get();
		traceScene->ReloadShader();
	}
	return true;
}

bool KRayTraceManager::AcquireRayTraceScene(IKRayTraceScenePtr& scene)
{
	scene = IKRayTraceScenePtr(KNEW KRayTraceScene());
	m_Scenes.insert(scene);
	return true;
}

bool KRayTraceManager::RemoveRayTraceScene(IKRayTraceScenePtr& scene)
{
	auto it = m_Scenes.find(scene);
	if (it != m_Scenes.end())
		m_Scenes.erase(it);
	return true;
}

bool KRayTraceManager::GetDebugRenderCommand(KRenderCommandList& commands)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->GetDebugRenderCommand(commands);
	}
	return true;
}