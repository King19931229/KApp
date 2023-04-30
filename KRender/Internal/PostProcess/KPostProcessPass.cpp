#include "KPostProcessPass.h"
#include "KPostProcessTexture.h"
#include "KPostProcessManager.h"
#include "KPostProcessConnection.h"

#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KStringParser.h"

KPostProcessPass::KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage)
	: m_Mgr(manager),
	m_Scale(1),
	m_MsaaCount(1),
	m_FrameInFlight(frameInFlight),
	m_Format(EF_R8G8B8A8_UNORM),
	m_Stage(stage),
	m_bInit(false)
{
	// TODO
	char szTempBuffer[256] = { 0 };
	size_t address = (size_t)this;
	ASSERT_RESULT(KStringParser::ParseFromSIZE_T(szTempBuffer, ARRAY_SIZE(szTempBuffer), &address, 1));
	m_ID = szTempBuffer;

	ZERO_ARRAY_MEMORY(m_InputConnection);
}

KPostProcessPass::KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage, IDType id)
	: m_Mgr(manager),
	m_Scale(1),
	m_MsaaCount(1),
	m_FrameInFlight(frameInFlight),
	m_Format(EF_R8G8B8A8_UNORM),
	m_Stage(stage),
	m_bInit(false),
	m_ID(id)
{
	ZERO_ARRAY_MEMORY(m_InputConnection);
}

KPostProcessPass::~KPostProcessPass()
{
	assert(!m_VSShader);
	assert(!m_FSShader);
	assert(m_RenderTarget == nullptr);
	assert(m_CommandBuffer == nullptr);

	for (auto& conn : m_InputConnection)
	{
		assert(!conn);
	}
	for (auto& connSet : m_OutputConnection)
	{
		assert(connSet.empty());
	}
}

KPostProcessPass::IDType KPostProcessPass::ID()
{
	return m_ID;
}

bool KPostProcessPass::SetShader(const char* vsFile, const char* fsFile)
{
	m_VSFile = vsFile;
	m_FSFile = fsFile;
	return true;
}

bool KPostProcessPass::SetScale(float scale)
{
	m_Scale = scale;
	return true;
}

bool KPostProcessPass::SetFormat(ElementFormat format)
{
	m_Format = format;
	return true;
}

bool KPostProcessPass::SetMSAA(unsigned short msaaCount)
{
	m_MsaaCount = msaaCount;
	return true;
}

std::tuple<std::string, std::string> KPostProcessPass::GetShader()
{
	return std::make_tuple(m_VSFile, m_FSFile);
}

float KPostProcessPass::GetScale()
{
	return m_Scale;
}

ElementFormat KPostProcessPass::GetFormat()
{
	return m_Format;
}

unsigned short KPostProcessPass::GetMSAA()
{
	return m_MsaaCount;
}

bool KPostProcessPass::Init()
{
	if(m_Stage == POST_PROCESS_STAGE_START_POINT)
	{
		for (auto& input : m_InputConnection)
		{
			ASSERT_RESULT(input == nullptr);
		}
	}
	else
	{
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, m_VSFile.c_str(), m_VSShader, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, m_FSFile.c_str(), m_FSShader, false));

		{
			KRenderGlobal::RenderDevice->CreatePipeline(m_Pipeline);

			IKPipelinePtr& pipeline = m_Pipeline;

			VertexFormat formats[] = { VF_SCREENQUAD_POS };

			pipeline->SetVertexBinding(formats, 1);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetShader(ST_VERTEX, *m_VSShader);
			pipeline->SetShader(ST_FRAGMENT, *m_FSShader);

			pipeline->SetBlendEnable(false);

			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);

			for (int16_t slot = 0; slot < PostProcessPort::MAX_INPUT_SLOT_COUNT; ++slot)
			{
				auto& input = m_InputConnection[slot];
				if (input)
				{
					ASSERT_RESULT(input->IsComplete());

					KPostProcessData& outputData = ((KPostProcessConnection*)input)->m_Output;
					KPostProcessData& inputData = ((KPostProcessConnection*)input)->m_Input;

					IKPostProcessNode* outputNode = outputData.node;
					PostProcessNodeType outputType = outputNode->GetType();

					assert(inputData.node == this);
					assert(inputData.slot == slot);

					size_t location = inputData.slot;
					IKSamplerPtr sampler = m_Mgr->m_Sampler;

					if (outputType == PPNT_TEXTURE)
					{
						KPostProcessTexture* outTexture = (KPostProcessTexture*)outputNode;
						pipeline->SetSampler((unsigned int)location, outTexture->GetTexture()->GetFrameBuffer(), sampler);
					}
					else if (outputType == PPNT_PASS)
					{
						KPostProcessPass* outPass = (KPostProcessPass*)outputNode;
						pipeline->SetSampler((unsigned int)location, outPass->GetRenderTarget()->GetFrameBuffer(), sampler);
					}
					else
					{
						assert(false && "impossible to reach");
					}
				}
			}

			pipeline->Init();
		}
	}

	{
		uint32_t width = static_cast<uint32_t>(m_Mgr->m_Width * m_Scale);
		uint32_t height = static_cast<uint32_t>(m_Mgr->m_Height * m_Scale);

		KRenderGlobal::RenderDevice->CreateRenderPass(m_RenderPass);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_RenderTarget);

		m_RenderTarget->InitFromColor(width, height, m_MsaaCount, 1, m_Format);
		m_RenderPass->SetColorAttachment(0, m_RenderTarget->GetFrameBuffer());
		m_RenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });

		if (m_Stage == POST_PROCESS_STAGE_START_POINT)
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_DepthStencilTarget);
			m_DepthStencilTarget->InitFromDepthStencil(width, height, m_MsaaCount, true);
			m_RenderPass->SetDepthStencilAttachment(m_DepthStencilTarget->GetFrameBuffer());
			m_RenderPass->SetClearDepthStencil({ 1.0f, 0 });
		}

		if (m_Stage == POST_PROCESS_STAGE_START_POINT)
		{
			m_RenderTarget->GetFrameBuffer()->SetDebugName("PostProcessStartPointColor");
			m_DepthStencilTarget->GetFrameBuffer()->SetDebugName("PostProcessStartPointDepthStencil");
			m_RenderPass->SetDebugName("PostProcessStartPointPass");
		}
		else if (m_Stage == POST_PROCESS_STAGE_END_POINT)
		{
			m_RenderTarget->GetFrameBuffer()->SetDebugName("PostProcessEndPointColor");
			m_RenderPass->SetDebugName("PostProcessEndPointPass");
		}
		else if(m_Stage == POST_PROCESS_STAGE_REGULAR)
		{
			m_RenderTarget->GetFrameBuffer()->SetDebugName(("PostProcessRegularColor" + m_ID).c_str());
			m_RenderPass->SetDebugName(("PostProcessRegularPass" + m_ID).c_str());
		}

		m_RenderPass->Init();
	}

	{
		KRenderGlobal::RenderDevice->CreatePipeline(m_ScreenDrawPipeline);

		IKPipelinePtr& pipeline = m_ScreenDrawPipeline;

		VertexFormat formats[] = { VF_SCREENQUAD_POS };

		pipeline->SetVertexBinding(formats, 1);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, *m_Mgr->m_ScreenDrawVS);
		pipeline->SetShader(ST_FRAGMENT, *m_Mgr->m_ScreenDrawFS);

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_RenderTarget->GetFrameBuffer(), m_Mgr->m_Sampler);

		pipeline->Init();

		pipeline->SetDebugName("ScreenDraw");
	}

	m_bInit = true;

	return true;
}

bool KPostProcessPass::UnInit()
{
	m_VSShader.Release();
	m_FSShader.Release();

	for (auto& input : m_InputConnection)
	{
		input = nullptr;
	}

	for (auto& connSet : m_OutputConnection)
	{
		connSet.clear();
	}

	SAFE_UNINIT(m_RenderTarget);
	SAFE_UNINIT(m_DepthStencilTarget);
	SAFE_UNINIT(m_RenderPass);
	SAFE_UNINIT(m_Pipeline);
	SAFE_UNINIT(m_ScreenDrawPipeline);

	m_bInit = false;

	return true;
}

const char* KPostProcessPass::msStageKey = "stage";
const char* KPostProcessPass::msScaleKey = "scale";
const char* KPostProcessPass::msFormatKey = "format";
const char* KPostProcessPass::msMSAAKey = "msaa";
const char* KPostProcessPass::msVSKey = "vs";
const char* KPostProcessPass::msFSKey = "fs";

bool KPostProcessPass::Save(IKJsonDocumentPtr jsonDoc, IKJsonValuePtr& object)
{
	object = jsonDoc->CreateObject();

	object->AddMember(msStageKey, jsonDoc->CreateInt(m_Stage));
	object->AddMember(msScaleKey, jsonDoc->CreateFloat(m_Scale));
	object->AddMember(msFormatKey, jsonDoc->CreateInt(m_Format));
	object->AddMember(msMSAAKey, jsonDoc->CreateInt(m_MsaaCount));
	object->AddMember(msVSKey, jsonDoc->CreateString(m_VSFile.c_str()));
	object->AddMember(msFSKey, jsonDoc->CreateString(m_FSFile.c_str()));

	return true;
}

bool KPostProcessPass::Load(IKJsonValuePtr& object)
{
	if (object->IsObject())
	{
		m_Stage = (PostProcessStage)object->GetMember(msStageKey)->GetInt();

		m_Scale = object->GetMember(msScaleKey)->GetFloat();
		m_Format = (ElementFormat)object->GetMember(msFormatKey)->GetInt();
		m_MsaaCount = object->GetMember(msMSAAKey)->GetInt();
		m_VSFile = object->GetMember(msVSKey)->GetString();
		m_FSFile = object->GetMember(msFSKey)->GetString();

		return true;
	}
	else
	{
		return false;
	}
}

bool KPostProcessPass::AddInputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	if (slot < PostProcessPort::MAX_INPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		m_InputConnection[slot] = conn;
		return true;
	}
	return false;
}

bool KPostProcessPass::AddOutputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	if (slot < PostProcessPort::MAX_OUTPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		m_OutputConnection[slot].insert(conn);
		return true;
	}
	return false;
}

bool KPostProcessPass::RemoveInputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	if (slot < PostProcessPort::MAX_INPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		if (m_InputConnection[slot] == conn)
		{
			m_InputConnection[slot] = nullptr;
			return true;
		}
	}
	return false;
}

bool KPostProcessPass::RemoveOutputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	if (slot < PostProcessPort::MAX_OUTPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		auto it = m_OutputConnection[slot].find(conn);
		if (it != m_OutputConnection[slot].end())
		{
			m_OutputConnection[slot].erase(it);
			return true;
		}
	}
	return false;
}

bool KPostProcessPass::GetOutputConnection(std::unordered_set<IKPostProcessConnection*>& set, int16_t slot)
{
	if (slot < PostProcessPort::MAX_OUTPUT_SLOT_COUNT && slot >= 0)
	{
		set = m_OutputConnection[slot];
		return true;
	}
	return false;
}

bool KPostProcessPass::GetInputConnection(IKPostProcessConnection*& conn, int16_t slot)
{
	if (slot < PostProcessPort::MAX_INPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		conn = m_InputConnection[slot];
		return true;
	}
	return false;
}