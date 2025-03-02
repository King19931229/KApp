#include "KDepthPeeling.h"
#include "Internal/KRenderGlobal.h"

KDepthPeeling::KDepthPeeling()
	: m_Width(0)
	, m_Height(0)
	, m_PeelingLayers(0)
{}

KDepthPeeling::~KDepthPeeling()
{
	ASSERT_RESULT(m_PeelingTarget == nullptr);
	ASSERT_RESULT(m_PeelingPass == nullptr);
	for (uint32_t i = 0; i < 2; ++i)
	{
		ASSERT_RESULT(m_PeelingDepthTarget[i] == nullptr);
	}
}

bool KDepthPeeling::Init(uint32_t width, uint32_t height, uint32_t layers)
{
	UnInit();
	m_PeelingLayers = layers;

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/underblend.frag", m_UnderBlendFS, false);

	Resize(width, height);

	KRenderGlobal::RenderDevice->CreateSampler(m_DeelingDepthSampler);
	m_DeelingDepthSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_DeelingDepthSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_DeelingDepthSampler->Init(0, 0);

	{
		KRenderGlobal::RenderDevice->CreatePipeline(m_UnderBlendPeelingPipeline);
		IKPipelinePtr& pipeline = m_UnderBlendPeelingPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_UnderBlendFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(true);
		pipeline->SetColorBlend(BF_DST_ALPHA, BF_ONE, BO_ADD);
		pipeline->SetAlphaBlend(BF_ZERO, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_PeelingTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);

		pipeline->SetDebugName("UnderBlendPeelingPipeline");
		pipeline->Init();
	}

	{
		KRenderGlobal::RenderDevice->CreatePipeline(m_UnderBlendOpaquePipeline);
		IKPipelinePtr& pipeline = m_UnderBlendOpaquePipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_UnderBlendFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(true);
		pipeline->SetColorBlend(BF_DST_ALPHA, BF_ONE, BO_ADD);
		pipeline->SetAlphaBlend(BF_ZERO, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetOpaqueColorCopy()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);

		pipeline->SetDebugName("UnderBlendOpaquePipeline");
		pipeline->Init();
	}

	return true;
}

bool KDepthPeeling::UnInit()
{
	if (m_QuadVS)
		m_QuadVS.Release();
	if (m_UnderBlendFS)
		m_UnderBlendFS.Release();
	SAFE_UNINIT(m_DeelingDepthSampler);
	SAFE_UNINIT(m_UnderBlendPeelingPipeline);
	SAFE_UNINIT(m_UnderBlendOpaquePipeline);
	SAFE_UNINIT(m_PeelingTarget);
	SAFE_UNINIT(m_UnderBlendPass);
	SAFE_UNINIT(m_CleanSceneColorPass);
	SAFE_UNINIT(m_PeelingPass);
	SAFE_UNINIT(m_CleanPeelingDepthPass);
	for (uint32_t i = 0; i < 2; ++i)
	{
		SAFE_UNINIT(m_PeelingDepthTarget[i]);
	}
	return true;
}

bool KDepthPeeling::Resize(uint32_t width, uint32_t height)
{
	if (m_Width == width && m_Height == height)
	{
		return true;
	}

	m_Width = width;
	m_Height = height;

	{
		if (!m_CleanSceneColorPass)
		{
			KRenderGlobal::RenderDevice->CreateRenderPass(m_CleanSceneColorPass);
		}
		else
		{
			m_CleanSceneColorPass->UnInit();
		}
		m_CleanSceneColorPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
		m_CleanSceneColorPass->SetOpColor(0, LO_CLEAR, SO_STORE);
		m_CleanSceneColorPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 1.0f });
		ASSERT_RESULT(m_CleanSceneColorPass->Init());
	}

	{
		if (!m_UnderBlendPass)
		{
			KRenderGlobal::RenderDevice->CreateRenderPass(m_UnderBlendPass);
		}
		else
		{
			m_UnderBlendPass->UnInit();
		}
		m_UnderBlendPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_UnderBlendPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
		m_UnderBlendPass->SetOpColor(0, LO_LOAD, SO_STORE);
		m_UnderBlendPass->Init();
		m_UnderBlendPass->SetDebugName("UnderBlendPass");
	}

	if (!m_PeelingTarget)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_PeelingTarget);
	}
	else
	{
		m_PeelingTarget->UnInit();
	}
	m_PeelingTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);
	m_PeelingTarget->GetFrameBuffer()->SetDebugName("PeelingTarget");

	for (uint32_t i = 0; i < 2; ++i)
	{
		if (!m_PeelingDepthTarget[i])
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_PeelingDepthTarget[i]);
		}
		else
		{
			m_PeelingDepthTarget[i]->UnInit();
		}
		m_PeelingDepthTarget[i]->InitFromDepthStencil(m_Width, m_Height, 1, true);
		m_PeelingDepthTarget[i]->GetFrameBuffer()->SetDebugName(("PeelingDepth" + std::to_string(i)).c_str());
	}

	
	{
		if (!m_PeelingPass)
		{
			KRenderGlobal::RenderDevice->CreateRenderPass(m_PeelingPass);
		}
		else
		{
			m_PeelingPass->UnInit();
		}
		m_PeelingPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_PeelingPass->SetColorAttachment(0, m_PeelingTarget->GetFrameBuffer());
		m_PeelingPass->SetOpColor(0, LO_CLEAR, SO_STORE);
		m_PeelingPass->SetClearDepthStencil({ 1.0f, 0 });
		m_PeelingPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
		m_PeelingPass->SetDepthStencilAttachment(m_PeelingDepthTarget[0]->GetFrameBuffer());
		m_PeelingPass->Init();
		m_PeelingPass->SetDebugName("PeelingPingPass");
	}

	{
		if (!m_CleanPeelingDepthPass)
		{
			KRenderGlobal::RenderDevice->CreateRenderPass(m_CleanPeelingDepthPass);
		}
		else
		{
			m_CleanPeelingDepthPass->UnInit();
		}
		m_CleanPeelingDepthPass->SetClearDepthStencil({ 0.0f, 0 });
		m_CleanPeelingDepthPass->SetOpDepthStencil(LO_CLEAR, SO_STORE, LO_CLEAR, SO_STORE);
		m_CleanPeelingDepthPass->SetDepthStencilAttachment(m_PeelingDepthTarget[1]->GetFrameBuffer());
		ASSERT_RESULT(m_CleanPeelingDepthPass->Init());
	}

	KRenderGlobal::ImmediateCommandList.BeginRecord();
	KRenderGlobal::ImmediateCommandList.Transition(m_PeelingDepthTarget[1]->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	KRenderGlobal::ImmediateCommandList.EndRecord();

	return true;
}

bool KDepthPeeling::PopulateRenderCommandList(const std::vector<IKEntity*>& cullRes, KRenderCommandList& renderCommands)
{
	std::vector<KMaterialSubMeshInstance> subMeshInstances;
	KRenderUtil::CalculateInstancesByMaterial(cullRes, subMeshInstances);

	renderCommands.clear();

	for (KMaterialSubMeshInstance& subMeshInstance : subMeshInstances)
	{
		KRenderCommand baseCommand;
		const std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = subMeshInstance.instanceData;

		if (!subMeshInstance.materialSubMesh->GetRenderCommand(RENDER_STAGE_TRANSPRANT_DEPTH_PEELING, baseCommand))
		{
			continue;
		}

		if (!KRenderUtil::AssignShadingParameter(baseCommand, subMeshInstance.materialSubMesh->GetMaterial()))
		{
			continue;
		}

		for (size_t idx = 0; idx < instances.size(); ++idx)
		{
			KRenderCommand command = baseCommand;

			const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[idx];

			KConstantDefinition::OBJECT objectData;
			objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
			objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));

			KDynamicConstantBufferUsage objectUsage;
			objectUsage.binding = SHADER_BINDING_OBJECT;
			objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

			command.dynamicConstantUsages.push_back(objectUsage);

			KRenderUtil::AssignRenderStageBinding(command, RENDER_STAGE_TRANSPRANT_DEPTH_PEELING, 0);
			command.pipeline->GetHandle(m_PeelingPass, command.pipelineHandle);

			if (command.Complete())
			{
				renderCommands.push_back(std::move(command));
			}
		}
	}

	return true;
}

bool KDepthPeeling::Execute(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	commandList.BeginDebugMarker("DepthPeeling", glm::vec4(1));
	{
		{
			commandList.BeginDebugMarker("CleanSceneColorPass", glm::vec4(1));
			commandList.BeginRenderPass(m_CleanSceneColorPass, SUBPASS_CONTENTS_INLINE);
			commandList.SetViewport(m_CleanSceneColorPass->GetViewPort());
			commandList.EndRenderPass();
			commandList.EndDebugMarker();
		}

		{
			commandList.BeginDebugMarker("CleanPeelingDepth", glm::vec4(1));
			commandList.Transition(m_PeelingDepthTarget[1]->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
			commandList.BeginRenderPass(m_CleanPeelingDepthPass, SUBPASS_CONTENTS_INLINE);
			commandList.SetViewport(m_CleanPeelingDepthPass->GetViewPort());
			commandList.EndRenderPass();
			commandList.Transition(m_PeelingDepthTarget[1]->GetFrameBuffer(), PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
			commandList.EndDebugMarker();
		}

		KRenderCommandList renderCommands;
		PopulateRenderCommandList(cullRes, renderCommands);

		for (uint32_t i = 0; i < m_PeelingLayers; ++i)
		{
			{
				commandList.BeginDebugMarker(("Peeling" + std::to_string(i)).c_str(), glm::vec4(1));

				{
					commandList.Transition(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_TRANSFER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_TRANSFER_SRC);
					commandList.Transition(m_PeelingDepthTarget[0]->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_TRANSFER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_TRANSFER_DST);
					commandList.Blit(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer(), m_PeelingDepthTarget[0]->GetFrameBuffer());
					commandList.Transition(m_PeelingDepthTarget[0]->GetFrameBuffer(), PIPELINE_STAGE_TRANSFER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_TRANSFER_DST, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
					commandList.Transition(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer(), PIPELINE_STAGE_TRANSFER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_TRANSFER_SRC, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
				}

				commandList.BeginRenderPass(m_PeelingPass, SUBPASS_CONTENTS_INLINE);
				commandList.SetViewport(m_PeelingPass->GetViewPort());

				for (KRenderCommand& renderCommand : renderCommands)
				{
					commandList.Render(renderCommand);
				}

				commandList.EndRenderPass();

				commandList.Transition(m_PeelingDepthTarget[0]->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_TRANSFER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_TRANSFER_SRC);
				commandList.Transition(m_PeelingDepthTarget[1]->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_TRANSFER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_TRANSFER_DST);

				commandList.Blit(m_PeelingDepthTarget[0]->GetFrameBuffer(), m_PeelingDepthTarget[1]->GetFrameBuffer());

				commandList.Transition(m_PeelingDepthTarget[1]->GetFrameBuffer(), PIPELINE_STAGE_TRANSFER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_TRANSFER_DST, IMAGE_LAYOUT_SHADER_READ_ONLY);
				commandList.Transition(m_PeelingDepthTarget[0]->GetFrameBuffer(), PIPELINE_STAGE_TRANSFER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_TRANSFER_SRC, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);

				commandList.EndDebugMarker();
			}

			{
				commandList.BeginDebugMarker(("UnderBlendPeeling" + std::to_string(i)).c_str(), glm::vec4(1));
				commandList.Transition(m_PeelingTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
				commandList.BeginRenderPass(m_UnderBlendPass, SUBPASS_CONTENTS_INLINE);
				commandList.SetViewport(m_UnderBlendPass->GetViewPort());

				KRenderCommand command;
				command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
				command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
				command.pipeline = m_UnderBlendPeelingPipeline;
				command.pipeline->GetHandle(m_UnderBlendPass, command.pipelineHandle);
				command.indexDraw = true;
				commandList.Render(command);

				commandList.EndRenderPass();
				commandList.Transition(m_PeelingTarget->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
				commandList.EndDebugMarker();
			}
		}

		{
			commandList.BeginDebugMarker("UnderBlendOpaque", glm::vec4(1));
			commandList.Transition(KRenderGlobal::GBuffer.GetOpaqueColorCopy()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
			commandList.BeginRenderPass(m_UnderBlendPass, SUBPASS_CONTENTS_INLINE);
			commandList.SetViewport(m_UnderBlendPass->GetViewPort());

			KRenderCommand command;
			command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			command.pipeline = m_UnderBlendOpaquePipeline;
			command.pipeline->GetHandle(m_UnderBlendPass, command.pipelineHandle);
			command.indexDraw = true;
			commandList.Render(command);

			commandList.EndRenderPass();
			commandList.Transition(KRenderGlobal::GBuffer.GetOpaqueColorCopy()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
			commandList.EndDebugMarker();
		}
	}
	commandList.EndDebugMarker();

	return true;
}