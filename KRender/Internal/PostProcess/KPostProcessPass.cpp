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
	assert(m_Inputs.empty());
	assert(m_Outputs.empty());
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
		ASSERT_RESULT(m_Inputs.empty());
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

			VertexFormat formats[] = {VF_SCREENQUAD_POS};

			pipeline->SetVertexBinding(formats, 1);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetShader(ST_VERTEX, m_VSShader);
			pipeline->SetShader(ST_FRAGMENT, m_FSShader);

			pipeline->SetBlendEnable(false);

			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);

			for(KPostProcessConnection* conn : m_Inputs)
			{
				KPostProcessPass* inputPass		= conn->m_InputPass;
				IKTexturePtr inputTexture		= conn->m_InputTexture;
				size_t location					= conn->m_InputSlot;
				KPostProcessPass* outputPass	= conn->m_OutputPass;
				IKSamplerPtr sampler			= m_Mgr->m_Sampler;

				ASSERT_RESULT(outputPass == this);

				if(conn->m_InputType == POST_PROCESS_INPUT_TEXTURE)
				{
					pipeline->SetSampler((unsigned int)location, inputTexture, sampler);
				}
				else
				{
					pipeline->SetSampler((unsigned int)location, inputPass->GetTexture(i), sampler);
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

	m_Inputs.clear();
	m_Outputs.clear();

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

bool KPostProcessPass::AddInput(KPostProcessConnection* conn)
{
	if(conn)
	{
		m_Inputs.insert(conn);
		return true;
	}
	return false;
}


bool KPostProcessPass::AddOutput(KPostProcessPass* pass)
{
	if(pass)
	{
		m_Outputs.insert(pass);
		return true;
	}
	return false;
}