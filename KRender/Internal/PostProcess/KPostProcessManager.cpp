#include "KPostProcessManager.h"
#include "KPostProcessPass.h"

#include <queue>

const KVertexDefinition::SCREENQUAD_POS_2F KPostProcessManager::ms_vertices[] = 
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint32_t KPostProcessManager::ms_Indices[] = {0, 1, 2, 2, 3, 0};

KPostProcessManager::KPostProcessManager()
	: m_Device(nullptr)
{

}

KPostProcessManager::~KPostProcessManager()
{
	assert(m_AllPasses.empty());
	assert(m_CommandPool == nullptr);
	assert(m_SharedVertexBuffer == nullptr);
	assert(m_SharedIndexBuffer == nullptr);
}

bool KPostProcessManager::Init(IKRenderDevice* device,
							   size_t width, size_t height,
							   unsigned short massCount,
							   ElementFormat startFormat,
							   size_t frameInFlight)
{
	UnInit();

	m_Device = device;

	m_Device->CreateVertexBuffer(m_SharedVertexBuffer);
	m_SharedVertexBuffer->InitMemory(ARRAY_SIZE(ms_vertices), sizeof(ms_vertices[0]), ms_vertices);
	m_SharedVertexBuffer->InitDevice(false);

	m_Device->CreateIndexBuffer(m_SharedIndexBuffer);
	m_SharedIndexBuffer->InitMemory(IT_32, ARRAY_SIZE(ms_Indices), ms_Indices);
	m_SharedIndexBuffer->InitDevice(false);

	m_SharedVertexData.vertexStart = 0;
	m_SharedVertexData.vertexCount = ARRAY_SIZE(ms_vertices);
	m_SharedVertexData.vertexFormats = std::vector<VertexFormat>(1, VF_SCREENQUAD_POS);
	m_SharedVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_SharedVertexBuffer);

	m_SharedIndexData.indexStart = 0;
	m_SharedIndexData.indexCount = ARRAY_SIZE(ms_Indices);
	m_SharedIndexData.indexBuffer = m_SharedIndexBuffer;

	m_StartPointPass = new KPostProcessPass(this);
	m_StartPointPass->SetFormat(startFormat);
	m_StartPointPass->SetMSAA(massCount);
	m_StartPointPass->SetSize(width, height);
	m_StartPointPass->Init(frameInFlight, POST_PROCESS_STAGE_START_POINT);

	m_Device->CreateCommandPool(m_CommandPool);

	m_AllPasses.insert(m_StartPointPass);

	return true;
}

bool KPostProcessManager::UnInit()
{
	m_SharedVertexData.Destroy();
	m_SharedIndexData.Destroy();

	m_SharedVertexBuffer = nullptr;
	m_SharedIndexBuffer = nullptr;

	for(KPostProcessPass* pass : m_AllPasses)
	{
		pass->UnInit();
		if(pass == m_StartPointPass)
		{
			m_StartPointPass = nullptr;
		}
		SAFE_DELETE(pass);
	}
	m_AllPasses.clear();

	assert(m_StartPointPass == nullptr);
	SAFE_DELETE(m_StartPointPass);

	if(m_CommandPool)
	{
		m_CommandPool->UnInit();
		m_CommandPool = nullptr;
	}

	return true;
}

bool KPostProcessManager::Resize(size_t width, size_t height)
{
	m_Device->Wait();

	std::map<KPostProcessPass*, bool> visitFlags;

	for(KPostProcessPass* pass : m_AllPasses)
	{
		visitFlags[pass] = false;
		pass->UnInit();
	}

	std::queue<KPostProcessPass*> bfsQueue;
	bfsQueue.push(m_StartPointPass);

	while(!bfsQueue.empty())
	{
		KPostProcessPass* pass = bfsQueue.front();
		bfsQueue.pop();

		if(!visitFlags[pass])
		{
			visitFlags[pass] = true;

			pass->SetSize(width, height);
			pass->Init(pass->m_FrameInFlight, pass->m_Flags);

			for(KPostProcessPass::PassConnection& conn : pass->m_OutputConnections)
			{
				KPostProcessPass* outPass = conn.pass;
				bfsQueue.push(outPass);
			}
		}
	}

	return true;
}

IKRenderTargetPtr KPostProcessManager::GetOffscreenTarget(size_t frameIndex)
{
	return m_StartPointPass ? m_StartPointPass->GetRenderTarget(frameIndex) : nullptr;
}

IKTexturePtr KPostProcessManager::GetOffscreenTextrue(size_t frameIndex)
{
	return m_StartPointPass ? m_StartPointPass->GetTexture(frameIndex) : nullptr;
}