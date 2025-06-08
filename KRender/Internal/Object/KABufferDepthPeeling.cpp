#include "KABufferDepthPeeling.h"
#include "Internal/KRenderGlobal.h"

KABufferDepthPeeling::KABufferDepthPeeling()
	: m_MaxElementCount(0)
	, m_Width(0)
	, m_Height(0)
{
}

KABufferDepthPeeling::~KABufferDepthPeeling()
{
	ASSERT_RESULT(!m_LinkNextBuffer);
	ASSERT_RESULT(!m_LinkResultBuffer);
	ASSERT_RESULT(!m_LinkDepthBuffer);
	ASSERT_RESULT(!m_LinkHeaderTarget);
}

bool KABufferDepthPeeling::Init(uint32_t width, uint32_t height, uint32_t depth)
{
	UnInit();

	m_Depth = depth;
	Resize(width, height);

	KRenderGlobal::RenderDevice->CreateComputePipeline(m_ResetLinkNextPipeline);
	m_ResetLinkNextPipeline->BindStorageBuffer(ABUFFER_BINDING_LINK_NEXT, m_LinkNextBuffer, COMPUTE_RESOURCE_OUT, true);
	m_ResetLinkNextPipeline->BindDynamicUniformBuffer(ABUFFER_BINDING_OBJECT);
	m_ResetLinkNextPipeline->Init("depthpeeling/reset_link_next.comp", KShaderCompileEnvironment());

	KRenderGlobal::RenderDevice->CreateComputePipeline(m_ResetLinkHeaderPipeline);
	m_ResetLinkHeaderPipeline->BindStorageImage(ABUFFER_BINDING_LINK_HEADER, m_LinkHeaderTarget->GetFrameBuffer(), EF_R32_UINT, COMPUTE_RESOURCE_OUT, 0, true);
	m_ResetLinkHeaderPipeline->BindDynamicUniformBuffer(ABUFFER_BINDING_OBJECT);
	m_ResetLinkHeaderPipeline->Init("depthpeeling/reset_link_header.comp", KShaderCompileEnvironment());

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "depthpeeling/abuffer_blend.frag", m_BlendFS, false);

	{
		KRenderGlobal::RenderDevice->CreatePipeline(m_BlendPipeline);
		IKPipelinePtr& pipeline = m_BlendPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_BlendFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(true);
		pipeline->SetColorBlend(BF_ONE, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetStorageImage(ABUFFER_BINDING_LINK_HEADER, m_LinkHeaderTarget->GetFrameBuffer(), EF_R32_UINT);
		pipeline->SetStorageBuffer(ABUFFER_BINDING_LINK_NEXT, ST_FRAGMENT, m_LinkNextBuffer);
		pipeline->SetStorageBuffer(ABUFFER_BINDING_LINK_RESULT, ST_FRAGMENT, m_LinkResultBuffer);
		pipeline->SetStorageBuffer(ABUFFER_BINDING_LINK_DEPTH, ST_FRAGMENT, m_LinkDepthBuffer);

		pipeline->SetDebugName("ABufferBlendPipeline");
		pipeline->Init();
	}

	return true;
}

bool KABufferDepthPeeling::UnInit()
{
	m_QuadVS.Release();
	m_BlendFS.Release();
	SAFE_UNINIT(m_BlendPipeline);
	SAFE_UNINIT(m_ResetLinkHeaderPipeline);
	SAFE_UNINIT(m_ResetLinkNextPipeline);
	SAFE_UNINIT(m_ShadingABufferPipeline);
	SAFE_UNINIT(m_LinkHeaderTarget);
	SAFE_UNINIT(m_LinkNextBuffer);
	SAFE_UNINIT(m_LinkResultBuffer);
	SAFE_UNINIT(m_LinkDepthBuffer);
	return true;
}

bool KABufferDepthPeeling::Resize(uint32_t width, uint32_t height)
{
	if (m_Width == width && m_Height == height)
	{
		return true;
	}

	if (!m_LinkHeaderTarget)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_LinkHeaderTarget);
	}
	if (!m_LinkNextBuffer)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_LinkNextBuffer);
	}
	if (!m_LinkResultBuffer)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_LinkResultBuffer);
	}
	if (!m_LinkDepthBuffer)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_LinkDepthBuffer);
	}

	m_Width = width;
	m_Height = height;

	m_MaxElementCount = m_Width * m_Height * m_Depth;

	m_LinkHeaderTarget->UnInit();
	m_LinkHeaderTarget->InitFromStorage(m_Width, m_Height, 1, EF_R32_UINT);
	m_LinkHeaderTarget->GetFrameBuffer()->SetDebugName("ABufferLinkHeader");

	m_LinkNextBuffer->UnInit();
	m_LinkNextBuffer->InitMemory(m_MaxElementCount * sizeof(uint32_t), nullptr);
	m_LinkNextBuffer->InitDevice(false, false);
	m_LinkNextBuffer->SetDebugName("ABufferLinkNext");

	m_LinkResultBuffer->UnInit();
	m_LinkResultBuffer->InitMemory(m_MaxElementCount * 4 * sizeof(uint32_t), nullptr);
	m_LinkResultBuffer->InitDevice(false, false);
	m_LinkResultBuffer->SetDebugName("ABufferLinkResult");

	m_LinkDepthBuffer->UnInit();
	m_LinkDepthBuffer->InitMemory(m_MaxElementCount * sizeof(uint32_t), nullptr);
	m_LinkDepthBuffer->InitDevice(false, false);
	m_LinkDepthBuffer->SetDebugName("ABufferLinkDepth");

	return true;
}

bool KABufferDepthPeeling::Execute(KRHICommandList& commandList, IKRenderPassPtr renderPass, const std::vector<IKEntity*>& cullRes)
{
	commandList.BeginDebugMarker("DepthPeelingABuffer", glm::vec4(1));

	glm::uvec4 constant = glm::uvec4(m_Width, m_Height, m_MaxElementCount, 0);
	KDynamicConstantBufferUsage objectUsage;
	objectUsage.binding = ABUFFER_BINDING_OBJECT;
	objectUsage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, objectUsage);

	{
		commandList.BeginDebugMarker("DepthPeelingABufferReset", glm::vec4(1));

		{
			commandList.BeginDebugMarker("DepthPeelingResetLinkHeader", glm::vec4(1));
			uint32_t groupX = (m_Width + (GROUP_SIZE - 1)) / GROUP_SIZE;
			uint32_t groupY = (m_Height + (GROUP_SIZE - 1)) / GROUP_SIZE;
			commandList.Execute(m_ResetLinkHeaderPipeline, groupX, groupY, 1, &objectUsage);
			commandList.EndDebugMarker();
		}

		{
			commandList.BeginDebugMarker("DepthPeelingResetLinkNext", glm::vec4(1));
			uint32_t groupX = (m_MaxElementCount + (GROUP_SIZE - 1)) / GROUP_SIZE;
			commandList.Execute(m_ResetLinkNextPipeline, groupX, 1, 1, &objectUsage);
			commandList.EndDebugMarker();
		}

		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("DepthPeelingABufferRecord", glm::vec4(1));

		commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(renderPass->GetViewPort());
		KRenderCommandList renderCommands;
		KRenderUtil::PopulateRenderCommandList(renderPass, cullRes, renderCommands, RENDER_STAGE_TRANSPRANT_ABUFFER_DEPTH_PEELING, RENDER_STAGE_TRANSPRANT_ABUFFER_DEPTH_PEELING_INSTANCE);
		for (KRenderCommand& renderCommand : renderCommands)
		{
			commandList.Render(renderCommand);
		}
		commandList.EndRenderPass();

		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("DepthPeelingABufferReplay", glm::vec4(1));
		commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(renderPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.pipeline = m_BlendPipeline;
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = true;
		command.dynamicConstantUsages.push_back(objectUsage);
		commandList.Render(command);

		commandList.EndRenderPass();
		commandList.EndDebugMarker();
	}

	commandList.EndDebugMarker();
	return true;
}