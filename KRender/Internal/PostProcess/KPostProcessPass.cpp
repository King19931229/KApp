#include "KPostProcessPass.h"
#include "KPostProcessManager.h"
#include "KPostProcessConnection.h"

#include "Internal/KRenderGlobal.h"

#include <assert.h>

KPostProcessPass::KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage)
	: m_Mgr(manager),
	m_Scale(1),
	m_MsaaCount(1),
	m_FrameInFlight(frameInFlight),
	m_Format(EF_R8GB8BA8_UNORM),
	m_Stage(stage),
	m_bInit(false)
{
}

KPostProcessPass::~KPostProcessPass()
{
	assert(m_VSShader == nullptr);
	assert(m_FSShader == nullptr);
	assert(m_Textures.empty());
	for (KPostProcessConnection*& conn : m_InputConnection)
	{
		assert(!conn);
	}
	for (ConnectionSet& connSet : m_OutputConnection)
	{
		assert(connSet.empty());
	}
	assert(m_RenderTargets.empty());
	assert(m_CommandBuffers.empty());
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

bool KPostProcessPass::Init()
{
	IKRenderDevice* device = m_Mgr->GetDevice();

	if(m_Stage == POST_PROCESS_STAGE_START_POINT)
	{
		for (KPostProcessConnection*& input : m_InputConnection)
		{
			ASSERT_RESULT(input == nullptr);
		}
	}
	else
	{
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(m_VSFile.c_str(), m_VSShader, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(m_FSFile.c_str(), m_FSShader, false));

		m_CommandBuffers.resize(m_FrameInFlight);
		for(size_t i = 0; i < m_CommandBuffers.size(); ++i)
		{
			device->CreateCommandBuffer(m_CommandBuffers[i]);
			m_CommandBuffers[i]->Init(m_Mgr->m_CommandPool, CBL_SECONDARY);
		}

		m_Pipelines.resize(m_FrameInFlight);
		for(size_t i = 0; i < m_Pipelines.size(); ++i)
		{
			KRenderGlobal::PipelineManager.CreatePipeline(m_Pipelines[i]);

			IKPipelinePtr pipeline = m_Pipelines[i];

			VertexFormat formats[] = { VF_SCREENQUAD_POS };

			pipeline->SetVertexBinding(formats, 1);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetShader(ST_VERTEX, m_VSShader);
			pipeline->SetShader(ST_FRAGMENT, m_FSShader);

			pipeline->SetBlendEnable(false);

			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);

			for (int16_t slot = 0; slot < MAX_INPUT_SLOT_COUNT; ++slot)
			{
				KPostProcessConnection*& input = m_InputConnection[slot];
				if (input)
				{
					ASSERT_RESULT(input->IsComplete());
					ASSERT_RESULT(input->m_Input.pass == this);

					KPostProcessOutputData& outputData = input->m_Output;
					KPostProcessInputData& inputData = input->m_Input;

					KPostProcessPass* outputPass = outputData.pass;
					IKTexturePtr outputTexture = outputData.texture;
					size_t location = inputData.slot;
					IKSamplerPtr sampler = m_Mgr->m_Sampler;

					if (outputData.type == POST_PROCESS_OUTPUT_TEXTURE)
					{
						pipeline->SetSampler((unsigned int)location, outputTexture, sampler);
					}
					else if (outputData.type == POST_PROCESS_OUTPUT_PASS)
					{
						pipeline->SetSampler((unsigned int)location, outputPass->GetTexture(i), sampler);
					}
					else
					{
						assert(false && "impossible to reach");
					}
				}
			}

			pipeline->Init(false);
		}
	}

	size_t width = static_cast<size_t>(m_Mgr->m_Width * m_Scale);
	size_t height = static_cast<size_t>(m_Mgr->m_Height * m_Scale);

	m_Textures.resize(m_FrameInFlight);
	for(size_t i = 0; i < m_Textures.size(); ++i)
	{
		device->CreateTexture(m_Textures[i]);
		m_Textures[i]->InitMemeoryAsRT(width, height, m_Format);
		m_Textures[i]->InitDevice(false);
	}

	m_RenderTargets.resize(m_FrameInFlight);
	for(size_t i = 0; i < m_RenderTargets.size(); ++i)
	{
		device->CreateRenderTarget(m_RenderTargets[i]);
		m_RenderTargets[i]->SetColorClear(0, 0, 0, 1);
		m_RenderTargets[i]->SetDepthStencilClear(1.0, 0);
		m_RenderTargets[i]->InitFromTexture(m_Textures[i].get(), true, true, m_MsaaCount);
	}

	m_ScreenDrawPipelines.resize(m_FrameInFlight);
	for(size_t i = 0; i < m_ScreenDrawPipelines.size(); ++i)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(m_ScreenDrawPipelines[i]);

		IKPipelinePtr pipeline = m_ScreenDrawPipelines[i];

		VertexFormat formats[] = {VF_SCREENQUAD_POS};

		pipeline->SetVertexBinding(formats, 1);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, m_Mgr->m_ScreenDrawVS);
		pipeline->SetShader(ST_FRAGMENT, m_Mgr->m_ScreenDrawFS);

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetSampler(0, m_Textures[i], m_Mgr->m_Sampler);

		pipeline->Init(false);
	}

	m_bInit = true;

	return true;
}

bool KPostProcessPass::UnInit()
{
	if(m_VSShader)
	{
		KRenderGlobal::ShaderManager.Release(m_VSShader);
		m_VSShader = nullptr;
	}

	if(m_FSShader)
	{
		KRenderGlobal::ShaderManager.Release(m_FSShader);
		m_FSShader = nullptr;
	}

	for (KPostProcessConnection*& input : m_InputConnection)
	{
		input = nullptr;
	}

	for (ConnectionSet& connSet : m_OutputConnection)
	{
		connSet.clear();
	}

	for(IKTexturePtr& texture : m_Textures)
	{
		texture->UnInit();
		texture = nullptr;
	}
	m_Textures.clear();

	for(IKRenderTargetPtr& target : m_RenderTargets)
	{
		target->UnInit();
		target = nullptr;
	}
	m_RenderTargets.clear();

	for(IKPipelinePtr& pipeline : m_Pipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_Pipelines.clear();

	for(IKPipelinePtr& pipeline : m_ScreenDrawPipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_ScreenDrawPipelines.clear();

	for(IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		buffer->UnInit();
		buffer = nullptr;
	}
	m_CommandBuffers.clear();

	m_bInit = false;

	return true;
}

bool KPostProcessPass::AddInputConnection(KPostProcessConnection* conn, int16_t slot)
{
	if (slot < MAX_INPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		m_InputConnection[slot] = conn;
		return true;
	}
	return false;
}

bool KPostProcessPass::AddOutputConnection(KPostProcessConnection* conn, int16_t slot)
{
	if (slot < MAX_OUTPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		m_OutputConnection[slot].insert(conn);
		return true;
	}
	return false;
}

bool KPostProcessPass::RemoveInputConnection(KPostProcessConnection* conn, int16_t slot)
{
	if (slot < MAX_INPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		if (m_InputConnection[slot] == conn)
		{
			m_InputConnection[slot] = nullptr;
		}
	}
	return false;
}

bool KPostProcessPass::RemoveOutputConnection(KPostProcessConnection* conn, int16_t slot)
{
	if (slot < MAX_OUTPUT_SLOT_COUNT && slot >= 0 && conn)
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