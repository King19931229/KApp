#include "KDeferredRenderer.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKStatistics.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Dispatcher/KRenderUtil.h"

const KDeferredRenderStageDescription GDeferredRenderStageDescription[DRS_STAGE_COUNT] =
{
	{DRS_STAGE_PRE_PASS, PIPELINE_STAGE_PRE_Z, PIPELINE_STAGE_PRE_Z_INSTANCE, "PrePass"},
	{DRS_STAGE_BASE_PASS, PIPELINE_STAGE_BASEPASS, PIPELINE_STAGE_BASEPASS_INSTANCE, "BasePass"},
	{DRS_STAGE_DEFERRED_LIGHTING, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "LightingPass"},
	{DRS_STAGE_FORWARD_TRANSPRANT, PIPELINE_STAGE_TRANSPRANT, PIPELINE_STAGE_UNKNOWN, "TransprantPass"}
};

KDeferredRenderer::KDeferredRenderer()
{
}

KDeferredRenderer::~KDeferredRenderer()
{
}

void KDeferredRenderer::Resize(uint32_t width, uint32_t height)
{
	RecreateRenderPass(width, height);
	RecreatePipeline();
}

void KDeferredRenderer::Init(const KCamera* camera, uint32_t width, uint32_t height)
{
	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
	{
		IKCommandBufferPtr& buffer = m_CommandBuffers[i];
		ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
		ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
	}

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_LightingVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/deferred.frag", m_LightingFS, false);

	renderDevice->CreateRenderTarget(m_LightPassTarget);

	RecreateRenderPass(width, height);
	RecreatePipeline();

	m_Camera = camera;
}

void KDeferredRenderer::UnInit()
{
	KRenderGlobal::ShaderManager.Release(m_LightingVS);
	KRenderGlobal::ShaderManager.Release(m_LightingFS);

	SAFE_UNINIT(m_LightPassTarget);
	SAFE_UNINIT(m_LightingPipeline);

	for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
	{
		SAFE_UNINIT(m_CommandBuffers[i]);
		SAFE_UNINIT(m_RenderPass[i]);
	}
	SAFE_UNINIT(m_CommandPool);
	m_Camera = nullptr;
}

void KDeferredRenderer::RecreateRenderPass(uint32_t width, uint32_t height)
{
	m_LightPassTarget->UnInit();
	m_LightPassTarget->InitFromColor(width, height, 1, EF_R16G16B16A16_FLOAT);

	for (uint32_t idx = 0; idx < DRS_STAGE_COUNT; ++idx)
	{
		IKRenderPassPtr& renderPass = m_RenderPass[idx];

		auto EnsureRenderPass = [](IKRenderPassPtr& renderPass)
		{
			if (renderPass) renderPass->UnInit();
			else ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(renderPass));
		};

		EnsureRenderPass(renderPass);

		if (idx == DRS_STAGE_BASE_PASS)
		{
			for (uint32_t gbuffer = GBUFFER_TARGET0; gbuffer < GBUFFER_TARGET_COUNT; ++gbuffer)
			{
				renderPass->SetColorAttachment(gbuffer, KRenderGlobal::GBuffer.GetGBufferTarget((GBufferTarget)gbuffer)->GetFrameBuffer());
				renderPass->SetClearColor(gbuffer, { 0.0f, 0.0f, 0.0f, 0.0f });
			}
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			renderPass->SetOpDepthStencil(LO_CLEAR, SO_STORE, LO_CLEAR, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STAGE_DEFERRED_LIGHTING)
		{
			renderPass->SetColorAttachment(0, m_LightPassTarget->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			renderPass->SetOpDepthStencil(LO_CLEAR, SO_STORE, LO_CLEAR, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}
	}
}

void KDeferredRenderer::RecreatePipeline()
{
	auto EnsurePipeline = [](IKPipelinePtr& pipeline)
	{
		if (pipeline) pipeline->UnInit();
		else ASSERT_RESULT(KRenderGlobal::RenderDevice->CreatePipeline(pipeline));
	};

	EnsurePipeline(m_LightingPipeline);

	IKPipelinePtr& pipeline = m_LightingPipeline;

	pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	pipeline->SetShader(ST_VERTEX, m_LightingVS);
	pipeline->SetShader(ST_FRAGMENT, m_LightingFS);

	pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	pipeline->SetBlendEnable(false);
	pipeline->SetCullMode(CM_NONE);
	pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	pipeline->SetPolygonMode(PM_FILL);
	pipeline->SetColorWrite(true, true, true, true);
	pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

	IKUniformBufferPtr voxelSVOBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
	pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_FRAGMENT, voxelSVOBuffer);

	IKUniformBufferPtr voxelClipmapBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);
	pipeline->SetConstantBuffer(CBT_VOXEL_CLIPMAP, ST_VERTEX | ST_FRAGMENT, voxelClipmapBuffer);

	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
	pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
	pipeline->SetConstantBuffer(CBT_GLOBAL, ST_VERTEX | ST_FRAGMENT, globalBuffer);

	pipeline->SetSampler(SHADER_BINDING_TEXTURE0,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);
	pipeline->SetSampler(SHADER_BINDING_TEXTURE1,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);
	pipeline->SetSampler(SHADER_BINDING_TEXTURE2,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET2)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);
	pipeline->SetSampler(SHADER_BINDING_TEXTURE3,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET3)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);

	pipeline->SetSampler(SHADER_BINDING_TEXTURE4,
		KRenderGlobal::CascadedShadowMap.GetFinalMask()->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);

	pipeline->Init();
}

void KDeferredRenderer::BuildRenderCommand(IKCommandBufferPtr primaryBuffer, DeferredRenderStage renderStage)
{
	PipelineStage pipelineStage = GDeferredRenderStageDescription[renderStage].pipelineStage;
	PipelineStage instancePipelineStage = GDeferredRenderStageDescription[renderStage].instancePipelineStage;
	const char* debugMarker = GDeferredRenderStageDescription[renderStage].debugMakrer;

	if (m_Camera)
	{
		std::vector<KRenderCommand> commands;

		KRenderStageStatistics& statistics = m_Statistics[renderStage];
		statistics.Reset();

		std::vector<KRenderComponent*> cullRes;
		KRenderGlobal::Scene.GetRenderComponent(*m_Camera, false, cullRes);

		KRenderUtil::MeshMaterialInstanceGroup meshMaterialGroups;
		KRenderUtil::CalculateInstanceGroupByMaterial(cullRes, false, meshMaterialGroups);

		for (auto& meshPair : meshMaterialGroups)
		{
			KMeshPtr mesh = meshPair.first;
			KRenderUtil::MaterialMapPtr materialMap = meshPair.second;

			for (auto& materialPair : materialMap->mapping)
			{
				IKMaterialPtr material = materialPair.first;
				KRenderUtil::InstanceGroupPtr instanceGroup = materialPair.second;
				KRenderComponent* render = instanceGroup->render;
				const std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = instanceGroup->instance;

				ASSERT_RESULT(render);
				ASSERT_RESULT(!instances.empty());

				if (instancePipelineStage != PIPELINE_STAGE_UNKNOWN && instances.size() > 1)
				{
					render->Visit(instancePipelineStage, [&](KRenderCommand& _command)
					{
						KRenderCommand command = std::move(_command);

						if (!KRenderUtil::AssignShadingParameter(command, material.get(), false))
						{
							return;
						}

						++statistics.drawcalls;

						std::vector<KInstanceBufferManager::AllocResultBlock> allocRes;
						ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.GetVertexSize() == sizeof(instances[0]));
						ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.Alloc(instances.size(), instances.data(), allocRes));

						command.instanceDraw = true;
						command.instanceUsages.resize(allocRes.size());
						for (size_t i = 0; i < allocRes.size(); ++i)
						{
							KInstanceBufferUsage& usage = command.instanceUsages[i];
							KInstanceBufferManager::AllocResultBlock& allocResult = allocRes[i];
							usage.buffer = allocResult.buffer;
							usage.start = allocResult.start;
							usage.count = allocResult.count;
							usage.offset = allocResult.offset;
						}

						if (command.indexDraw)
						{
							statistics.faces += command.indexData->indexCount / 3;
						}
						else
						{
							statistics.faces += command.vertexData->vertexCount / 3;
						}

						command.pipeline->GetHandle(m_RenderPass[renderStage], command.pipelineHandle);

						if (command.Complete())
						{
							commands.push_back(std::move(command));
						}
					});
				}
				else
				{
					render->Visit(pipelineStage, [&](KRenderCommand& _command)
					{
						KRenderCommand command = std::move(_command);

						if (!KRenderUtil::AssignShadingParameter(command, material.get(), false))
						{
							return;
						}

						++statistics.drawcalls;

						for (size_t idx = 0; idx < instances.size(); ++idx)
						{
							const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[idx];

							KConstantDefinition::OBJECT objectData;
							objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
							objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));
							command.objectUsage.binding = SHADER_BINDING_OBJECT;
							command.objectUsage.range = sizeof(objectData);
							KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

							if (command.indexDraw)
							{
								statistics.faces += command.indexData->indexCount / 3;
							}
							else
							{
								statistics.faces += command.vertexData->vertexCount / 3;
							}

							command.pipeline->GetHandle(m_RenderPass[renderStage], command.pipelineHandle);

							if (command.Complete())
							{
								commands.push_back(std::move(command));
							}
						}
					});
				}
			}
		}

		IKCommandBufferPtr commandBuffer = m_CommandBuffers[renderStage];

		primaryBuffer->BeginDebugMarker(debugMarker, glm::vec4(0, 1, 0, 0));
		primaryBuffer->BeginRenderPass(m_RenderPass[renderStage], SUBPASS_CONTENTS_SECONDARY);

		commandBuffer->BeginSecondary(m_RenderPass[renderStage]);
		commandBuffer->SetViewport(m_RenderPass[renderStage]->GetViewPort());

		for (KRenderCommand& command : commands)
		{
			IKPipelineHandlePtr handle = nullptr;
			if (command.pipeline->GetHandle(m_RenderPass[renderStage], handle))
			{
				commandBuffer->Render(command);
			}
		}

		commandBuffer->End();

		primaryBuffer->Execute(commandBuffer);
		primaryBuffer->EndRenderPass();
		primaryBuffer->EndDebugMarker();

		KRenderGlobal::Statistics.UpdateRenderStageStatistics(GDeferredRenderStageDescription[renderStage].debugMakrer, statistics);
	}
}

void KDeferredRenderer::PrePass(IKCommandBufferPtr primaryBuffer)
{
}

void KDeferredRenderer::BasePass(IKCommandBufferPtr primaryBuffer)
{
	BuildRenderCommand(primaryBuffer, DRS_STAGE_BASE_PASS);
}

void KDeferredRenderer::DeferredLighting(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker(GDeferredRenderStageDescription[DRS_STAGE_DEFERRED_LIGHTING].debugMakrer, glm::vec4(0, 1, 0, 0));
	primaryBuffer->BeginRenderPass(m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING], SUBPASS_CONTENTS_SECONDARY);

	IKCommandBufferPtr commandBuffer = m_CommandBuffers[DRS_STAGE_DEFERRED_LIGHTING];
	IKRenderPassPtr renderPass = m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING];

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_LightingPipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	commandBuffer->BeginSecondary(renderPass);
	commandBuffer->SetViewport(renderPass->GetViewPort());
	commandBuffer->Render(command);
	commandBuffer->End();

	primaryBuffer->Execute(commandBuffer);
	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();
}

void KDeferredRenderer::ForwardTransprant(IKCommandBufferPtr primaryBuffer)
{
}