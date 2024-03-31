#include "KDeferredRenderer.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKStatistics.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Render/KRenderUtil.h"
#include "Internal/ECS/Component/KDebugComponent.h"

KDeferredRenderer::KDeferredRenderer()
	: m_Camera(nullptr)
	, m_DebugOption(DRD_NONE)
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
	UnInit();

	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	for (uint32_t i = 0; i < DRS_STAGE_COUNT; ++i)
	{
		KRenderGlobal::Statistics.RegisterRenderStage(GDeferredRenderStageDescription[i].debugMarker);
	}

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/deferred.frag", m_DeferredLightingFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/draw.frag", m_SceneColorDrawFS, false);

	
	renderDevice->CreateRenderTarget(m_FinalTarget);

	RecreateRenderPass(width, height);
	RecreatePipeline();

	m_Camera = camera;
}

void KDeferredRenderer::UnInit()
{
	m_QuadVS.Release();
	m_DeferredLightingFS.Release();
	m_SceneColorDrawFS.Release();

	SAFE_UNINIT(m_FinalTarget);
	SAFE_UNINIT(m_LightingPipeline);
	SAFE_UNINIT(m_DrawSceneColorPipeline);
	SAFE_UNINIT(m_DrawFinalPipeline);

	SAFE_UNINIT(m_EmptyAORenderPass);
	for (uint32_t i = 0; i < DRS_STAGE_COUNT; ++i)
	{
		SAFE_UNINIT(m_RenderPass[i]);
		m_RenderCallFuncs[i].clear();
		KRenderGlobal::Statistics.UnRegisterRenderStage(GDeferredRenderStageDescription[i].debugMarker);
	}
	m_Camera = nullptr;
}

void KDeferredRenderer::AddCallFunc(DeferredRenderStage stage, RenderPassCallFuncType* func)
{
	auto it = std::find(m_RenderCallFuncs[stage].begin(), m_RenderCallFuncs[stage].end(), func);
	if (it == m_RenderCallFuncs[stage].end())
	{
		m_RenderCallFuncs[stage].push_back(func);
	}
}

void KDeferredRenderer::RemoveCallFunc(DeferredRenderStage stage, RenderPassCallFuncType* func)
{
	auto it = std::find(m_RenderCallFuncs[stage].begin(), m_RenderCallFuncs[stage].end(), func);
	if (it != m_RenderCallFuncs[stage].end())
	{
		m_RenderCallFuncs[stage].erase(it);
	}
}

void KDeferredRenderer::RecreateRenderPass(uint32_t width, uint32_t height)
{
	m_FinalTarget->UnInit();
	m_FinalTarget->InitFromColor(width, height, 1, 1, EF_R16G16B16A16_FLOAT);

	auto EnsureRenderPass = [](IKRenderPassPtr& renderPass)
	{
		if (renderPass) renderPass->UnInit();
		else ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(renderPass));
	};

	EnsureRenderPass(m_EmptyAORenderPass);
	m_EmptyAORenderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer());
	m_EmptyAORenderPass->SetOpColor(0, LO_CLEAR, SO_STORE);
	m_EmptyAORenderPass->SetClearColor(0, { 1.0f, 1.0f, 1.0f, 1.0f });
	ASSERT_RESULT(m_EmptyAORenderPass->Init());

	for (uint32_t idx = 0; idx < DRS_STAGE_COUNT; ++idx)
	{
		IKRenderPassPtr& renderPass = m_RenderPass[idx];
		EnsureRenderPass(renderPass);

		if (idx == DRS_STAGE_MAIN_BASE_PASS || idx == DRS_STAGE_POST_BASE_PASS)
		{
			LoadOperation loadOp = (idx == DRS_STAGE_MAIN_BASE_PASS) ? LO_CLEAR : LO_LOAD;
			for (uint32_t gbuffer = GBUFFER_TARGET0; gbuffer < GBUFFER_TARGET_COUNT; ++gbuffer)
			{
				renderPass->SetColorAttachment(gbuffer, KRenderGlobal::GBuffer.GetGBufferTarget((GBufferTarget)gbuffer)->GetFrameBuffer());
				renderPass->SetOpColor(gbuffer, loadOp, SO_STORE);
				renderPass->SetClearColor(gbuffer, { 0.0f, 0.0f, 0.0f, 0.0f });
			}
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			renderPass->SetOpDepthStencil(loadOp, SO_STORE, loadOp, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STAGE_DEFERRED_LIGHTING)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_CLEAR, SO_STORE);
			renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STAGE_FORWARD_OPAQUE)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STAGE_FORWARD_TRANSPRANT)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_SKY)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_COPY_SCENE_COLOR)
		{
			renderPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_DEBUG_OBJECT)
		{
			renderPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_FOREGROUND)
		{
			renderPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
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

	{
		EnsurePipeline(m_LightingPipeline);
		IKPipelinePtr& pipeline = m_LightingPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_DeferredLightingFS);

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
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET4)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE5,
			KRenderGlobal::CascadedShadowMap.GetFinalMask()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
		{
			pipeline->SetSampler(SHADER_BINDING_TEXTURE6,
				KRenderGlobal::Voxilzer.GetFinalMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);
		}

		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			pipeline->SetSampler(SHADER_BINDING_TEXTURE6,
				KRenderGlobal::ClipmapVoxilzer.GetFinalMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);
		}

		pipeline->SetSampler(SHADER_BINDING_TEXTURE7,
			KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE8,
			KRenderGlobal::VolumetricFog.GetScatteringTarget()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE9,
			KRenderGlobal::ScreenSpaceReflection.GetAOTarget()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE10,
			KRenderGlobal::CubeMap.GetDiffuseIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetDiffuseIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE11,
			KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetSpecularIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE12,
			KRenderGlobal::CubeMap.GetIntegrateBRDF()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetIntegrateBRDFSampler(),
			true);

		pipeline->Init();
	}

	{
		EnsurePipeline(m_DrawFinalPipeline);
		IKPipelinePtr& pipeline = m_DrawFinalPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_SceneColorDrawFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_FinalTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
	
		pipeline->Init();
	}

	{
		EnsurePipeline(m_DrawSceneColorPipeline);
		IKPipelinePtr& pipeline = m_DrawSceneColorPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_SceneColorDrawFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);

		pipeline->Init();
	}
}

void KDeferredRenderer::BuildMaterialSubMeshInstance(DeferredRenderStage renderStage, const std::vector<IKEntity*>& cullRes, std::vector<KMaterialSubMeshInstance>& instances)
{
	if (renderStage == DRS_STAGE_MAIN_BASE_PASS)
	{
		KRenderUtil::CalculateInstancesByMaterial(cullRes, instances);
	}
	else if(renderStage == DRS_STAGE_FORWARD_TRANSPRANT)
	{
		KMaterialSubMeshInstanceCompareFunction comp = [this](const KMaterialSubMeshInstance& lhs, const KMaterialSubMeshInstance& rhs) ->bool
		{
			const glm::vec3& cameraPos = m_Camera->GetPosition();
			const glm::vec3& cameraForward = m_Camera->GetForward();

			assert(lhs.instanceData.size() == rhs.instanceData.size() == 1);
			const glm::vec3 lhsPos = glm::vec3(lhs.instanceData[0].ROW0[3], lhs.instanceData[0].ROW1[3], lhs.instanceData[0].ROW2[3]);
			const glm::vec3 rhsPos = glm::vec3(rhs.instanceData[0].ROW0[3], rhs.instanceData[0].ROW1[3], rhs.instanceData[0].ROW2[3]);

			const float lhsDis = glm::dot(lhsPos - cameraPos, cameraForward);
			const float rhsDis = glm::dot(rhsPos - cameraPos, cameraForward);

			return lhsDis > rhsDis;
		};

		KRenderUtil::GetInstances(cullRes, instances, comp);
	}
}

void KDeferredRenderer::HandleRenderCommandBinding(DeferredRenderStage renderStage, KRenderCommand& command)
{
	if (renderStage == DRS_STAGE_FORWARD_TRANSPRANT)
	{
		IKPipelinePtr& pipeline = command.pipeline;

		IKUniformBufferPtr voxelSVOBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
		pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_FRAGMENT, voxelSVOBuffer);

		IKUniformBufferPtr voxelClipmapBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);
		pipeline->SetConstantBuffer(CBT_VOXEL_CLIPMAP, ST_VERTEX | ST_FRAGMENT, voxelClipmapBuffer);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
		pipeline->SetConstantBuffer(CBT_GLOBAL, ST_VERTEX | ST_FRAGMENT, globalBuffer);

		for (uint32_t cascadedIndex = 0; cascadedIndex <= 3; ++cascadedIndex)
		{
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, true);
			if (!shadowRT) shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, true);
			pipeline->SetSampler(SHADER_BINDING_TEXTURE5 + cascadedIndex,
				shadowRT->GetFrameBuffer(),
				KRenderGlobal::CascadedShadowMap.GetSampler(),
				false);
		}
		for (uint32_t cascadedIndex = 0; cascadedIndex <= 3; ++cascadedIndex)
		{
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, false);
			if (!shadowRT) shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, false);
			pipeline->SetSampler(SHADER_BINDING_TEXTURE9 + cascadedIndex,
				shadowRT->GetFrameBuffer(),
				KRenderGlobal::CascadedShadowMap.GetSampler(),
				false);
		}

		pipeline->SetSampler(SHADER_BINDING_TEXTURE13,
			KRenderGlobal::CubeMap.GetDiffuseIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetDiffuseIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE14,
			KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetSpecularIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE15,
			KRenderGlobal::CubeMap.GetIntegrateBRDF()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetIntegrateBRDFSampler(),
			true);

		struct Debug
		{
			uint32_t debugOption;
		} debug;

		debug.debugOption = m_DebugOption;

		KDynamicConstantBufferUsage debugUsage;
		debugUsage.binding = SHADER_BINDING_DEBUG;
		debugUsage.range = sizeof(debugUsage);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&debug, debugUsage);

		command.dynamicConstantUsages.push_back(debugUsage);
	}
}

void KDeferredRenderer::BuildRenderCommand(KMultithreadingRenderContext& renderContext, DeferredRenderStage deferredRenderStage, const std::vector<IKEntity*>& cullRes)
{
	RenderStage renderStage = GDeferredRenderStageDescription[deferredRenderStage].renderStage;
	RenderStage instanceRenderStage = GDeferredRenderStageDescription[deferredRenderStage].instanceRenderStage;
	const char* debugMarker = GDeferredRenderStageDescription[deferredRenderStage].debugMarker;

	std::vector<KRenderCommand> commands;

	KRenderStageStatistics& statistics = m_Statistics[deferredRenderStage];
	statistics.Reset();

	std::vector<KMaterialSubMeshInstance> subMeshInstances;
	BuildMaterialSubMeshInstance(deferredRenderStage, cullRes, subMeshInstances);

	IKRenderPassPtr renderPass = m_RenderPass[deferredRenderStage];
	renderContext.primaryBuffer->BeginDebugMarker(debugMarker, glm::vec4(1));

	if (!KRenderGlobal::GPUScene.GetEnable() || deferredRenderStage != DRS_STAGE_MAIN_BASE_PASS)
	{
		for (KMaterialSubMeshInstance& subMeshInstance : subMeshInstances)
		{
			const std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = subMeshInstance.instanceData;

			ASSERT_RESULT(!instances.empty());

			if (instanceRenderStage != RENDER_STAGE_UNKNOWN && instances.size() > 1)
			{
				KRenderCommand command;
				if (subMeshInstance.materialSubMesh->GetRenderCommand(instanceRenderStage, command))
				{
					if (!KRenderUtil::AssignShadingParameter(command, subMeshInstance.materialSubMesh->GetMaterial()))
					{
						continue;
					}

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

					++statistics.drawcalls;

					if (command.indexDraw)
					{
						statistics.primtives += command.indexData->indexCount;
						statistics.faces += command.indexData->indexCount / 3;
					}
					else
					{
						statistics.primtives += command.vertexData->vertexCount;
						statistics.faces += command.vertexData->vertexCount / 3;
					}

					HandleRenderCommandBinding(deferredRenderStage, command);
					command.pipeline->GetHandle(m_RenderPass[deferredRenderStage], command.pipelineHandle);

					if (command.Complete())
					{
						commands.push_back(std::move(command));
					}
				}
			}
			else
			{
				KRenderCommand command;
				if (subMeshInstance.materialSubMesh->GetRenderCommand(renderStage, command))
				{
					if (!KRenderUtil::AssignShadingParameter(command, subMeshInstance.materialSubMesh->GetMaterial()))
					{
						continue;
					}

					for (size_t idx = 0; idx < instances.size(); ++idx)
					{
						const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[idx];

						KConstantDefinition::OBJECT objectData;
						objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
						objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));

						KDynamicConstantBufferUsage objectUsage;
						objectUsage.binding = SHADER_BINDING_OBJECT;
						objectUsage.range = sizeof(objectData);
						KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

						command.dynamicConstantUsages.push_back(objectUsage);

						++statistics.drawcalls;

						if (command.indexDraw)
						{
							statistics.primtives += command.indexData->indexCount;
							statistics.faces += command.indexData->indexCount / 3;
						}
						else
						{
							statistics.primtives += command.vertexData->vertexCount;
							statistics.faces += command.vertexData->vertexCount / 3;
						}

						HandleRenderCommandBinding(deferredRenderStage, command);
						command.pipeline->GetHandle(m_RenderPass[deferredRenderStage], command.pipelineHandle);

						if (command.Complete())
						{
							commands.push_back(std::move(command));
						}
					}
				}
			}
		}
	}

	if (renderContext.enableMultithreading)
	{
		std::vector<KRenderCommand>& commandList = commands;

		renderContext.primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_SECONDARY);

		size_t threadNum = renderContext.threadCommandPools.size();

		size_t commandEachThread = commandList.size() / threadNum;
		size_t currentCommandIndex = 0;
		size_t currentCommandCount = commandEachThread + commandList.size() % threadNum;

		KCommandBufferList commandBuffers;
		commandBuffers.resize((commandEachThread > 0) ? renderContext.threadCommandPools.size() : 1);

		for (auto it = commandList.begin(); it != commandList.end();)
		{
			KRenderCommand& command = *it;
			if (!command.pipeline->GetHandle(renderPass, command.pipelineHandle))
			{
				it = commandList.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (size_t threadIndex = 0; threadIndex < commandBuffers.size(); ++threadIndex)
		{
			renderContext.threadPool->AddJob(threadIndex, [this, deferredRenderStage, currentCommandIndex, currentCommandCount, renderPass, &commandList, &renderContext, &commandBuffers, threadIndex]()
			{
				IKCommandBufferPtr commandBuffer = renderContext.threadCommandPools[threadIndex]->Request(CBL_SECONDARY);

				commandBuffer->BeginSecondary(renderPass);

				commandBuffer->SetViewport(renderPass->GetViewPort());

				for (size_t idx = 0; idx < currentCommandCount; ++idx)
				{
					KRenderCommand& command = commandList[currentCommandIndex + idx];
					command.threadIndex = (uint32_t)threadIndex;
					commandBuffer->Render(command);
				}

				if (threadIndex == 0)
				{
					std::for_each(m_RenderCallFuncs[deferredRenderStage].begin(), m_RenderCallFuncs[deferredRenderStage].end(), [renderPass, commandBuffer](RenderPassCallFuncType* func)
					{
						(*func)(renderPass, commandBuffer);
					});
				}

				commandBuffer->End();

				commandBuffers[threadIndex] = commandBuffer;
			});
			currentCommandIndex += currentCommandCount;
			currentCommandCount = commandEachThread;
		}

		renderContext.threadPool->WaitAll();
		renderContext.primaryBuffer->ExecuteAll(commandBuffers, false);
	}
	else
	{
		renderContext.primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		renderContext.primaryBuffer->SetViewport(renderPass->GetViewPort());
		for (KRenderCommand& command : commands)
		{
			IKPipelineHandlePtr handle = nullptr;
			if (command.pipeline->GetHandle(renderPass, handle))
			{
				renderContext.primaryBuffer->Render(command);
			}
		}

		std::for_each(m_RenderCallFuncs[deferredRenderStage].begin(), m_RenderCallFuncs[deferredRenderStage].end(), [renderPass, renderContext](RenderPassCallFuncType* func)
		{
			(*func)(renderPass, renderContext.primaryBuffer);
		});
	}

	renderContext.primaryBuffer->EndRenderPass();
	renderContext.primaryBuffer->EndDebugMarker();

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(GDeferredRenderStageDescription[renderStage].debugMarker, statistics);
}

void KDeferredRenderer::EmptyAO(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker("EmptyAO", glm::vec4(1));
	primaryBuffer->BeginRenderPass(m_EmptyAORenderPass, SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(m_EmptyAORenderPass->GetViewPort());

	primaryBuffer->EndRenderPass();
	primaryBuffer->Transition(KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	primaryBuffer->EndDebugMarker();
}

void KDeferredRenderer::Foreground(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_FOREGROUND].debugMarker, glm::vec4(1));
	primaryBuffer->BeginRenderPass(m_RenderPass[DRS_STATE_FOREGROUND], SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(m_RenderPass[DRS_STATE_FOREGROUND]->GetViewPort());

	std::for_each(m_RenderCallFuncs[DRS_STATE_FOREGROUND].begin(), m_RenderCallFuncs[DRS_STATE_FOREGROUND].end(), [this, primaryBuffer](RenderPassCallFuncType* func)
	{
		(*func)(m_RenderPass[DRS_STATE_FOREGROUND], primaryBuffer);
	});

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();
}

void KDeferredRenderer::SkyPass(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_SKY].debugMarker, glm::vec4(1));
	primaryBuffer->BeginRenderPass(m_RenderPass[DRS_STATE_SKY], SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(m_RenderPass[DRS_STATE_SKY]->GetViewPort());

	KRenderGlobal::SkyBox.Render(m_RenderPass[DRS_STATE_SKY], primaryBuffer);

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();
}

void KDeferredRenderer::CopySceneColorToFinal(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	primaryBuffer->BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_COPY_SCENE_COLOR].debugMarker, glm::vec4(1));
	primaryBuffer->BeginRenderPass(m_RenderPass[DRS_STATE_COPY_SCENE_COLOR], SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(m_RenderPass[DRS_STATE_COPY_SCENE_COLOR]->GetViewPort());

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_DrawSceneColorPipeline;
	command.pipeline->GetHandle(m_RenderPass[DRS_STATE_COPY_SCENE_COLOR], command.pipelineHandle);
	command.indexDraw = true;
	primaryBuffer->Render(command);

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();

	primaryBuffer->Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
}

void KDeferredRenderer::PrePass(KMultithreadingRenderContext& renderContext, const std::vector<IKEntity*>& cullRes)
{
}

void KDeferredRenderer::MainBasePass(KMultithreadingRenderContext& renderContext, const std::vector<IKEntity*>& cullRes)
{
	BuildRenderCommand(renderContext, DRS_STAGE_MAIN_BASE_PASS, cullRes);
}

void KDeferredRenderer::PostBasePass(KMultithreadingRenderContext& renderContext)
{
	BuildRenderCommand(renderContext, DRS_STAGE_POST_BASE_PASS, {});
}

void KDeferredRenderer::ForwardOpaque(KMultithreadingRenderContext& renderContext, const std::vector<IKEntity*>& cullRes)
{
	BuildRenderCommand(renderContext, DRS_STAGE_FORWARD_OPAQUE, cullRes);
}

void KDeferredRenderer::ForwardTransprant(IKCommandBufferPtr primaryBuffer, const std::vector<IKEntity*>& cullRes)
{
	KMultithreadingRenderContext context;
	context.primaryBuffer = primaryBuffer;
	context.enableMultithreading = false;
	BuildRenderCommand(context, DRS_STAGE_FORWARD_TRANSPRANT, cullRes);
}

void KDeferredRenderer::DebugObject(IKCommandBufferPtr primaryBuffer, const std::vector<IKEntity*>& cullRes)
{
	primaryBuffer->BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_DEBUG_OBJECT].debugMarker, glm::vec4(1));
	primaryBuffer->BeginRenderPass(m_RenderPass[DRS_STATE_DEBUG_OBJECT], SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(m_RenderPass[DRS_STATE_DEBUG_OBJECT]->GetViewPort());

	KRenderStageStatistics& statistics = m_Statistics[DRS_STATE_DEBUG_OBJECT];
	statistics.Reset();

	std::vector<KRenderCommand> commands;
	std::vector<glm::vec3> positions;
	std::vector<size_t> sortedIndices;

	auto BuildCommand = [&statistics, &commands, &sortedIndices, &positions, this](const std::vector<KMaterialSubMeshPtr>& materialSubMeshes, const KConstantDefinition::DEBUG& objectData)
	{
		glm::vec3 position = KMath::ExtractPosition(objectData.MODEL.MODEL);

		for (KMaterialSubMeshPtr materialSubMesh : materialSubMeshes)
		{
			KRenderCommand command;

			if (materialSubMesh->GetRenderCommand(RENDER_STAGE_DEBUG_TRIANGLE, command))
			{
				KDynamicConstantBufferUsage objectUsage;
				objectUsage.binding = SHADER_BINDING_OBJECT;
				objectUsage.range = sizeof(objectData);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

				command.dynamicConstantUsages.push_back(objectUsage);

				++statistics.drawcalls;
				if (command.indexDraw)
				{
					statistics.faces += command.indexData->indexCount / 3;
					statistics.primtives += command.indexData->indexCount;
				}
				else
				{
					statistics.faces += command.vertexData->vertexCount / 3;
					statistics.primtives += command.vertexData->vertexCount;
				}

				command.pipeline->GetHandle(m_RenderPass[DRS_STATE_DEBUG_OBJECT], command.pipelineHandle);

				if (command.Complete())
				{
					sortedIndices.push_back(commands.size());
					positions.push_back(position);
					commands.push_back(std::move(command));
				}
			}

			if (materialSubMesh->GetRenderCommand(RENDER_STAGE_DEBUG_LINE, command))
			{
				KDynamicConstantBufferUsage objectUsage;
				objectUsage.binding = SHADER_BINDING_OBJECT;
				objectUsage.range = sizeof(objectData);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

				command.dynamicConstantUsages.push_back(objectUsage);

				++statistics.drawcalls;
				if (command.indexDraw)
				{
					statistics.faces += command.indexData->indexCount / 2;
					statistics.primtives += command.indexData->indexCount;
				}
				else
				{
					statistics.faces += command.vertexData->vertexCount / 2;
					statistics.primtives += command.vertexData->vertexCount;
				}

				command.pipeline->GetHandle(m_RenderPass[DRS_STATE_DEBUG_OBJECT], command.pipelineHandle);

				if (command.Complete())
				{
					sortedIndices.push_back(commands.size());
					positions.push_back(position);
					commands.push_back(std::move(command));
				}
			}
		}
	};

	for (IKEntity* entity : cullRes)
	{
		KRenderComponent* render = nullptr;
		KTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_RENDER, &render) && entity->GetComponent(CT_TRANSFORM, &transform))
		{
			const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();

			KConstantDefinition::DEBUG objectData;
			objectData.MODEL = transform->FinalTransform();
			objectData.COLOR = render->GetUtilityColor();

			BuildCommand(materialSubMeshes, objectData);
		}

		KDebugComponent* debug = nullptr;
		if (entity->GetComponent(CT_DEBUG, &debug))
		{
			const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = debug->GetMaterialSubMeshs();
			const std::vector<glm::vec4>& colors = debug->GetColors();

			assert(colors.size() == materialSubMeshes.size());

			for (size_t i = 0; i < materialSubMeshes.size(); ++i)
			{
				KConstantDefinition::DEBUG objectData;
				objectData.MODEL.MODEL = glm::mat4(1.0f);
				objectData.MODEL.PRVE_MODEL = glm::mat4(1.0f);
				objectData.COLOR = colors[i];

				BuildCommand({ materialSubMeshes[i] }, objectData);
			}
		}
	}

	const glm::vec3& cameraPos = m_Camera->GetPosition();
	const glm::vec3& cameraForward = m_Camera->GetForward();

	std::sort(sortedIndices.begin(), sortedIndices.end(), [&cameraPos, &cameraForward, &positions](size_t lhs, size_t rhs) ->bool
	{
		const glm::vec3 lhsPos = positions[lhs];
		const glm::vec3 rhsPos = positions[rhs];

		const float lhsDis = glm::dot(lhsPos - cameraPos, cameraForward);
		const float rhsDis = glm::dot(rhsPos - cameraPos, cameraForward);

		return lhsDis > rhsDis;
	});

	for (size_t index : sortedIndices)
	{
		KRenderCommand& command = commands[index];
		IKPipelineHandlePtr handle = nullptr;
		if (command.pipeline->GetHandle(m_RenderPass[DRS_STATE_DEBUG_OBJECT], handle))
		{
			primaryBuffer->Render(command);
		}
	}

	std::for_each(m_RenderCallFuncs[DRS_STATE_DEBUG_OBJECT].begin(), m_RenderCallFuncs[DRS_STATE_DEBUG_OBJECT].end(), [this, primaryBuffer](RenderPassCallFuncType* func)
	{
		(*func)(m_RenderPass[DRS_STATE_DEBUG_OBJECT], primaryBuffer);
	});

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(GDeferredRenderStageDescription[DRS_STATE_DEBUG_OBJECT].debugMarker, statistics);

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();
}

void KDeferredRenderer::DeferredLighting(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker(GDeferredRenderStageDescription[DRS_STAGE_DEFERRED_LIGHTING].debugMarker, glm::vec4(1));
	primaryBuffer->BeginRenderPass(m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING], SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING]->GetViewPort());

	IKRenderPassPtr renderPass = m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING];

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_LightingPipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	struct Debug
	{
		uint32_t debugOption;
	} debug;

	debug.debugOption = m_DebugOption;

	KDynamicConstantBufferUsage debugUsage;	
	debugUsage.binding = SHADER_BINDING_DEBUG;
	debugUsage.range = sizeof(debugUsage);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&debug, debugUsage);

	command.dynamicConstantUsages.push_back(debugUsage);

	primaryBuffer->Render(command);

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();
}

void KDeferredRenderer::DrawFinalResult(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker("DrawFinalResult", glm::vec4(1,0,0,0));
	primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(renderPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_DrawFinalPipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;
	primaryBuffer->Render(command);

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();
}

void KDeferredRenderer::ReloadShader()
{
	if (m_QuadVS)
		m_QuadVS->Reload();
	if (m_DeferredLightingFS)
		m_DeferredLightingFS->Reload();
	if (m_SceneColorDrawFS)
		m_SceneColorDrawFS->Reload();
	if (m_LightingPipeline)
		m_LightingPipeline->Reload();
	if (m_DrawSceneColorPipeline)
		m_DrawSceneColorPipeline->Reload();
	if (m_DrawFinalPipeline)
		m_DrawSceneColorPipeline->Reload();
}