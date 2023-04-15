#include "KHiZBuffer.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KMath.h"

KHiZBuffer::KHiZBuffer()
	: m_NumMips(0)
	, m_HiZWidth(0)
	, m_HiZHeight(0)
{}

KHiZBuffer::~KHiZBuffer()
{
}

void KHiZBuffer::InitializePipeline()
{
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);

	m_ReadDepthPipeline->UnInit();

	m_ReadDepthPipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	m_ReadDepthPipeline->SetShader(ST_VERTEX, *m_QuadVS);
	m_ReadDepthPipeline->SetShader(ST_FRAGMENT, *m_ReadDepthFS);

	m_ReadDepthPipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	m_ReadDepthPipeline->SetBlendEnable(false);
	m_ReadDepthPipeline->SetCullMode(CM_NONE);
	m_ReadDepthPipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	m_ReadDepthPipeline->SetPolygonMode(PM_FILL);
	m_ReadDepthPipeline->SetColorWrite(true, true, true, true);
	m_ReadDepthPipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

	m_ReadDepthPipeline->SetSampler(SHADER_BINDING_TEXTURE0,
		KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer(),
		*m_ReadDepthSampler,
		true);

	m_ReadDepthPipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);
	m_ReadDepthPipeline->Init();

	SAFE_UNINIT_CONTAINER(m_BuildHiZMinPipelines);
	SAFE_UNINIT_CONTAINER(m_BuildHiZMaxPipelines);

	m_BuildHiZMinPipelines.resize(m_NumMips - 1);
	m_BuildHiZMaxPipelines.resize(m_NumMips - 1);

	for (uint32_t i = 0; i < m_NumMips - 1; ++i)
	{
		for (bool buildMin : {true, false})
		{
			IKPipelinePtr& pipeline = buildMin ? m_BuildHiZMinPipelines[i] : m_BuildHiZMaxPipelines[i];

			KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

			pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
			pipeline->SetShader(ST_VERTEX, *m_QuadVS);
			pipeline->SetShader(ST_FRAGMENT, *m_BuildHiZFS);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			if (buildMin)
			{
				pipeline->SetSamplerMipmap(SHADER_BINDING_TEXTURE0,
					m_HiZMinBuffer->GetFrameBuffer(),
					*m_HiZBuildSampler,
					i, 1,
					true);
			}
			else
			{
				pipeline->SetSamplerMipmap(SHADER_BINDING_TEXTURE0,
					m_HiZMaxBuffer->GetFrameBuffer(),
					*m_HiZBuildSampler,
					i, 1,
					true);
			}

			pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);
			pipeline->Init();
		}
	}
}

bool KHiZBuffer::Resize(uint32_t width, uint32_t height)
{
	const uint32_t DIMENSION_DIV = 1;

	m_HiZWidth = KMath::BiggestPowerOf2LessEqualThan(width / DIMENSION_DIV);
	m_HiZHeight = KMath::BiggestPowerOf2LessEqualThan(height / DIMENSION_DIV);

	m_NumMips = (uint32_t)std::log2(std::max(m_HiZWidth, m_HiZHeight)) + 1;

	m_HiZMinBuffer->UnInit();
	m_HiZMinBuffer->InitFromColor(m_HiZWidth, m_HiZHeight, 1, m_NumMips, EF_R32_FLOAT);
	m_HiZMaxBuffer->UnInit();
	m_HiZMaxBuffer->InitFromColor(m_HiZWidth, m_HiZHeight, 1, m_NumMips, EF_R32_FLOAT);

	IKCommandBufferPtr primaryBuffer = KRenderGlobal::CommandPool->Request(CBL_PRIMARY);

	primaryBuffer->BeginPrimary();
	primaryBuffer->Translate(m_HiZMinBuffer->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	primaryBuffer->Translate(m_HiZMaxBuffer->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	primaryBuffer->End();
	primaryBuffer->Flush();

	SAFE_UNINIT_CONTAINER(m_HiZMinRenderPass);
	SAFE_UNINIT_CONTAINER(m_HiZMaxRenderPass);

	m_HiZMinRenderPass.resize(m_NumMips);
	m_HiZMaxRenderPass.resize(m_NumMips);

	for (uint32_t mipmap = 0; mipmap < m_NumMips; ++mipmap)
	{
		KRenderGlobal::RenderDevice->CreateRenderPass(m_HiZMinRenderPass[mipmap]);
		m_HiZMinRenderPass[mipmap]->SetColorAttachment(0, m_HiZMinBuffer->GetFrameBuffer());
		m_HiZMinRenderPass[mipmap]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_HiZMinRenderPass[mipmap]->SetOpColor(0, LO_DONT_CARE, SO_STORE);
		m_HiZMinRenderPass[mipmap]->Init(mipmap);

		KRenderGlobal::RenderDevice->CreateRenderPass(m_HiZMaxRenderPass[mipmap]);
		m_HiZMaxRenderPass[mipmap]->SetColorAttachment(0, m_HiZMaxBuffer->GetFrameBuffer());
		m_HiZMaxRenderPass[mipmap]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_HiZMaxRenderPass[mipmap]->SetOpColor(0, LO_DONT_CARE, SO_STORE);
		m_HiZMaxRenderPass[mipmap]->Init(mipmap);
	}

	m_HiZSampler.Release();

	KSamplerDescription desc;
	desc.minFilter = desc.magFilter = FM_NEAREST;
	desc.addressU = desc.addressV = desc.addressW = AM_CLAMP_TO_EDGE;
	desc.minMipmap = 0;
	desc.maxMipmap = m_NumMips - 1;
	KRenderGlobal::SamplerManager.Acquire(desc, m_HiZSampler);

	InitializePipeline();

	return true;
}

bool KHiZBuffer::Init(uint32_t width, uint32_t height)
{
	UnInit();

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/hiz_read.frag", m_ReadDepthFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/hiz_build.frag", m_BuildHiZFS, false);

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_HiZMinBuffer);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_HiZMaxBuffer);

	KRenderGlobal::RenderDevice->CreatePipeline(m_ReadDepthPipeline);

	KSamplerDescription desc;

	desc.minFilter = FM_NEAREST;
	desc.magFilter = FM_NEAREST;
	KRenderGlobal::SamplerManager.Acquire(desc, m_ReadDepthSampler);

	desc.minFilter = FM_NEAREST;
	desc.magFilter = FM_NEAREST;
	KRenderGlobal::SamplerManager.Acquire(desc, m_HiZBuildSampler);

	Resize(width, height);

	return true;
}

bool KHiZBuffer::UnInit()
{
	m_QuadVS.Release();
	m_ReadDepthFS.Release();
	m_BuildHiZFS.Release();
	m_ReadDepthSampler.Release();
	m_HiZBuildSampler.Release();
	m_HiZSampler.Release();
	SAFE_UNINIT(m_HiZMinBuffer);
	SAFE_UNINIT(m_HiZMaxBuffer);
	SAFE_UNINIT(m_ReadDepthPipeline);
	SAFE_UNINIT_CONTAINER(m_BuildHiZMinPipelines);
	SAFE_UNINIT_CONTAINER(m_BuildHiZMaxPipelines);

	SAFE_UNINIT_CONTAINER(m_HiZMinRenderPass);
	SAFE_UNINIT_CONTAINER(m_HiZMaxRenderPass);

	m_NumMips = 0;
	m_HiZWidth = 0;
	m_HiZHeight = 0;
	return true;
}

bool KHiZBuffer::Construct(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker("HiZMinBuild", glm::vec4(0, 1, 0, 0));
	for (uint32_t i = 0; i < m_NumMips; ++i)
	{
		if (i == 0)
		{
			KRenderCommand command;
			command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			command.indexDraw = true;

			IKRenderPassPtr renderPass = m_HiZMinRenderPass[i];

			primaryBuffer->BeginDebugMarker("HiZMinInit", glm::vec4(0, 1, 0, 0));

			primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			primaryBuffer->SetViewport(renderPass->GetViewPort());

			command.pipeline = m_ReadDepthPipeline;
			command.pipeline->GetHandle(renderPass, command.pipelineHandle);

			struct ObjectData
			{
				glm::vec4 sampleScaleBias;
				int32_t minBuild;
			} objectData;

			IKFrameBufferPtr srcBuffer = KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer();
			IKFrameBufferPtr destBuffer = m_HiZMinBuffer->GetFrameBuffer();

			objectData.sampleScaleBias.x = (float)srcBuffer->GetWidth();
			objectData.sampleScaleBias.y = (float)srcBuffer->GetHeight();
			objectData.sampleScaleBias.z = (0.5f / destBuffer->GetWidth()) * srcBuffer->GetWidth();
			objectData.sampleScaleBias.w = (0.5f / destBuffer->GetHeight()) * srcBuffer->GetHeight();

			objectData.minBuild = true;

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

			primaryBuffer->Render(command);

			primaryBuffer->EndRenderPass();
			primaryBuffer->EndDebugMarker();
			primaryBuffer->TranslateMipmap(m_HiZMinBuffer->GetFrameBuffer(), i, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
		else
		{
			KRenderCommand command;
			command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			command.indexDraw = true;

			IKRenderPassPtr renderPass = m_HiZMinRenderPass[i];

			primaryBuffer->BeginDebugMarker("HiZMinBuild_" + std::to_string(i), glm::vec4(0, 1, 0, 0));

			primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			primaryBuffer->SetViewport(renderPass->GetViewPort());

			command.pipeline = m_BuildHiZMinPipelines[i - 1];
			command.pipeline->GetHandle(renderPass, command.pipelineHandle);

			struct ObjectData
			{
				int32_t minBuild;
			} objectData;
			objectData.minBuild = true;

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

			primaryBuffer->Render(command);

			primaryBuffer->EndRenderPass();
			primaryBuffer->EndDebugMarker();
			primaryBuffer->TranslateMipmap(m_HiZMinBuffer->GetFrameBuffer(), i, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
	}
	primaryBuffer->EndDebugMarker();

	primaryBuffer->BeginDebugMarker("HiZMaxBuild", glm::vec4(0, 1, 0, 0));
	for (uint32_t i = 0; i < m_NumMips; ++i)
	{
		if (i == 0)
		{
			KRenderCommand command;
			command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			command.indexDraw = true;

			IKRenderPassPtr renderPass = m_HiZMaxRenderPass[i];

			primaryBuffer->BeginDebugMarker("HiZMaxInit", glm::vec4(0, 1, 0, 0));

			primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			primaryBuffer->SetViewport(renderPass->GetViewPort());

			command.pipeline = m_ReadDepthPipeline;
			command.pipeline->GetHandle(renderPass, command.pipelineHandle);

			struct ObjectData
			{
				glm::vec4 sampleScaleBias;
				int32_t minBuild;
			} objectData;

			IKFrameBufferPtr srcBuffer = KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer();
			IKFrameBufferPtr destBuffer = m_HiZMinBuffer->GetFrameBuffer();

			objectData.sampleScaleBias.x = (float)srcBuffer->GetWidth();
			objectData.sampleScaleBias.y = (float)srcBuffer->GetHeight();
			objectData.sampleScaleBias.z = (0.5f / destBuffer->GetWidth()) * srcBuffer->GetWidth();
			objectData.sampleScaleBias.w = (0.5f / destBuffer->GetHeight()) * srcBuffer->GetHeight();

			objectData.minBuild = false;

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

			primaryBuffer->Render(command);

			primaryBuffer->EndRenderPass();
			primaryBuffer->EndDebugMarker();
			primaryBuffer->TranslateMipmap(m_HiZMaxBuffer->GetFrameBuffer(), i, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
		else
		{
			KRenderCommand command;
			command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			command.indexDraw = true;

			IKRenderPassPtr renderPass = m_HiZMaxRenderPass[i];

			primaryBuffer->BeginDebugMarker("HiZMaxBuild_" + std::to_string(i), glm::vec4(0, 1, 0, 0));

			primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			primaryBuffer->SetViewport(renderPass->GetViewPort());

			command.pipeline = m_BuildHiZMaxPipelines[i - 1];
			command.pipeline->GetHandle(renderPass, command.pipelineHandle);

			struct ObjectData
			{
				int32_t minBuild;
			} objectData;
			objectData.minBuild = false;

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

			primaryBuffer->Render(command);

			primaryBuffer->EndRenderPass();
			primaryBuffer->EndDebugMarker();
			primaryBuffer->TranslateMipmap(m_HiZMaxBuffer->GetFrameBuffer(), i, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
	}
	primaryBuffer->EndDebugMarker();

	return true;
}

bool KHiZBuffer::ReloadShader()
{
	if (m_QuadVS)
		m_QuadVS->Reload();
	if (m_ReadDepthFS)
		m_ReadDepthFS->Reload();
	if (m_BuildHiZFS)
		m_BuildHiZFS->Reload();
	if (m_ReadDepthPipeline)
	{
		m_ReadDepthPipeline->Reload();
	}
	for (IKPipelinePtr& pipeline : m_BuildHiZMinPipelines)
	{
		pipeline->Reload();
	}
	for (IKPipelinePtr& pipeline : m_BuildHiZMaxPipelines)
	{
		pipeline->Reload();
	}
	return true;
}