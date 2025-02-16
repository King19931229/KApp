#include "KPostProcessManager.h"
#include "KPostProcessPass.h"
#include "KPostProcessTexture.h"
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

const KVertexDefinition::SCREENQUAD_POS_2F KPostProcessManager::ms_Vertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KPostProcessManager::ms_Indices[] = {0, 1, 2, 2, 3, 0};

KPostProcessManager::KPostProcessManager()
	: m_StartPointPass(nullptr),
	m_FrameInFlight(0),
	m_Width(0),
	m_Height(0)
{
}

KPostProcessManager::~KPostProcessManager()
{
	assert(m_AllConnections.empty());
	assert(m_AllNodes.empty());
	assert(m_StartPointPass == nullptr);
}

bool KPostProcessManager::Init(size_t width, size_t height,
							   unsigned short massCount,
							   ElementFormat startFormat,
							   size_t frameInFlight)
{
	UnInit();

	m_FrameInFlight = frameInFlight;
	m_Width = width;
	m_Height = height;

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "postprocess/screenquad.vert", m_ScreenDrawVS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "postprocess/screenquad.frag", m_ScreenDrawFS, false));

	KRenderGlobal::RenderDevice->CreateSampler(m_Sampler);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->Init(0, 0);

	m_StartPointPass = IKPostProcessNodePtr(KNEW KPostProcessPass(this, frameInFlight, POST_PROCESS_STAGE_START_POINT));
	KPostProcessPass* pass = static_cast<KPostProcessPass*>(m_StartPointPass.get());
	pass->SetFormat(startFormat);
	pass->SetMSAA(massCount);
	pass->SetScale(1.0f);

	m_AllNodes[m_StartPointPass->ID()] = m_StartPointPass;

	return true;
}

bool KPostProcessManager::UnInit()
{
	m_FrameInFlight = 0;

	m_ScreenDrawVS.Release();
	m_ScreenDrawFS.Release();

	ClearDeletedPassConnection();
	ClearCreatedPassConnection();

	if(m_Sampler)
	{
		m_Sampler->UnInit();
		m_Sampler = nullptr;
	}

	return true;
}

void KPostProcessManager::ClearDeletedPassConnection()
{
	for (IKPostProcessConnectionPtr conn : m_DeletedConnections)
	{
		KPostProcessData& outputData = ((KPostProcessConnection*)conn.get())->m_Output;
		KPostProcessData& inputData = ((KPostProcessConnection*)conn.get())->m_Input;

		outputData.node->RemoveOutputConnection(conn.get(), outputData.slot);
		inputData.node->RemoveInputConnection(conn.get(), inputData.slot);
	}
	m_DeletedConnections.clear();

	for (IKPostProcessNodePtr node : m_DeletedNodes)
	{
		if (node->GetType() == PPNT_PASS)
		{
			KPostProcessPass* pass = (KPostProcessPass*)node.get();
			pass->UnInit();
		}
		else if (node->GetType() == PPNT_TEXTURE)
		{
			KPostProcessTexture* texture = (KPostProcessTexture*)node.get();
			texture->UnInit();
		}
		assert(m_StartPointPass != node);
	}
	m_DeletedNodes.clear();
}

void KPostProcessManager::ClearCreatedPassConnection()
{
	m_AllConnections.clear();

	for (auto& pair : m_AllNodes)
	{
		IKPostProcessNodePtr node = pair.second;

		if (node->GetType() == PPNT_PASS)
		{
			KPostProcessPass* pass = (KPostProcessPass*)node.get();
			pass->UnInit();
		}
		else if (node->GetType() == PPNT_TEXTURE)
		{
			KPostProcessTexture* texture = (KPostProcessTexture*)node.get();
			texture->UnInit();
		}
		else
		{
			assert(false && "impossile to reach");
		}

		if (node == m_StartPointPass)
		{
			m_StartPointPass = nullptr;
		}
	}
	m_AllNodes.clear();
}

void KPostProcessManager::GetAllParentNode(IKPostProcessNode* node, std::unordered_set<IKPostProcessNode*>& parents)
{
	parents.clear();

	for (auto i = 0; i < PostProcessPort::MAX_INPUT_SLOT_COUNT; ++i)
	{
		IKPostProcessConnection* conn = nullptr;
		if (node->GetInputConnection(conn, i) && conn)
		{
			parents.insert(conn->GetOutputPortNode());
		}
	}
}

void KPostProcessManager::IterPostProcessGraph(std::function<void(IKPostProcessNode*)> func)
{
	std::unordered_map<IKPostProcessNode*, bool> visitFlags;
	std::queue<IKPostProcessNode*> bfsQueue;

	bfsQueue.push(m_StartPointPass.get());

	for (auto& pair : m_AllNodes)
	{
		IKPostProcessNode* node = pair.second.get();
		visitFlags[node] = false;
	}

	while (!bfsQueue.empty())
	{
		IKPostProcessNode* node = bfsQueue.front();
		bfsQueue.pop();

		if (!visitFlags[node])
		{
			bool skipVisit = false;

			std::unordered_set<IKPostProcessNode*> parents;
			GetAllParentNode(node, parents);

			for (IKPostProcessNode* parent : parents)
			{
				if (!visitFlags[parent])
				{
					skipVisit = true;
					break;
				}
			}

			if (skipVisit)
			{
				continue;
			}

			visitFlags[node] = true;

			func(node);

			for (int16_t slot = 0; slot < PostProcessPort::MAX_OUTPUT_SLOT_COUNT; ++slot)
			{
				std::unordered_set<IKPostProcessConnection*> connections;
				if (node->GetOutputConnection(connections, slot))
				{
					for (auto conn : connections)
					{
						bfsQueue.push(((KPostProcessConnection*)conn)->m_Input.node);
					}
				}
			}
		}
	}
}

bool KPostProcessManager::Resize(size_t width, size_t height)
{
	m_Width = width;
	m_Height = height;

	Construct();

	return true;
}

const char* KPostProcessManager::msTypeKey = "type";
const char* KPostProcessManager::msIDKey = "id";
const char* KPostProcessManager::msStartPointKey = "start_point";
const char* KPostProcessManager::msNodesKey = "nodes";
const char* KPostProcessManager::msConnectionsKey = "connections";

bool KPostProcessManager::Save(const char* jsonFile)
{
	IKJsonDocumentPtr jsonDoc = GetJsonDocument();
	IKJsonValuePtr root = jsonDoc->GetRoot();

	root->AddMember(msStartPointKey, jsonDoc->CreateString(m_StartPointPass->ID().c_str()));

	auto nodes = jsonDoc->CreateArray();
	for (auto& pair : m_AllNodes)
	{
		IKPostProcessNodePtr node = pair.second;
		PostProcessNodeType nodeType = node->GetType();
		IKJsonValuePtr node_json = nullptr;

		if (nodeType == PPNT_PASS)
		{
			KPostProcessPass* pass = (KPostProcessPass*)node.get();
			ASSERT_RESULT(pass->Save(jsonDoc, node_json));
		}
		else
		{
			KPostProcessTexture* texture = (KPostProcessTexture*)node.get();
			ASSERT_RESULT(texture->Save(jsonDoc, node_json));
		}

		if (node_json)
		{
			node_json->AddMember(msIDKey, jsonDoc->CreateString(node->ID().c_str()));
			node_json->AddMember(msTypeKey, jsonDoc->CreateInt(node->GetType()));
			nodes->Push(node_json);
		}
	}
	root->AddMember(msNodesKey, nodes);

	auto connections = jsonDoc->CreateArray();
	for (auto& pair : m_AllConnections)
	{
		IKPostProcessConnectionPtr conn = pair.second;
		IKJsonValuePtr conn_json = nullptr;
		ASSERT_RESULT(((KPostProcessConnection*)conn.get())->Save(jsonDoc, conn_json));
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

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
	if (system && system->Open(jsonFile, IT_FILEHANDLE, fileStream))
	{
		if (jsonDoc->ParseFromDataStream(fileStream))
		{
			ClearDeletedPassConnection();
			ClearCreatedPassConnection();

			auto startID = jsonDoc->GetMember(msStartPointKey)->GetString();

			auto nodes = jsonDoc->GetMember(msNodesKey);
			for (size_t i = 0, count = nodes->Size(); i < count; ++i)
			{
				auto node_json = nodes->GetArrayElement(i);

				auto id = node_json->GetMember(msIDKey)->GetString();
				PostProcessNodeType type = (PostProcessNodeType)node_json->GetMember(msTypeKey)->GetInt();

				IKPostProcessNodePtr node = nullptr;

				if (type == PPNT_PASS)
				{
					node = IKPostProcessNodePtr(KNEW KPostProcessPass(this, m_FrameInFlight, POST_PROCESS_STAGE_REGULAR /* stage and will be overwrited */));
					KPostProcessPass* pass = (KPostProcessPass*)node.get();
					pass->Load(node_json);
					pass->m_ID = id;

					if (pass->ID() == startID)
					{
						m_StartPointPass = node;
					}
				}
				else
				{
					node = IKPostProcessNodePtr(KNEW KPostProcessTexture());
					KPostProcessTexture* texture = (KPostProcessTexture*)node.get();
					texture->Load(node_json);
					texture->m_ID = id;
				}

				if (node)
				{
					m_AllNodes[node->ID()] = node;
				}
			}

			auto connections = jsonDoc->GetMember(msConnectionsKey);
			for (size_t i = 0, count = connections->Size(); i < count; ++i)
			{
				auto conn_json = connections->GetArrayElement(i);
				IKPostProcessConnectionPtr conn = IKPostProcessConnectionPtr(KNEW KPostProcessConnection(this /* id will be overwrited */));
				((KPostProcessConnection*)conn.get())->Load(conn_json);

				m_AllConnections[conn->ID()] = conn;
			}

			return true;
		}
	}

	return false;
}

bool KPostProcessManager::PopulateRenderCommand(KRenderCommand& command, IKPipelinePtr pipeline, IKRenderPassPtr renderPass)
{
	IKPipelineHandlePtr pipeHandle = nullptr;
	if (pipeline->GetHandle(renderPass, pipeHandle))
	{
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		command.pipeline = pipeline;
		command.pipelineHandle = pipeHandle;

		return true;
	}
	return false;
}

bool KPostProcessManager::Construct()
{
	FLUSH_INFLIGHT_RENDER_JOB();

	ClearDeletedPassConnection();

	if (m_StartPointPass)
	{
		for (auto& pair : m_AllNodes)
		{
			IKPostProcessNodePtr node = pair.second;
			node->UnInit();	
		}

		for (auto& pair : m_AllConnections)
		{
			IKPostProcessConnectionPtr conn = pair.second;
			ASSERT_RESULT(conn->IsComplete());

			{
				KPostProcessData& outputData = ((KPostProcessConnection*)conn.get())->m_Output;
				IKPostProcessNode* outNode = outputData.node;
				outNode->AddOutputConnection(conn.get(), outputData.slot);
			}

			{
				KPostProcessData& inputData = ((KPostProcessConnection*)conn.get())->m_Input;
				IKPostProcessNode* inNode = inputData.node;
				inNode->AddInputConnection(conn.get(), inputData.slot);
			}
		}

		IterPostProcessGraph([this](IKPostProcessNode* node)
		{
			node->Init();
		});

		return true;
	}
	return false;
}

bool KPostProcessManager::Execute(unsigned int chainImageIndex, IKSwapChain* swapChain, IKUIOverlay* ui, KRHICommandList& commandList)
{
	KPostProcessPass* endPass = nullptr;

	KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };

	IterPostProcessGraph([=, &endPass, &commandList](IKPostProcessNode* node)
	{
		PostProcessNodeType type = node->GetType();
		if (type == PPNT_PASS)
		{
			KPostProcessPass* pass = (KPostProcessPass*)node;
			endPass = pass;

			if (pass == m_StartPointPass.get())
			{
				return;
			}

			IKRenderPassPtr renderPass = pass->GetRenderPass();

			commandList.BeginDebugMarker("PostProcess", glm::vec4(1));
			commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			{
				commandList.SetViewport(renderPass->GetViewPort());

				KRenderCommand command;
				if (PopulateRenderCommand(command, pass->GetPipeline(), renderPass))
				{
					commandList.Render(command);
				}
			}
			commandList.EndRenderPass();
			commandList.EndDebugMarker();

			commandList.Transition(pass->GetRenderTarget()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
	});

	if (endPass)
	{
		IKRenderPassPtr renderPass = swapChain->GetRenderPass(chainImageIndex);
		commandList.BeginDebugMarker("MainChain", glm::vec4(1));
		commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(renderPass->GetViewPort());
		KRenderCommand command;
		if (PopulateRenderCommand(command, endPass->GetScreenDrawPipeline(), renderPass))
		{
			commandList.Render(command);
		}
		if (ui)
		{
			ui->Draw(renderPass, commandList);
		}
		commandList.EndRenderPass();
		commandList.EndDebugMarker();
	}

	return true;
}

IKPostProcessNodePtr KPostProcessManager::CreatePass()
{
	IKPostProcessNodePtr pass = IKPostProcessNodePtr(KNEW KPostProcessPass(this, m_FrameInFlight, POST_PROCESS_STAGE_REGULAR));
	ASSERT_RESULT(m_AllNodes.insert(std::make_pair(pass->ID(), pass)).second);
	return pass;
}

IKPostProcessNodePtr KPostProcessManager::CreateTextrue()
{
	IKPostProcessNodePtr texture = IKPostProcessNodePtr(KNEW KPostProcessTexture());
	ASSERT_RESULT(m_AllNodes.insert(std::make_pair(texture->ID(), texture)).second);
	return texture;
}

void KPostProcessManager::DeleteNode(IKPostProcessNodePtr node)
{
	if (node == m_StartPointPass)
	{
		return;
	}

	auto itNode = m_AllNodes.find(node->ID());
	if (itNode != m_AllNodes.end())
	{
		for (auto itConn = m_AllConnections.begin(); itConn != m_AllConnections.end();)
		{
			IKPostProcessConnectionPtr conn = itConn->second;

			ASSERT_RESULT(conn->IsComplete());

			KPostProcessData& outputData = ((KPostProcessConnection*)conn.get())->m_Output;
			KPostProcessData& inputData = ((KPostProcessConnection*)conn.get())->m_Input;

			if (outputData.node == node.get() || inputData.node == node.get())
			{
				itConn = m_AllConnections.erase(itConn);
				m_DeletedConnections.insert(conn);
			}
			else
			{
				++itConn;
			}
		}

		m_AllNodes.erase(itNode);
		m_DeletedNodes.insert(node);
	}
}

IKPostProcessNodePtr KPostProcessManager::GetNode(IKPostProcessNode::IDType id)
{
	auto it = m_AllNodes.find(std::move(id));
	if (it != m_AllNodes.end())
	{
		return it->second;
	}
	return nullptr;
}

bool KPostProcessManager::GetAllNodes(KPostProcessNodeSet& set)
{
	set.clear();
	for (auto pair : m_AllNodes)
	{
		set.insert(pair.second);
	}
	return true;
}

IKPostProcessConnectionPtr KPostProcessManager::CreateConnection(IKPostProcessNodePtr outNode, int16_t outSlot, IKPostProcessNodePtr inNode, int16_t inSlot)
{
	IKPostProcessConnectionPtr conn = IKPostProcessConnectionPtr(KNEW KPostProcessConnection(this));

	conn->SetInputPort(inNode.get(), inSlot);
	conn->SetOutputPort(outNode.get(), outSlot);

	ASSERT_RESULT(m_AllConnections.insert(std::make_pair(conn->ID(), conn)).second);

	return conn;
}

IKPostProcessConnectionPtr KPostProcessManager::FindConnection(IKPostProcessNodePtr outputNode, int16_t outSlot, IKPostProcessNodePtr inputNode, int16_t inSlot)
{
	auto it = std::find_if(m_AllConnections.begin(), m_AllConnections.end(), [&, this](auto pair)->bool
	{
		IKPostProcessConnectionPtr conn = pair.second;
		if (conn->IsComplete())
		{
			if (conn->GetInputPortNode() == inputNode.get() &&
				conn->GetOutputPortNode() == outputNode.get() &&
				conn->GetInputSlot() == inSlot &&
				conn->GetOutputSlot() == outSlot)
			{
				return true;
			}
		}
		return false;
	});

	if (it != m_AllConnections.end())
	{
		return it->second;
	}

	return nullptr;
}


void KPostProcessManager::DeleteConnection(IKPostProcessConnectionPtr conn)
{
	auto it = m_AllConnections.find(conn->ID());
	if(it != m_AllConnections.end())
	{
		m_DeletedConnections.insert(conn);
		m_AllConnections.erase(it);
	}
}

bool KPostProcessManager::GetAllConnections(KPostProcessConnectionSet& set)
{
	set.clear();
	for (auto pair : m_AllConnections)
	{
		set.insert(pair.second);
	}
	return true;
}

IKPostProcessConnectionPtr KPostProcessManager::GetConnection(KPostProcessConnection::IDType id)
{
	auto it = m_AllConnections.find(std::move(id));
	if (it != m_AllConnections.end())
	{
		return it->second;
	}
	return nullptr;
}

IKPostProcessNodePtr KPostProcessManager::GetStartPointPass()
{
	return m_StartPointPass;
}