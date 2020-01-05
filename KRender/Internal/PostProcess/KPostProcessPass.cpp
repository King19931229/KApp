#include "KPostProcessPass.h"
#include "KPostProcessManager.h"

#include "Internal/KRenderGlobal.h"

#include <assert.h>

KPostProcessPass::KPostProcessPass(KPostProcessManager* manager)
	: m_Mgr(manager),
	m_Width(0),
	m_Height(0),
	m_MsaaCount(1),
	m_FrameInFlight(0),
	m_Format(EF_R8GB8BA8_UNORM),
	m_bIsStartPoint(false),
	m_bInit(false)
{
}

KPostProcessPass::~KPostProcessPass()
{
	assert(m_VSShader == nullptr);
	assert(m_FSShader == nullptr);
	assert(m_Textures.empty());
	assert(m_RenderTargets.empty());
	assert(m_CommandBuffers.empty());
}

bool KPostProcessPass::SetShader(const char* vsFile, const char* fsFile)
{
	m_VSFile = vsFile;
	m_FSFile = fsFile;
	return true;
}

bool KPostProcessPass::SetSize(size_t width, size_t height)
{
	m_Width = width;
	m_Height = height;
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

bool KPostProcessPass::ConnectInput(KPostProcessPass* input, uint16_t slot)
{
	if(input)
	{
		if(std::find_if(m_InputConnections.begin(), m_InputConnections.end(),
			[=](const PassConnection& element) { return element.slot == slot; }) != m_InputConnections.end())
		{
			assert(false && "input slot conflit");
			return false;
		}

		PassConnection inputSlot = {input, slot};
		PassConnection ouputSlot = {this, slot};

		this->m_InputConnections.push_back(inputSlot);
		input->m_OutputConnections.push_back(ouputSlot);

		return true;
	}

	return false;
}

bool KPostProcessPass::DisconnectInput(const PassConnection& connection)
{
	auto it = std::find(m_InputConnections.begin(), m_InputConnections.end(), connection);
	if(it != m_InputConnections.end())
	{
		KPostProcessPass* input = it->pass;

		PassConnection outputConnection = { this, connection.slot };
		input->DisconnectOutput(outputConnection);

		m_InputConnections.erase(it);
	}

	return true;
}

bool KPostProcessPass::DisconnectOutput(const PassConnection& connection)
{
	auto it = std::find(m_OutputConnections.begin(), m_OutputConnections.end(), connection);
	if(it != m_OutputConnections.end())
	{
		KPostProcessPass* output = it->pass;

		PassConnection inputConnection = { this, connection.slot };
		output->DisconnectInput(inputConnection);

		m_OutputConnections.erase(it);
	}

	return true;
}

bool KPostProcessPass::DisconnectAll()
{
	std::vector<PassConnection> inputs = m_InputConnections;
	std::vector<PassConnection> outputs = m_OutputConnections;

	for(const PassConnection& connection : inputs)
	{
		DisconnectInput(connection);
	}

	for(const PassConnection& connection : outputs)
	{
		DisconnectOutput(connection);
	}

	return true;
}

bool KPostProcessPass::Init(size_t frameInFlight, bool isStartPoint)
{
	ASSERT_RESULT(UnInit());

	m_FrameInFlight = frameInFlight;
	m_bIsStartPoint = isStartPoint;

	IKRenderDevice* device = m_Mgr->GetDevice();

	if(m_bIsStartPoint)
	{
		ASSERT_RESULT(m_InputConnections.empty());
	}
	else
	{
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(m_VSFile.c_str(), m_VSShader));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(m_FSFile.c_str(), m_FSShader));

		m_CommandBuffers.resize(m_FrameInFlight);
		for(size_t i = 0; i < m_Pipelines.size(); ++i)
		{
			device->CreateCommandBuffer(m_CommandBuffers[i]);
			m_CommandBuffers[i]->Init(m_Mgr->GetCommandPool().get(), CBL_SECONDARY);
		}

		m_Pipelines.resize(m_FrameInFlight);
		for(size_t i = 0; i < m_Pipelines.size(); ++i)
		{
			device->CreatePipeline(m_Pipelines[i]);

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

			for(const PassConnection& connection : m_InputConnections)
			{
				KPostProcessPass* inputPass = connection.pass;
				uint16_t location = connection.slot;

				IKTexturePtr inputTexture = inputPass->GetTexture(i);
				IKSamplerPtr sampler = inputPass->GetSampler();

				ASSERT_RESULT(inputTexture != nullptr && sampler != nullptr);

				pipeline->SetSampler(location, inputTexture, sampler);
			}
			pipeline->Init();
		}
	}

	m_Textures.resize(m_FrameInFlight);
	for(size_t i = 0; i < m_Textures.size(); ++i)
	{
		device->CreateTexture(m_Textures[i]);
		m_Textures[i]->InitMemeoryAsRT(m_Width, m_Height, m_Format);
		m_Textures[i]->InitDevice();
	}

	m_RenderTargets.resize(m_FrameInFlight);
	for(size_t i = 0; i < m_RenderTargets.size(); ++i)
	{
		device->CreateRenderTarget(m_RenderTargets[i]);
		m_RenderTargets[i]->SetColorClear(0, 0, 0, 1);
		m_RenderTargets[i]->SetDepthStencilClear(1.0, 0);
		m_RenderTargets[i]->InitFromTexture(m_Textures[i].get(), true, true, m_MsaaCount);
	}

	device->CreateSampler(m_Sampler);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->SetMipmapLod(0, 0);
	m_Sampler->Init();

	m_bInit = true;

	return true;
}

bool KPostProcessPass::UnInit()
{
	if(m_VSShader)
	{
		m_VSShader->UnInit();
		m_VSShader = nullptr;
	}
	if(m_FSShader)
	{
		m_FSShader->UnInit();
		m_FSShader = nullptr;
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
		pipeline->UnInit();
		pipeline = nullptr;
	}
	m_Pipelines.clear();

	for(IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		buffer->UnInit();
		buffer = nullptr;
	}
	m_CommandBuffers.clear();

	if(m_Sampler)
	{
		m_Sampler->UnInit();
		m_Sampler = nullptr;
	}

	m_bInit = false;

	return true;
}