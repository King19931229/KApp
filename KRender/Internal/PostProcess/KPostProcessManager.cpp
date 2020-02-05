#include "KPostProcessManager.h"
#include "KPostProcessPass.h"
#include "KPostProcessConnection.h"

#include "KBase/Interface/IKJson.h"
#include "KBase/Interface/IKFileSystem.h"

#include "Interface/IKSwapChain.h"
#include "Interface/IKUIOverlay.h"

#include "Internal/KRenderGlobal.h"

#include <unordered_map>
#include <queue>

EXPORT_DLL IKPostProcessManager* GetProcessManager()
{
	return &KRenderGlobal::PostProcessManager;
}

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
	assert(m_AllConnections.empty());
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

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/screenquad.vert", m_ScreenDrawVS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/screenquad.frag", m_ScreenDrawFS, false));

	m_Device->CreateSampler(m_Sampler);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->Init(0, 0);

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
	m_StartPointPass->SetScale(1.0f);

	m_AllPasses[m_StartPointPass->ID()] = m_StartPointPass;

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

	ClearCreatedPassConnection();

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

void KPostProcessManager::ClearCreatedPassConnection()
{
	for (auto& pair : m_AllPasses)
	{
		KPostProcessPass* pass = pair.second;
		pass->UnInit();
		if (pass == m_StartPointPass)
		{
			m_StartPointPass = nullptr;
		}
		SAFE_DELETE(pass);
	}
	m_AllPasses.clear();

	for (auto& pair : m_AllConnections)
	{
		KPostProcessConnection* conn = pair.second;
		SAFE_DELETE(conn);
	}
	m_AllConnections.clear();
}

void KPostProcessManager::IterPostProcessGraph(std::function<void(KPostProcessPass*)> func)
{
	std::unordered_map<KPostProcessPass*, bool> visitFlags;
	std::queue<KPostProcessPass*> bfsQueue;

	bfsQueue.push(m_StartPointPass);

	for (auto& pair : m_AllPasses)
	{
		KPostProcessPass* pass = pair.second;
		visitFlags[pass] = false;
	}

	while (!bfsQueue.empty())
	{
		KPostProcessPass* pass = bfsQueue.front();
		bfsQueue.pop();

		if(!visitFlags[pass])
		{
			visitFlags[pass] = true;

			func(pass);

			for (int16_t slot = 0; slot < MAX_OUTPUT_SLOT_COUNT; ++slot)
			{
				auto& connections = pass->m_OutputConnection[slot];
				for (IKPostProcessConnection* conn : connections)
				{
					bfsQueue.push(((KPostProcessConnection*)conn)->m_Input.pass);
				}
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

const char* KPostProcessManager::msStartPointKey = "start_point";
const char* KPostProcessManager::msPassesKey = "passes";
const char* KPostProcessManager::msConnectionsKey = "connections";

bool KPostProcessManager::Save(const char* jsonFile)
{
	IKJsonDocumentPtr jsonDoc = GetJsonDocument();
	IKJsonValuePtr root = jsonDoc->GetRoot();

	root->AddMember(msStartPointKey, jsonDoc->CreateString(m_StartPointPass->ID().c_str()));

	auto passes = jsonDoc->CreateArray();
	for (auto& pair : m_AllPasses)
	{
		KPostProcessPass* pass = pair.second;
		IKJsonValuePtr pass_json = nullptr;
		ASSERT_RESULT(pass->Save(jsonDoc, pass_json));
		passes->Push(pass_json);
	}
	root->AddMember(msPassesKey, passes);

	auto connections = jsonDoc->CreateArray();
	for (auto& pair : m_AllConnections)
	{
		KPostProcessConnection* conn = pair.second;
		IKJsonValuePtr conn_json = nullptr;
		ASSERT_RESULT(conn->Save(jsonDoc, conn_json));
		connections->Push(conn_json);
	}
	root->AddMember(msConnectionsKey, connections);

	return jsonDoc->SaveAsFile(jsonFile);
}

bool KPostProcessManager::Load(const char* jsonFile)
{
	IKJsonDocumentPtr jsonDoc = GetJsonDocument();
	IKJsonValuePtr root = jsonDoc->GetRoot();

	IKDataStreamPtr fileStream = nullptr;
	if (KFileSystem::Manager->Open(jsonFile, IT_FILEHANDLE, fileStream))
	{
		if (jsonDoc->ParseFromDataStream(fileStream))
		{
			ClearCreatedPassConnection();

			auto startID = jsonDoc->GetMember(msStartPointKey)->GetString();

			auto passes = jsonDoc->GetMember(msPassesKey);
			for (size_t i = 0, count = passes->Size(); i < count; ++i)
			{
				auto pass_json = passes->GetArrayElement(i);
				KPostProcessPass* pass = new KPostProcessPass(this, m_FrameInFlight, POST_PROCESS_STAGE_REGULAR /* stage and id will be overwrited */);
				pass->Load(pass_json);

				if (pass->ID() == startID)
				{
					m_StartPointPass = pass;
				}

				m_AllPasses[pass->ID()] = pass;
			}

			auto connections = jsonDoc->GetMember(msConnectionsKey);
			for (size_t i = 0, count = connections->Size(); i < count; ++i)
			{
				auto conn_json = connections->GetArrayElement(i);
				KPostProcessConnection* conn = new KPostProcessConnection(this /* id will be overwrited */);
				conn->Load(conn_json);

				m_AllConnections[conn->ID()] = conn;
			}

			return true;
		}
	}

	return false;
}

void KPostProcessManager::PopulateRenderCommand(KRenderCommand& command, IKPipelinePtr pipeline, IKRenderTargetPtr target)
{
	IKPipelineHandlePtr pipeHandle = nullptr;
	KRenderGlobal::PipelineManager.GetPipelineHandle(pipeline, target, pipeHandle, false);

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
	if (m_StartPointPass)
	{
		for (auto& pair : m_AllPasses)
		{
			KPostProcessPass* pass = pair.second;
			pass->UnInit();
		}

		for (auto& pair : m_AllConnections)
		{
			KPostProcessConnection* conn = pair.second;
			ASSERT_RESULT(conn->IsComplete());

			KPostProcessOutputData& outputData = conn->m_Output;
			KPostProcessInputData& inputData = conn->m_Input;

			if (outputData.type == POST_PROCESS_OUTPUT_PASS)
			{
				ASSERT_RESULT(outputData.pass->AddOutputConnection(conn, outputData.slot));
			}
			ASSERT_RESULT(inputData.pass->AddInputConnection(conn, inputData.slot));
		}

		IterPostProcessGraph([this](KPostProcessPass* pass)
		{
			pass->Init();
		});

		return true;
	}
	return false;
}

bool KPostProcessManager::Execute(unsigned int chainImageIndex, unsigned int frameIndex, IKSwapChainPtr& swapChain, IKUIOverlayPtr& ui, IKCommandBufferPtr primaryCommandBuffer)
{
	KPostProcessPass* endPass = nullptr;
	{
		IterPostProcessGraph([=, &endPass](KPostProcessPass* pass)
		{
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

IKPostProcessPass* KPostProcessManager::CreatePass(const char* vsFile, const char* fsFile, float scale, ElementFormat format)
{
	KPostProcessPass* pass = new KPostProcessPass(this, m_FrameInFlight, POST_PROCESS_STAGE_REGULAR);

	pass->SetFormat(format);
	pass->SetMSAA(0);
	pass->SetShader(vsFile, fsFile);
	pass->SetScale(scale);

	ASSERT_RESULT(m_AllPasses.insert(std::make_pair(pass->ID(), pass)).second);

	return pass;
}

void KPostProcessManager::DeletePass(IKPostProcessPass* pass)
{
	std::vector<KPostProcessConnection*> invalidConn;

	for(auto& pair : m_AllConnections)
	{
		KPostProcessConnection* conn = pair.second;
	
		ASSERT_RESULT(conn->IsComplete());

		KPostProcessOutputData& outputData = conn->m_Output;
		KPostProcessInputData& inputData = conn->m_Input;
		
		bool invalid = false;

		if (outputData.type == POST_PROCESS_OUTPUT_PASS)
		{
			if (outputData.pass == pass)
			{
				invalid = true;
			}
		}
		if (inputData.pass == pass)
		{
			invalid = true;
		}

		if (invalid)
		{
			invalidConn.push_back(conn);
		}
	}

	for(KPostProcessConnection* conn : invalidConn)
	{
		auto it = m_AllConnections.find(conn->ID());
		m_AllConnections.erase(it);
		SAFE_DELETE(conn); 
	}

	auto it = m_AllPasses.find(((KPostProcessPass*)pass)->ID());
	if(it != m_AllPasses.end())
	{
		m_AllPasses.erase(it);
		((KPostProcessPass*)pass)->UnInit();
		SAFE_DELETE(pass);
	}
}

KPostProcessPass* KPostProcessManager::GetPass(KPostProcessPass::IDType id)
{
	auto it = m_AllPasses.find(std::move(id));
	if (it != m_AllPasses.end())
	{
		return it->second;
	}
	return nullptr;
}

bool KPostProcessManager::GetAllPasses(KPostProcessPassSet& set)
{
	set.clear();
	for (auto pair : m_AllPasses)
	{
		set.insert(pair.second);
	}
	return true;
}

IKPostProcessConnection* KPostProcessManager::CreatePassConnection(IKPostProcessPass* outputPass, int16_t outSlot, IKPostProcessPass* inputPass, int16_t inSlot)
{
	KPostProcessConnection* conn = new KPostProcessConnection(this);

	conn->m_Output.InitAsPass((KPostProcessPass*)outputPass, outSlot);
	conn->m_Input.Init((KPostProcessPass*)inputPass, inSlot);

	ASSERT_RESULT(outputPass->AddOutputConnection(conn, outSlot));
	ASSERT_RESULT(inputPass->AddInputConnection(conn, inSlot));

	ASSERT_RESULT(m_AllConnections.insert(std::make_pair(conn->ID(), conn)).second);

	return conn;
}

IKPostProcessConnection* KPostProcessManager::CreateTextureConnection(IKTexturePtr outputTexure, int16_t outSlot, IKPostProcessPass* inputPass, int16_t inSlot)
{
	KPostProcessConnection* conn = new KPostProcessConnection(this);

	conn->m_Output.InitAsTexture(outputTexure, outSlot);
	conn->m_Input.Init((KPostProcessPass*)inputPass, inSlot);

	ASSERT_RESULT(inputPass->AddInputConnection(conn, inSlot));

	ASSERT_RESULT(m_AllConnections.insert(std::make_pair(conn->ID(), conn)).second);

	return conn;
}

void KPostProcessManager::DeleteConnection(IKPostProcessConnection* conn)
{
	auto it = m_AllConnections.find(((KPostProcessConnection*)conn)->ID());
	if(it != m_AllConnections.end())
	{
		KPostProcessOutputData& outputData = ((KPostProcessConnection*)conn)->m_Output;
		KPostProcessInputData& inputData = ((KPostProcessConnection*)conn)->m_Input;

		if (outputData.type == POST_PROCESS_OUTPUT_PASS)
		{
			ASSERT_RESULT(outputData.pass->RemoveOutputConnection(conn, outputData.slot));
		}
		ASSERT_RESULT(inputData.pass->RemoveInputConnection(conn, inputData.slot));

		m_AllConnections.erase(it);
		SAFE_DELETE(conn);
	}
}

KPostProcessConnection* KPostProcessManager::GetConnection(KPostProcessConnection::IDType id)
{
	auto it = m_AllConnections.find(std::move(id));
	if (it != m_AllConnections.end())
	{
		return it->second;
	}
	return nullptr;
}

IKPostProcessPass* KPostProcessManager::GetStartPointPass()
{
	return m_StartPointPass;
}