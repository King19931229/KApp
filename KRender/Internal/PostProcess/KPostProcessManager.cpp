#include "KPostProcessManager.h"
#include "KPostProcessPass.h"

#include "Interface/IKSwapChain.h"
#include "Interface/IKUIOverlay.h"

#include "Internal/KRenderGlobal.h"

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
	: m_Device(nullptr),
	m_StartPointPass(nullptr),
	m_FrameInFlight(0),
	m_Width(0),
	m_Height(0)
{
}

KPostProcessManager::~KPostProcessManager()
{
	assert(m_AllPasses.empty());
	assert(m_StartPointPass == nullptr);
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
	m_FrameInFlight = frameInFlight;
	m_Width = width;
	m_Height = height;

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/screenquad.vert", m_ScreenDrawVS));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/screenquad.frag", m_ScreenDrawFS));

	m_Device->CreateSampler(m_Sampler);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->SetMipmapLod(0, 0);
	m_Sampler->Init();

	m_Device->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

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

	m_StartPointPass = new KPostProcessPass(this, frameInFlight, POST_PROCESS_STAGE_START_POINT);
	m_StartPointPass->SetFormat(startFormat);
	m_StartPointPass->SetMSAA(massCount);
	m_StartPointPass->SetSize(width, height);

	m_AllPasses.insert(m_StartPointPass);

	return true;
}

bool KPostProcessManager::UnInit()
{
	m_FrameInFlight = 0;

	if(m_ScreenDrawVS)
	{
		KRenderGlobal::ShaderManager.Release(m_ScreenDrawVS);
		m_ScreenDrawVS = nullptr;
	}
	if(m_ScreenDrawFS)
	{
		KRenderGlobal::ShaderManager.Release(m_ScreenDrawFS);
		m_ScreenDrawFS = nullptr;
	}

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

	if(m_Sampler)
	{
		m_Sampler->UnInit();
		m_Sampler = nullptr;
	}

	return true;
}

void KPostProcessManager::IterPostProcessGraph(std::function<void(KPostProcessPass*)> func)
{
	std::map<KPostProcessPass*, bool> visitFlags;
	std::queue<KPostProcessPass*> bfsQueue;
	bfsQueue.push(m_StartPointPass);

	for(KPostProcessPass* pass : m_AllPasses)
	{
		visitFlags[pass] = false;
	}

	while(!bfsQueue.empty())
	{
		KPostProcessPass* pass = bfsQueue.front();
		bfsQueue.pop();

		if(!visitFlags[pass])
		{
			visitFlags[pass] = true;
			func(pass);
			for(KPostProcessPass::PassConnection& conn : pass->m_OutputConnections)
			{
				KPostProcessPass* outPass = conn.pass;
				bfsQueue.push(outPass);
			}
		}
	}
}

bool KPostProcessManager::Resize(size_t width, size_t height)
{
	m_Device->Wait();

	m_Width = width;
	m_Height = height;

	Construct();

	return true;
}

void KPostProcessManager::PopulateRenderCommand(KRenderCommand& command, IKPipelinePtr pipeline, IKRenderTargetPtr target)
{
	IKPipelineHandlePtr pipeHandle = nullptr;
	KRenderGlobal::PipelineManager.GetPipelineHandle(pipeline, target, pipeHandle);

	command.vertexData = &m_SharedVertexData;
	command.indexData = &m_SharedIndexData;
	command.pipeline = pipeline;
	command.pipelineHandle = pipeHandle;

	command.objectData = nullptr;
	command.objectPushOffset = 0;
	command.useObjectData = false;

	command.indexDraw = true;
}

bool KPostProcessManager::Construct()
{
	for(KPostProcessPass* pass : m_AllPasses)
	{
		pass->UnInit();
	}

	IterPostProcessGraph([this](KPostProcessPass* pass)
	{
		pass->SetSize(m_Width, m_Height);
		pass->Init();
	});
	return true;
}

bool KPostProcessManager::Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChainPtr& swapChain, IKUIOverlayPtr& ui, IKCommandBufferPtr primaryCommandBuffer)
{
	KPostProcessPass* endPass = nullptr;
	{
		IterPostProcessGraph([=, &endPass](KPostProcessPass* pass)
		{
			// TODO
			endPass = pass;

			if(pass == m_StartPointPass)
			{
				return;
			}

			IKCommandBufferPtr commandBuffer = pass->GetCommandBuffer(frameIndex);
			IKRenderTargetPtr renderTarget = pass->GetRenderTarget(frameIndex);

			primaryCommandBuffer->BeginRenderPass(renderTarget, SUBPASS_CONTENTS_SECONDARY);
			{
				commandBuffer->BeginSecondary(renderTarget);
				commandBuffer->SetViewport(renderTarget);

				KRenderCommand command;
				PopulateRenderCommand(command, pass->GetPipeline(frameIndex), renderTarget);
				commandBuffer->Render(command);
				commandBuffer->End();
			}
			primaryCommandBuffer->Execute(commandBuffer);
			primaryCommandBuffer->EndRenderPass();
		});
	}

	{
		IKRenderTargetPtr swapChainTarget = swapChain->GetRenderTarget(chainImageIndex);
		primaryCommandBuffer->BeginRenderPass(swapChainTarget, SUBPASS_CONTENTS_INLINE);

		primaryCommandBuffer->SetViewport(swapChainTarget);

		KRenderCommand command;
		PopulateRenderCommand(command, endPass->GetScreenDrawPipeline(frameIndex), swapChainTarget);
		primaryCommandBuffer->Render(command);

		ui->Draw(frameIndex, swapChainTarget, primaryCommandBuffer);

		primaryCommandBuffer->EndRenderPass();
	}

	return true;
}

KPostProcessPass* KPostProcessManager::CreatePass(const char* vsFile, const char* fsFile, ElementFormat format)
{
	KPostProcessPass* pass = new KPostProcessPass(this, m_FrameInFlight, POST_PROCESS_STAGE_REGULAR);
	pass->SetFormat(format);
	pass->SetMSAA(0);
	pass->SetShader(vsFile, fsFile);
	pass->SetSize(m_Width, m_Height);
	m_AllPasses.insert(pass);
	return pass;
}

void KPostProcessManager::DeletePass(KPostProcessPass* pass)
{
	auto it = m_AllPasses.find(pass);
	if(it != m_AllPasses.end())
	{
		m_AllPasses.erase(it);
		pass->DisconnectAll();
		pass->UnInit();
		SAFE_DELETE(pass);
	}
}

KPostProcessPass* KPostProcessManager::GetStartPointPass()
{
	return m_StartPointPass;
}