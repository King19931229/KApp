#include "KVirtualGeometryScene.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "KBase/Publish/KMath.h"

KVirtualGeometryScene::KVirtualGeometryScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
{
	m_OnSceneChangedFunc = std::bind(&KVirtualGeometryScene::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
	m_OnRenderComponentChangedFunc = std::bind(&KVirtualGeometryScene::OnRenderComponentChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KVirtualGeometryScene::~KVirtualGeometryScene()
{
	assert(m_Instances.empty());
}

void KVirtualGeometryScene::OnSceneChanged(EntitySceneOp op, IKEntity* entity)
{
	if (op == ESO_ADD)
	{
		KRenderComponent* renderComponent = nullptr;
		KTransformComponent* transformComponent = nullptr;
		ASSERT_RESULT(entity->GetComponent(CT_RENDER, &renderComponent));
		ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));
		const glm::mat4& transform = transformComponent->GetFinal_RenderThread();
		if (renderComponent->IsVirtualGeometry())
		{
			KVirtualGeometryResourceRef vg = renderComponent->GetVirtualGeometry();
			AddInstance(entity, transform, vg);
		}
		renderComponent->RegisterCallback(&m_OnRenderComponentChangedFunc);
	}
	if (op == ESO_REMOVE)
	{
		RemoveInstance(entity);
	}
	else if (op == ESO_TRANSFORM)
	{
		KTransformComponent* transformComponent = nullptr;
		ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));
		const glm::mat4& transform = transformComponent->GetFinal_RenderThread();
		TransformInstance(entity, transform);
	}
}

void KVirtualGeometryScene::OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init)
{
	IKEntity* entity = renderComponent->GetEntityHandle();
	if (!entity)
	{
		return;
	}

	if (init)
	{
		if (renderComponent->IsVirtualGeometry())
		{
			KTransformComponent* transformComponent = nullptr;
			ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));
			const glm::mat4& transform = transformComponent->GetFinal_RenderThread();

			KVirtualGeometryResourceRef vg = ((KRenderComponent*)renderComponent)->GetVirtualGeometry();
			AddInstance(entity, transform, vg);
		}
	}
	else
	{
		RemoveInstance(entity);
	}
}

bool KVirtualGeometryScene::Init(IKRenderScene* scene, const KCamera* camera)
{
	UnInit();
	if (scene && camera)
	{
		m_Scene = scene;
		m_Camera = camera;
		uint32_t frameCount = KRenderGlobal::NumFramesInFlight;

		m_PrevViewProj = m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix();
		{
			KRenderGlobal::RenderDevice->CreateUniformBuffer(m_GlobalDataBuffer);
			m_GlobalDataBuffer->InitMemory(sizeof(KVirtualGeometryGlobal), nullptr);
			m_GlobalDataBuffer->InitDevice();
			m_GlobalDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_GLOBAL_DATA);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_InstanceDataBuffer);
			m_InstanceDataBuffer->InitMemory(sizeof(KVirtualGeometryInstance), nullptr);
			m_InstanceDataBuffer->InitDevice(false, false);
			m_InstanceDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INSTANCE_DATA);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MainCullResultBuffer);
			m_MainCullResultBuffer->InitMemory(sizeof(uint32_t), nullptr);
			m_MainCullResultBuffer->InitDevice(false, false);
			m_MainCullResultBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_MAIN_CULL_RESULT);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_QueueStateBuffer);
			m_QueueStateBuffer->InitMemory(2 * sizeof(KVirtualGeometryQueueState), nullptr);
			m_QueueStateBuffer->InitDevice(false, false);
			m_QueueStateBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_QUEUE_STATE);

			std::vector<glm::uvec4> emptyCandidateNodeData;
			emptyCandidateNodeData.resize(2 * MAX_CANDIDATE_NODE);
			memset(emptyCandidateNodeData.data(), -1, 2 * MAX_CANDIDATE_NODE * sizeof(glm::uvec4));

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_CandidateNodeBuffer);
			m_CandidateNodeBuffer->InitMemory(2 * MAX_CANDIDATE_NODE * sizeof(glm::uvec4), emptyCandidateNodeData.data());
			m_CandidateNodeBuffer->InitDevice(false, false);
			m_CandidateNodeBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_CANDIDATE_NODE);

			std::vector<glm::uvec4> emptyBatchClusterData;
			emptyBatchClusterData.resize(2 * MAX_CANDIDATE_CLUSTERS);
			memset(emptyBatchClusterData.data(), -1, 2 * MAX_CANDIDATE_CLUSTERS * sizeof(glm::uvec4));

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_CandidateClusterBuffer);
			m_CandidateClusterBuffer->InitMemory(2 * MAX_CANDIDATE_CLUSTERS * sizeof(glm::uvec4), emptyBatchClusterData.data());
			m_CandidateClusterBuffer->InitDevice(false, false);
			m_CandidateClusterBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_CANDIDATE_CLUSTER);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_SelectedClusterBuffer);
			m_SelectedClusterBuffer->InitMemory(2 * MAX_CANDIDATE_CLUSTERS * sizeof(glm::uvec4), emptyBatchClusterData.data());
			m_SelectedClusterBuffer->InitDevice(false, false);
			m_SelectedClusterBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_SELECTED_CLUSTER);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_ExtraDebugBuffer);
			m_ExtraDebugBuffer->InitMemory(2 * MAX_CANDIDATE_CLUSTERS * sizeof(glm::uvec4), emptyBatchClusterData.data());
			m_ExtraDebugBuffer->InitDevice(false, false);
			m_ExtraDebugBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINDING_EXTRA_DEBUG_INFO);

			uint32_t indirectInfo[] = { 1, 1, 1, 1 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_IndirectAgrsBuffer);
			m_IndirectAgrsBuffer->InitMemory(sizeof(indirectInfo), indirectInfo);
			m_IndirectAgrsBuffer->InitDevice(true, false);
			m_IndirectAgrsBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_ARGS);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_PostCullIndirectArgsBuffer);
			m_PostCullIndirectArgsBuffer->InitMemory(sizeof(indirectInfo), indirectInfo);
			m_PostCullIndirectArgsBuffer->InitDevice(true, false);
			m_PostCullIndirectArgsBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_POST_CULL_INDIRECT_ARGS);

			// { vertexCount, instanceCount, firstVertex, firstInstance }
			int32_t indirectDrawInfo[] = { 0, 0, 0, 0 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_IndirectDrawBuffer);
			m_IndirectDrawBuffer->InitMemory(sizeof(indirectDrawInfo), indirectDrawInfo);
			m_IndirectDrawBuffer->InitDevice(true, false);
			m_IndirectDrawBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_DRAW_ARGS);

			// { groupCountX, groupCountY, groupCountZ }
			int32_t indirectMeshInfo[] = { 0, 0, 0 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_IndirectMeshBuffer);
			m_IndirectMeshBuffer->InitMemory(sizeof(indirectMeshInfo), indirectMeshInfo);
			m_IndirectMeshBuffer->InitDevice(true, false);
			m_IndirectMeshBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_MESH_ARGS);

			std::vector<glm::uvec4> emptyBinningBatchData;
			emptyBinningBatchData.resize(MAX_CANDIDATE_CLUSTERS);
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_BinningDataBuffer);
			m_BinningDataBuffer->InitMemory(emptyBinningBatchData.size() * sizeof(glm::uvec4), emptyBinningBatchData.data());
			m_BinningDataBuffer->InitDevice(false, false);
			m_BinningDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINNING_DATA);

			uint32_t emptyBinningHeader[] = { 0, 0, 0, 0 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_BinningHeaderBuffer);
			m_BinningHeaderBuffer->InitMemory(sizeof(emptyBinningHeader), emptyBinningHeader);
			m_BinningHeaderBuffer->InitDevice(false, false);
			m_BinningHeaderBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINNIIG_HEADER);
		}

		{
			for (uint32_t i = 0; i < INSTANCE_CULL_COUNT; ++i)
			{
				KShaderCompileEnvironment cullEnv;
				cullEnv.parentEnv = &KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv();
				if (i == INSTANCE_CULL_NONE)
				{
					cullEnv.macros.push_back({ "INSTANCE_CULL_MODE", "INSTANCE_CULL_NONE" });
				}
				else if (i == INSTANCE_CULL_MAIN)
				{
					cullEnv.macros.push_back({ "INSTANCE_CULL_MODE", "INSTANCE_CULL_MAIN" });
				}
				else
				{
					cullEnv.macros.push_back({ "INSTANCE_CULL_MODE", "INSTANCE_CULL_POST" });
				}

				KShaderCompileEnvironment persistentCullEnv;
				persistentCullEnv.parentEnv = &cullEnv;
				persistentCullEnv.macros.push_back({ "PERSISTENT_CULL", "" });

				KShaderCompileEnvironment nodeCullEnv;
				nodeCullEnv.parentEnv = &cullEnv;
				nodeCullEnv.macros.push_back({ "NODE_CULL", "" });

				KShaderCompileEnvironment clusterCullEnv;
				clusterCullEnv.parentEnv = &cullEnv;
				clusterCullEnv.macros.push_back({ "CLUSTER_CULL", "" });

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitQueueStatePipeline[i]);
				m_InitQueueStatePipeline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_OUT, true);
				if (i == INSTANCE_CULL_MAIN)
				{
					m_InitQueueStatePipeline[i]->BindStorageBuffer(BINDING_POST_CULL_INDIRECT_ARGS, m_PostCullIndirectArgsBuffer, COMPUTE_RESOURCE_OUT, true);
					m_InitQueueStatePipeline[i]->BindUniformBuffer(BINDING_STREAMING_DATA, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
				}

				m_InitQueueStatePipeline[i]->Init("virtualgeometry/init.comp", cullEnv);

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_InstanceCullPipeline[i]);
				m_InstanceCullPipeline[i]->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
				m_InstanceCullPipeline[i]->BindUniformBuffer(BINDING_STREAMING_DATA, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
				m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);
				m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
				m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
				m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_CANDIDATE_NODE_BATCH, m_CandidateNodeBuffer, COMPUTE_RESOURCE_OUT, true);

				if (i == INSTANCE_CULL_MAIN)
				{
					m_InstanceCullPipeline[i]->BindSampler(BINDING_HIZ_BUFFER, KRenderGlobal::HiZBuffer.GetMaxBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), true);
					m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_MAIN_CULL_RESULT, m_MainCullResultBuffer, COMPUTE_RESOURCE_OUT, true);
					m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_POST_CULL_INDIRECT_ARGS, m_PostCullIndirectArgsBuffer, COMPUTE_RESOURCE_OUT, true);
				}
				else if (i == INSTANCE_CULL_POST)
				{
					m_InstanceCullPipeline[i]->BindStorageBuffer(BINDING_MAIN_CULL_RESULT, m_MainCullResultBuffer, COMPUTE_RESOURCE_IN, true);
					m_InstanceCullPipeline[i]->BindSampler(BINDING_HIZ_BUFFER, KRenderGlobal::HiZBuffer.GetMaxBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), true);
				}

				m_InstanceCullPipeline[i]->Init("virtualgeometry/instance_cull.comp", cullEnv);

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitNodeCullArgsPipeline[i]);
				m_InitNodeCullArgsPipeline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_InitNodeCullArgsPipeline[i]->BindStorageBuffer(BINDING_INDIRECT_ARGS, m_IndirectAgrsBuffer, COMPUTE_RESOURCE_OUT, true);
				m_InitNodeCullArgsPipeline[i]->Init("virtualgeometry/init_node_cull_args.comp", cullEnv);

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitClusterCullArgsPipeline[i]);
				m_InitClusterCullArgsPipeline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_InitClusterCullArgsPipeline[i]->BindStorageBuffer(BINDING_INDIRECT_ARGS, m_IndirectAgrsBuffer, COMPUTE_RESOURCE_OUT, true);
				m_InitClusterCullArgsPipeline[i]->Init("virtualgeometry/init_cluster_cull_args.comp", cullEnv);

				auto BindNodeCullResource = [this](IKComputePipelinePtr pipeline, InstanceCull cullMode)
				{
					pipeline->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
					pipeline->BindUniformBuffer(BINDING_STREAMING_DATA, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
					pipeline->BindStorageBuffer(BINDING_PAGE_DATA, KRenderGlobal::VirtualGeometryManager.GetPageDataBuffer(), COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_HIERARCHY_DATA, KRenderGlobal::VirtualGeometryManager.GetPackedHierarchyBuffer(), COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
					pipeline->BindStorageBuffer(BINDING_CANDIDATE_NODE_BATCH, m_CandidateNodeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
					pipeline->BindStorageBuffer(BINDING_CANDIDATE_CLUSTER_BATCH, m_CandidateClusterBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
					pipeline->BindStorageBuffer(BINDING_EXTRA_DEBUG_INFO, m_ExtraDebugBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
					if (cullMode != INSTANCE_CULL_NONE)
					{
						pipeline->BindSampler(BINDING_HIZ_BUFFER, KRenderGlobal::HiZBuffer.GetMaxBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), true);
					}
				};

				m_NodeCullPipelines[i].resize(frameCount);
				for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
				{
					KRenderGlobal::RenderDevice->CreateComputePipeline(m_NodeCullPipelines[i][frameIndex]);
					BindNodeCullResource(m_NodeCullPipelines[i][frameIndex], (InstanceCull)i);
					m_NodeCullPipelines[i][frameIndex]->BindStorageBuffer(BINDING_STREAMING_REQUEST, KRenderGlobal::VirtualGeometryManager.GetStreamingRequestPipeline(frameIndex), COMPUTE_RESOURCE_IN, true);
					m_NodeCullPipelines[i][frameIndex]->Init("virtualgeometry/node_and_cluster_cull.comp", nodeCullEnv);
				}

				auto BindNodeClusterResource = [this](IKComputePipelinePtr pipeline, InstanceCull cullMode)
				{
					pipeline->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
					pipeline->BindUniformBuffer(BINDING_STREAMING_DATA, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
					pipeline->BindStorageBuffer(BINDING_PAGE_DATA, KRenderGlobal::VirtualGeometryManager.GetPageDataBuffer(), COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
					pipeline->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_CANDIDATE_CLUSTER_BATCH, m_CandidateClusterBuffer, COMPUTE_RESOURCE_IN, true);
					pipeline->BindStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, m_SelectedClusterBuffer, COMPUTE_RESOURCE_OUT, true);
					pipeline->BindStorageBuffer(BINDING_EXTRA_DEBUG_INFO, m_ExtraDebugBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
					if (cullMode != INSTANCE_CULL_NONE)
					{
						pipeline->BindSampler(BINDING_HIZ_BUFFER, KRenderGlobal::HiZBuffer.GetMaxBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), true);
					}
				};

				m_ClusterCullPipelines[i].resize(frameCount);
				for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
				{
					KRenderGlobal::RenderDevice->CreateComputePipeline(m_ClusterCullPipelines[i][frameIndex]);
					BindNodeClusterResource(m_ClusterCullPipelines[i][frameIndex], (InstanceCull)i);
					m_ClusterCullPipelines[i][frameIndex]->Init("virtualgeometry/node_and_cluster_cull.comp", clusterCullEnv);
				}

				m_PersistentCullPipelines[i].resize(frameCount);
				for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
				{
					KRenderGlobal::RenderDevice->CreateComputePipeline(m_PersistentCullPipelines[i][frameIndex]);
					BindNodeCullResource(m_PersistentCullPipelines[i][frameIndex], (InstanceCull)i);
					BindNodeClusterResource(m_PersistentCullPipelines[i][frameIndex], (InstanceCull)i);
					m_PersistentCullPipelines[i][frameIndex]->BindStorageBuffer(BINDING_STREAMING_REQUEST, KRenderGlobal::VirtualGeometryManager.GetStreamingRequestPipeline(frameIndex), COMPUTE_RESOURCE_IN, true);
					m_PersistentCullPipelines[i][frameIndex]->Init("virtualgeometry/node_and_cluster_cull.comp", persistentCullEnv);
				}

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_CalcDrawArgsPipeline[i]);
				m_CalcDrawArgsPipeline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_CalcDrawArgsPipeline[i]->BindStorageBuffer(BINDING_INDIRECT_DRAW_ARGS, m_IndirectDrawBuffer, COMPUTE_RESOURCE_OUT, true);
				m_CalcDrawArgsPipeline[i]->BindStorageBuffer(BINDING_INDIRECT_MESH_ARGS, m_IndirectMeshBuffer, COMPUTE_RESOURCE_OUT, true);
				m_CalcDrawArgsPipeline[i]->Init("virtualgeometry/calc_draw_args.comp", cullEnv);

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitBinningPipline[i]);
				m_InitBinningPipline[i]->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
				m_InitBinningPipline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_InitBinningPipline[i]->BindStorageBuffer(BINDING_INDIRECT_ARGS, m_IndirectAgrsBuffer, COMPUTE_RESOURCE_OUT, true);
				m_InitBinningPipline[i]->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
				m_InitBinningPipline[i]->Init("virtualgeometry/init_binning.comp", cullEnv);

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_BinningClassifyPipline[i]);
				m_BinningClassifyPipline[i]->BindUniformBuffer(BINDING_STREAMING_DATA, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_PAGE_DATA, KRenderGlobal::VirtualGeometryManager.GetPageDataBuffer(), COMPUTE_RESOURCE_IN, true);
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, m_SelectedClusterBuffer, COMPUTE_RESOURCE_IN, true);
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);;
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_CLUSTER_MATERIAL_BUFFER, KRenderGlobal::VirtualGeometryManager.GetClusterMaterialStorageBuffer(), COMPUTE_RESOURCE_IN, true);
				m_BinningClassifyPipline[i]->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
				m_BinningClassifyPipline[i]->Init("virtualgeometry/binning_classify.comp", cullEnv);

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_BinningAllocatePipline[i]);
				m_BinningAllocatePipline[i]->BindUniformBuffer(BINDING_STREAMING_DATA, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
				m_BinningAllocatePipline[i]->BindStorageBuffer(BINDING_PAGE_DATA, KRenderGlobal::VirtualGeometryManager.GetPageDataBuffer(), COMPUTE_RESOURCE_IN, true);
				m_BinningAllocatePipline[i]->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
				m_BinningAllocatePipline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_BinningAllocatePipline[i]->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
				m_BinningAllocatePipline[i]->BindStorageBuffer(BINDING_INDIRECT_DRAW_ARGS, m_IndirectDrawBuffer, COMPUTE_RESOURCE_OUT, true);
				m_BinningAllocatePipline[i]->BindStorageBuffer(BINDING_INDIRECT_MESH_ARGS, m_IndirectMeshBuffer, COMPUTE_RESOURCE_OUT, true);
				m_BinningAllocatePipline[i]->Init("virtualgeometry/binning_allocate.comp", cullEnv);

				KRenderGlobal::RenderDevice->CreateComputePipeline(m_BinningScatterPipline[i]);
				m_BinningScatterPipline[i]->BindUniformBuffer(BINDING_STREAMING_DATA, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_PAGE_DATA, KRenderGlobal::VirtualGeometryManager.GetPageDataBuffer(), COMPUTE_RESOURCE_IN, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, m_SelectedClusterBuffer, COMPUTE_RESOURCE_IN, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);;
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_CLUSTER_MATERIAL_BUFFER, KRenderGlobal::VirtualGeometryManager.GetClusterMaterialStorageBuffer(), COMPUTE_RESOURCE_IN, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_BINNING_DATA, m_BinningDataBuffer, COMPUTE_RESOURCE_OUT, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_INDIRECT_DRAW_ARGS, m_IndirectDrawBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
				m_BinningScatterPipline[i]->BindStorageBuffer(BINDING_INDIRECT_MESH_ARGS, m_IndirectMeshBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
				m_BinningScatterPipline[i]->Init("virtualgeometry/binning_scatter.comp", cullEnv);
			}
		}

		{
			KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "virtualgeometry/vg_basepass.vert", KRenderGlobal::VirtualGeometryManager.GetBasepassBindingEnv(), m_BasePassVertexShader, false);
			KRenderGlobal::ShaderManager.Acquire(ST_MESH, "virtualgeometry/vg_basepass.mesh", KRenderGlobal::VirtualGeometryManager.GetBasepassBindingEnv(), m_BasePassMeshShader, false);
		}

		{
			KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "virtualgeometry/vg_debug.vert", KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv(), m_DebugVertexShader, false);
			KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "virtualgeometry/vg_debug.frag", KRenderGlobal::VirtualGeometryManager.GetDefaultBindingEnv(), m_DebugFragmentShader, false);

			KRenderGlobal::RenderDevice->CreatePipeline(m_DebugPipeline);
			m_DebugPipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			m_DebugPipeline->SetBlendEnable(false);
			m_DebugPipeline->SetCullMode(CM_BACK);
			m_DebugPipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			m_DebugPipeline->SetPolygonMode(PM_FILL);
			m_DebugPipeline->SetColorWrite(true, true, true, true);
			m_DebugPipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			m_DebugPipeline->SetShader(ST_VERTEX, m_DebugVertexShader.Get());
			m_DebugPipeline->SetShader(ST_FRAGMENT, m_DebugFragmentShader.Get());

			m_DebugPipeline->SetStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, ST_VERTEX, m_SelectedClusterBuffer);
			m_DebugPipeline->SetStorageBuffer(BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA, ST_VERTEX, m_InstanceDataBuffer);
			m_DebugPipeline->SetStorageBuffer(BINDING_RESOURCE, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer());
			m_DebugPipeline->SetStorageBuffer(BINDING_CLUSTER_BATCH, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer());

			m_DebugPipeline->SetStorageBuffer(BINDING_CLUSTER_VERTEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterVertexStorageBuffer());
			m_DebugPipeline->SetStorageBuffer(BINDING_CLUSTER_INDEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterIndexStorageBuffer());
			m_DebugPipeline->SetStorageBuffer(BINDING_CLUSTER_MATERIAL_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterMaterialStorageBuffer());

			m_DebugPipeline->SetStorageBuffer(BINDING_BINNING_DATA, ST_VERTEX, m_BinningDataBuffer);
			m_DebugPipeline->SetStorageBuffer(BINDING_BINNING_HEADER, ST_VERTEX, m_BinningHeaderBuffer);

			m_DebugPipeline->SetConstantBuffer(BINDING_GLOBAL_DATA, ST_VERTEX, m_GlobalDataBuffer);

			m_DebugPipeline->SetConstantBuffer(BINDING_STREAMING_DATA, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
			m_DebugPipeline->SetStorageBuffer(BINDING_PAGE_DATA, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetPageDataBuffer());

			m_DebugPipeline->Init();
		}

		std::vector<IKEntity*> entites;
		scene->GetAllEntities(entites);

		for (IKEntity*& entity : entites)
		{
			OnSceneChanged(ESO_ADD, entity);
		}

		m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);

		return true;
	}
	return false;
}

bool KVirtualGeometryScene::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
		m_Scene = nullptr;
	}

	m_Camera = nullptr;

	SAFE_UNINIT(m_GlobalDataBuffer);
	SAFE_UNINIT(m_InstanceDataBuffer);
	SAFE_UNINIT(m_MainCullResultBuffer);
	SAFE_UNINIT(m_PostCullIndirectArgsBuffer);
	SAFE_UNINIT(m_QueueStateBuffer);
	SAFE_UNINIT(m_CandidateNodeBuffer);
	SAFE_UNINIT(m_CandidateClusterBuffer);
	SAFE_UNINIT(m_IndirectAgrsBuffer);
	SAFE_UNINIT(m_SelectedClusterBuffer);
	SAFE_UNINIT(m_ExtraDebugBuffer);
	SAFE_UNINIT(m_IndirectDrawBuffer);
	SAFE_UNINIT(m_IndirectMeshBuffer);
	SAFE_UNINIT(m_BinningDataBuffer);
	SAFE_UNINIT(m_BinningHeaderBuffer);
	for (uint32_t i = 0; i < INSTANCE_CULL_COUNT; ++i)
	{
		SAFE_UNINIT(m_InitQueueStatePipeline[i]);
		SAFE_UNINIT(m_InstanceCullPipeline[i]);
		SAFE_UNINIT(m_InitNodeCullArgsPipeline[i]);
		SAFE_UNINIT(m_InitClusterCullArgsPipeline[i]);
		SAFE_UNINIT_CONTAINER(m_NodeCullPipelines[i]);
		SAFE_UNINIT_CONTAINER(m_PersistentCullPipelines[i]);
		SAFE_UNINIT_CONTAINER(m_ClusterCullPipelines[i]);
		SAFE_UNINIT(m_CalcDrawArgsPipeline[i]);
		SAFE_UNINIT(m_InitBinningPipline[i]);
		SAFE_UNINIT(m_BinningClassifyPipline[i]);
		SAFE_UNINIT(m_BinningAllocatePipline[i]);
		SAFE_UNINIT(m_BinningScatterPipline[i]);
	}

	SAFE_UNINIT(m_DebugPipeline);

	m_DebugVertexShader.Release();
	m_DebugFragmentShader.Release();

	m_InstanceMap.clear();
	m_Instances.clear();
	m_LastInstanceData.clear();
	m_BinningMaterials.clear();

	m_BasePassVertexShader.Release();
	m_BasePassMeshShader.Release();

	for (uint32_t i = 0; i < BINNIING_PIPELINE_COUNT; ++i)
	{
		m_BasePassFragmentShaders[i].clear();
		SAFE_UNINIT_CONTAINER(m_BinningPipelines[i]);
	}

	return true;
}

bool KVirtualGeometryScene::UpdateInstanceData(KRHICommandList& commandList)
{
	void* pWrite = nullptr;

	if (m_GlobalDataBuffer)
	{
		KVirtualGeometryGlobal globalData;

		globalData.worldToClip = m_Camera ? (m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix()) : glm::mat4(1);
		globalData.prevWorldToClip = m_PrevViewProj;
		globalData.worldToView = m_Camera ? m_Camera->GetViewMatrix() : glm::mat4(1);
		globalData.worldToTranslateView = m_Camera ? m_Camera->GetTranslateViewMatrix() : glm::mat4(1);
		globalData.misc.x = m_Camera ? m_Camera->GetNear() : 0.0f;
		globalData.misc.y = m_Camera ? m_Camera->GetAspect() : 1.0f;

		globalData.misc2.x = (uint32_t)m_Instances.size();
		globalData.misc2.y = (uint32_t)m_BinningMaterials.size();
		globalData.misc2.z = KRenderGlobal::VirtualGeometryManager.GetUseConeCull();

		size_t sceneWidth = 0;
		size_t sceneHeight = 0;
		KRenderGlobal::GBuffer.GetSceneColor()->GetSize(sceneWidth, sceneHeight);
		globalData.misc.z = m_Camera ? (sceneHeight / m_Camera->GetHeight()) : 1.0f;

		commandList.UpdateUniformBuffer(m_GlobalDataBuffer, &globalData, 0, sizeof(globalData));

		m_PrevViewProj = globalData.worldToClip;
	}

	if (!m_InstanceDataBuffer)
	{
		return false;
	}

	std::vector<KVirtualGeometryInstance> instanceData;
	instanceData.resize(m_Instances.size());

	const std::vector<KMaterialRef>& allMaterials = KRenderGlobal::VirtualGeometryManager.GetAllMaterials();
	std::unordered_map<uint32_t, uint32_t> materialIndexToBinningIndex;

	std::vector<KMaterialRef> binningMaterials;

	for (size_t i = 0; i < m_Instances.size(); ++i)
	{
		KVirtualGeometryResourceRef resource = m_Instances[i]->resource;
		instanceData[i].resourceIndex = resource->resourceIndex;
		instanceData[i].prevTransform = m_Instances[i]->prevTransform;
		instanceData[i].transform = m_Instances[i]->transform;

		uint32_t materialBaseIndex = resource->materialBaseIndex;
		uint32_t binningBaseIndex = 0;

		auto it = materialIndexToBinningIndex.find(materialBaseIndex);
		if (it != materialIndexToBinningIndex.end())
		{
			binningBaseIndex = it->second;
		}
		else
		{
			binningBaseIndex = (uint32_t)binningMaterials.size();
			for (uint32_t localMaterialIndex = 0; localMaterialIndex < resource->materialNum; ++localMaterialIndex)
			{
				uint32_t materialIndex = materialBaseIndex + localMaterialIndex;
				uint32_t binningIndex = binningBaseIndex + localMaterialIndex;
				binningMaterials.push_back(allMaterials[materialIndex]);
				materialIndexToBinningIndex[materialIndex] = binningIndex;
			}
		}

		instanceData[i].binningBaseIndex = binningBaseIndex;

		m_Instances[i]->prevTransform = m_Instances[i]->transform;
	}

	if (instanceData.size() == m_LastInstanceData.size())
	{
		if (memcmp(instanceData.data(), m_LastInstanceData.data(), m_LastInstanceData.size() * sizeof(KVirtualGeometryInstance)) == 0)
		{
			return true;
		}
	}

	m_BinningMaterials = std::move(binningMaterials);

	// TODO
	for (uint32_t i = 0; i < BINNIING_PIPELINE_COUNT; ++i)
	{
		SAFE_UNINIT_CONTAINER(m_BinningPipelines[i]);		
		m_BinningPipelines[i].resize(m_BinningMaterials.size());
		m_BasePassFragmentShaders[i].clear();
		m_BasePassFragmentShaders[i].resize(m_BinningMaterials.size());
	}

	for (uint32_t k = 0; k < BINNIING_PIPELINE_COUNT; ++k)
	{
		for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
		{
			IKPipelinePtr& pipeline = m_BinningPipelines[k][i];
			KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

			KShaderCompileEnvironment env;
			env.parentEnv = &KRenderGlobal::VirtualGeometryManager.GetBasepassBindingEnv();
			env.includeFiles.push_back({ "material_generate_code.h", m_BinningMaterials[i]->GetMaterialGeneratedCodeReader() });

			const IKMaterialTextureBindingPtr materialTextureBinding = m_BinningMaterials[i]->GetTextureBinding();
			const IKMaterialTextureBinding* textureBinding = materialTextureBinding.get();

			uint8_t numSlot = textureBinding->GetNumSlot();
			for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
			{
				env.macros.push_back({ PERMUTATING_MACRO[MATERIAL_TEXTURE_BINDING_MACRO_INDEX[i]].macro, textureBinding->GetTexture(i) ? "1" : "0" });
			}

			if (k == BINNIING_PIPELINE_VERTEX)
			{
				IKShaderPtr vsShader = m_BasePassVertexShader.Get();
				pipeline->SetShader(ST_VERTEX, vsShader);
				KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "virtualgeometry/vg_basepass.frag", env, m_BasePassFragmentShaders[k][i], false);
			}
			else
			{
				IKShaderPtr msShader = m_BasePassMeshShader.Get();
				pipeline->SetShader(ST_MESH, msShader);
				KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "virtualgeometry/vg_basepass_mesh.frag", env, m_BasePassFragmentShaders[k][i], false);
			}

			pipeline->SetVertexBinding(nullptr, 0);

			IKShaderPtr fsShader = m_BasePassFragmentShaders[k][i].Get();
			pipeline->SetShader(ST_FRAGMENT, fsShader);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_SELECTED_CLUSTER_BATCH, ST_VERTEX, m_SelectedClusterBuffer);
			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA, ST_VERTEX, m_InstanceDataBuffer);
			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_RESOURCE, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer());
			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_CLUSTER_BATCH, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer());

			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_CLUSTER_VERTEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterVertexStorageBuffer());
			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_CLUSTER_INDEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterIndexStorageBuffer());
			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_CLUSTER_MATERIAL_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterMaterialStorageBuffer());

			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_BINNING_DATA, ST_VERTEX, m_BinningDataBuffer);
			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_BINNING_HEADER, ST_VERTEX, m_BinningHeaderBuffer);

			pipeline->SetConstantBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_GLOBAL_DATA, ST_VERTEX | ST_FRAGMENT, m_GlobalDataBuffer);

			pipeline->SetConstantBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_STREAMING_DATA, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetStreamingDataBuffer());
			pipeline->SetStorageBuffer(VG_BASEPASS_BINDING_OFFSET + BINDING_PAGE_DATA, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetPageDataBuffer());

			for (uint8_t i = 0; i < numSlot; ++i)
			{
				IKTexturePtr texture = textureBinding->GetTexture(i);
				IKSamplerPtr sampler = textureBinding->GetSampler(i);
				if (texture && sampler)
				{
					pipeline->SetSampler(VG_BASEPASS_BINDING_OFFSET + BINDING_VIRTUAL_GEOMETRY_TEXTURE0 + i, texture->GetFrameBuffer(), sampler, true);
				}
			}

			pipeline->Init();
		}
	}

	{
		int32_t indirectDrawInfo[] = { 0, 0, 0, 0 };
		int32_t indirectMeshInfo[] = { 0, 0, 0 };

		size_t binningCount = m_BinningMaterials.size();// +1;

		size_t dataSize = 0;
		size_t targetBufferSize = 0;

		{
			dataSize = sizeof(indirectDrawInfo) * binningCount;
			targetBufferSize = sizeof(indirectDrawInfo) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(binningCount));

			if (m_IndirectDrawBuffer->GetBufferSize() != targetBufferSize)
			{
				FLUSH_INFLIGHT_RENDER_JOB();
				m_IndirectDrawBuffer->UnInit();
				m_IndirectDrawBuffer->InitMemory(targetBufferSize, nullptr);
				m_IndirectDrawBuffer->InitDevice(true, false);
				m_IndirectDrawBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_DRAW_ARGS);
			}
		}

		{
			dataSize = sizeof(indirectMeshInfo) * binningCount;
			targetBufferSize = sizeof(indirectMeshInfo) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(binningCount));

			if (m_IndirectMeshBuffer->GetBufferSize() != targetBufferSize)
			{
				FLUSH_INFLIGHT_RENDER_JOB();
				m_IndirectMeshBuffer->UnInit();
				m_IndirectMeshBuffer->InitMemory(targetBufferSize, nullptr);
				m_IndirectMeshBuffer->InitDevice(true, false);
				m_IndirectMeshBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_MESH_ARGS);
			}
		}
	}

	{
		uint32_t binningHeader[] = { 0, 0, 0, 0 };

		size_t dataSize = sizeof(binningHeader) * m_BinningMaterials.size();
		size_t targetBufferSize = sizeof(binningHeader) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(m_BinningMaterials.size()));

		if (m_BinningHeaderBuffer->GetBufferSize() != targetBufferSize)
		{
			FLUSH_INFLIGHT_RENDER_JOB();
			m_BinningHeaderBuffer->UnInit();
			m_BinningHeaderBuffer->InitMemory(targetBufferSize, nullptr);
			m_BinningHeaderBuffer->InitDevice(false, false);
			m_BinningHeaderBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINNIIG_HEADER);
		}

		if (dataSize)
		{
			m_BinningHeaderBuffer->Map(&pWrite);
			for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
			{
				memcpy(pWrite, binningHeader, sizeof(binningHeader));
				pWrite = POINTER_OFFSET(pWrite, sizeof(binningHeader));
			}
			m_BinningHeaderBuffer->UnMap();
			pWrite = nullptr;
		}
	}

	{
		size_t dataSize = sizeof(KVirtualGeometryInstance) * instanceData.size();
		size_t targetBufferSize = sizeof(KVirtualGeometryInstance) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(instanceData.size()));

		if (m_InstanceDataBuffer->GetBufferSize() != targetBufferSize)
		{
			FLUSH_INFLIGHT_RENDER_JOB();

			m_InstanceDataBuffer->UnInit();
			m_InstanceDataBuffer->InitMemory(targetBufferSize, nullptr);
			m_InstanceDataBuffer->InitDevice(false, false);
			m_InstanceDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INSTANCE_DATA);
		}

		if (dataSize)
		{
			m_InstanceDataBuffer->Map(&pWrite);
			memcpy(pWrite, instanceData.data(), dataSize);
			m_InstanceDataBuffer->UnMap();
			pWrite = nullptr;
		}
	}

	{
		size_t targetBufferSize = sizeof(uint32_t) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(instanceData.size()));

		if (m_MainCullResultBuffer->GetBufferSize() != targetBufferSize)
		{
			FLUSH_INFLIGHT_RENDER_JOB();

			m_MainCullResultBuffer->UnInit();
			m_MainCullResultBuffer->InitMemory(targetBufferSize, nullptr);
			m_MainCullResultBuffer->InitDevice(false, false);
			m_MainCullResultBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_MAIN_CULL_RESULT);
		}
	}

	m_LastInstanceData = std::move(instanceData);

	return true;
}

std::string KVirtualGeometryScene::InstanceCullString(KVirtualGeometryScene::InstanceCull cullMode)
{
	switch (cullMode)
	{
		case INSTANCE_CULL_MAIN:
			return "Main";
		case INSTANCE_CULL_POST:
			return "Post";
		default:
			return "Normal";
	}
}

bool KVirtualGeometryScene::Execute(KRHICommandList& commandList, InstanceCull cullMode)
{
	UpdateInstanceData(commandList);

	std::string cullString = InstanceCullString(cullMode);
	uint32_t currentFrame = KRenderGlobal::CurrentInFlightFrameIndex;

	commandList.BeginDebugMarker("VirtualGeometry" + cullString, glm::vec4(1));
	if (m_InitQueueStatePipeline)
	{
		{
			commandList.BeginDebugMarker("VirtualGeometry_Init" + cullString, glm::vec4(1));
			commandList.Execute(m_InitQueueStatePipeline[cullMode], 1, 1, 1, nullptr);
			commandList.EndDebugMarker();
		}

		{
			commandList.BeginDebugMarker("VirtualGeometry_InstanceCull" + cullString, glm::vec4(1));
			if (cullMode != INSTANCE_CULL_POST)
			{
				uint32_t groupNum = 0;
				groupNum = ((uint32_t)m_Instances.size() + VG_GROUP_SIZE - 1) / VG_GROUP_SIZE;
				commandList.Execute(m_InstanceCullPipeline[cullMode], groupNum, 1, 1, nullptr);
			}
			else
			{
				commandList.ExecuteIndirect(m_InstanceCullPipeline[cullMode], m_PostCullIndirectArgsBuffer, nullptr);
			}
			commandList.EndDebugMarker();
		}

		if (KRenderGlobal::VirtualGeometryManager.GetUsePersistentCull())
		{
			commandList.BeginDebugMarker("VirtualGeometry_NodeCull_Persistent", glm::vec4(1));
			commandList.Execute(m_PersistentCullPipelines[cullMode][currentFrame], 1024, 1, 1, nullptr);
			commandList.EndDebugMarker();
		}
		else
		{
			for (uint32_t level = 0; level < 12; ++level)
			{
				commandList.BeginDebugMarker(("VirtualGeometry_NodeProcess_" + std::to_string(level) + "_" + cullString).c_str(), glm::vec4(1));
				{
					commandList.BeginDebugMarker(("VirtualGeometry_InitNodeCullArgs_" + std::to_string(level) + "_" + cullString).c_str(), glm::vec4(1));
					commandList.Execute(m_InitNodeCullArgsPipeline[cullMode], 1, 1, 1, nullptr);
					commandList.EndDebugMarker();

					commandList.BeginDebugMarker(("VirtualGeometry_NodeCull_" + std::to_string(level) + "_" + cullString).c_str(), glm::vec4(1));
					commandList.ExecuteIndirect(m_NodeCullPipelines[cullMode][currentFrame], m_IndirectAgrsBuffer, nullptr);
					commandList.EndDebugMarker();
				}
				commandList.EndDebugMarker();
			}

			commandList.BeginDebugMarker("VirtualGeometry_InitNodeCulusterArgs" + cullString, glm::vec4(1));
			commandList.Execute(m_InitClusterCullArgsPipeline[cullMode], 1, 1, 1, nullptr);
			commandList.EndDebugMarker();

			commandList.BeginDebugMarker("VirtualGeometry_ClusterCull" + cullString, glm::vec4(1));
			commandList.ExecuteIndirect(m_ClusterCullPipelines[cullMode][currentFrame], m_IndirectAgrsBuffer, nullptr);
			commandList.EndDebugMarker();
		}

		{
			commandList.BeginDebugMarker("VirtualGeometry_CalcDrawArgs" + cullString, glm::vec4(1));
			commandList.ExecuteIndirect(m_CalcDrawArgsPipeline[cullMode], m_IndirectAgrsBuffer, nullptr);
			commandList.EndDebugMarker();
		}

		{
			commandList.BeginDebugMarker("VirtualGeometry_Binning" + cullString, glm::vec4(1));
			{
				uint32_t numGroup = (uint32_t)(m_BinningMaterials.size() + VG_GROUP_SIZE - 1) / VG_GROUP_SIZE;
				{
					commandList.BeginDebugMarker("VirtualGeometry_InitBinning" + cullString, glm::vec4(1));
					commandList.Execute(m_InitBinningPipline[cullMode], numGroup, 1, 1, nullptr);
					commandList.EndDebugMarker();

					commandList.BeginDebugMarker("VirtualGeometry_BinningClassify" + cullString, glm::vec4(1));
					commandList.ExecuteIndirect(m_BinningClassifyPipline[cullMode], m_IndirectAgrsBuffer, nullptr);
					commandList.EndDebugMarker();

					commandList.BeginDebugMarker("VirtualGeometry_BinningAllocate" + cullString, glm::vec4(1));
					commandList.Execute(m_BinningAllocatePipline[cullMode], numGroup, 1, 1, nullptr);
					commandList.EndDebugMarker();

					commandList.BeginDebugMarker("VirtualGeometry_BinningScatter" + cullString, glm::vec4(1));
					commandList.ExecuteIndirect(m_BinningScatterPipline[cullMode], m_IndirectAgrsBuffer, nullptr);
					commandList.EndDebugMarker();
				}
			}
			commandList.EndDebugMarker();
		}

		commandList.TransitionIndirect(m_IndirectDrawBuffer);
		commandList.TransitionIndirect(m_IndirectMeshBuffer);
	}
	commandList.EndDebugMarker();
	return true;
}

bool KVirtualGeometryScene::ExecuteMain(KRHICommandList& commandList)
{
	if (KRenderGlobal::VirtualGeometryManager.GetUseDoubleOcclusion())
	{
		return Execute(commandList, INSTANCE_CULL_MAIN);
	}
	else
	{
		return Execute(commandList, INSTANCE_CULL_NONE);
	}
}

bool KVirtualGeometryScene::ExecutePost(KRHICommandList& commandList)
{
	if (KRenderGlobal::VirtualGeometryManager.GetUseDoubleOcclusion())
	{
		return Execute(commandList, INSTANCE_CULL_POST);
	}
	else
	{
		return true;
	}
}

bool KVirtualGeometryScene::BasePass(IKRenderPassPtr renderPass, KRHICommandList& commandList, InstanceCull cullMode)
{
	std::string cullString = InstanceCullString(cullMode);

	commandList.BeginDebugMarker("VirtualGeometry_BasePass" + cullString, glm::vec4(1));

	for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
	{
		KRenderCommand command;

		if (!KRenderUtil::AssignShadingParameter(command, m_BinningMaterials[i]))
		{
			continue;
		}

		KVirtualGeometryMaterial material;
		material.misc3.x = (uint32_t)i;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = VG_BASEPASS_BINDING_OFFSET + BINDING_MATERIAL_DATA;
		objectUsage.range = sizeof(material);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&material, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		if (KRenderGlobal::VirtualGeometryManager.GetUseMeshPipeline())
		{
			command.pipeline = m_BinningPipelines[BINNIING_PIPELINE_MESH][i];
			command.indirectArgsBuffer = m_IndirectMeshBuffer;
			command.pipeline->GetHandle(renderPass, command.pipelineHandle);
			command.indexDraw = false;
			command.indirectDraw = true;
			command.meshShaderDraw = true;
			command.indirectOffset = (uint32_t)i;
			command.indirectCount = 1;
			commandList.Render(command);
		}
		else
		{
			command.pipeline = m_BinningPipelines[BINNIING_PIPELINE_VERTEX][i];
			command.indirectArgsBuffer = m_IndirectDrawBuffer;
			command.pipeline->GetHandle(renderPass, command.pipelineHandle);
			command.indexDraw = false;
			command.indirectDraw = true;
			command.indirectOffset = (uint32_t)i;
			command.indirectCount = 1;
			commandList.Render(command);
		}
	}

	commandList.EndDebugMarker();

	return true;
}

bool KVirtualGeometryScene::BasePassMain(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	if (KRenderGlobal::VirtualGeometryManager.GetUseDoubleOcclusion())
	{
		return BasePass(renderPass, commandList, INSTANCE_CULL_MAIN);
	}
	else
	{
		return BasePass(renderPass, commandList, INSTANCE_CULL_NONE);
	}
}

bool KVirtualGeometryScene::BasePassPost(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	if (KRenderGlobal::VirtualGeometryManager.GetUseDoubleOcclusion())
	{
		return BasePass(renderPass, commandList, INSTANCE_CULL_POST);
	}
	else
	{
		return true;
	}
}

bool KVirtualGeometryScene::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	commandList.BeginDebugMarker("VirtualGeometry_DebugRender", glm::vec4(1));
#if 0
	for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
	{
		KRenderCommand command;

		command.pipeline = m_DebugPipeline;
		command.indirectArgsBuffer = m_IndirectDrawBuffer;
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = false;
		command.indirectDraw = true;
		command.indirectOffset = (uint32_t)i;
		command.indirectCount = 1;

		KVirtualGeometryMaterial material;
		material.miscs3.x = (uint32_t)i;
		command.objectUsage.binding = BINDING_MATERIAL_DATA;
		command.objectUsage.range = sizeof(material);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&material, command.objectUsage);

		commandList.Render(command);
	}
#endif
	commandList.EndDebugMarker();
	return true;
}

bool KVirtualGeometryScene::Reload()
{
	for (uint32_t i = 0; i < INSTANCE_CULL_COUNT; ++i)
	{
		if (m_InitQueueStatePipeline[i])
		{
			m_InitQueueStatePipeline[i]->Reload();
		}
		if (m_InstanceCullPipeline[i])
		{
			m_InstanceCullPipeline[i]->Reload();
		}
		if (m_InitNodeCullArgsPipeline[i])
		{
			m_InitNodeCullArgsPipeline[i]->Reload();
		}
		if (m_InitClusterCullArgsPipeline[i])
		{
			m_InitClusterCullArgsPipeline[i]->Reload();
		}
		for(IKComputePipelinePtr pipeline : m_NodeCullPipelines[i])
		{
			pipeline->Reload();
		}
		for (IKComputePipelinePtr pipeline : m_PersistentCullPipelines[i])
		{
			pipeline->Reload();
		}
		for (IKComputePipelinePtr pipeline : m_ClusterCullPipelines[i])
		{
			pipeline->Reload();
		}
		if (m_CalcDrawArgsPipeline[i])
		{
			m_CalcDrawArgsPipeline[i]->Reload();
		}
		if (m_InitBinningPipline[i])
		{
			m_InitBinningPipline[i]->Reload();
		}
		if (m_BinningClassifyPipline[i])
		{
			m_BinningClassifyPipline[i]->Reload();
		}
		if (m_BinningAllocatePipline[i])
		{
			m_BinningAllocatePipline[i]->Reload();
		}
		if (m_BinningScatterPipline[i])
		{
			m_BinningScatterPipline[i]->Reload();
		}
	}

	if (m_BasePassVertexShader)
	{
		m_BasePassVertexShader->Reload();
	}
	if (m_BasePassMeshShader)
	{
		m_BasePassMeshShader->Reload();
	}

	for (uint32_t k = 0; k < BINNIING_PIPELINE_COUNT; ++k)
	{
		for (size_t i = 0; i < m_BasePassFragmentShaders[k].size(); ++i)
		{
			m_BasePassFragmentShaders[k][i]->Reload();
			m_BinningPipelines[k][i]->Reload(false);
		}
	}

	if (m_DebugVertexShader)
	{
		m_DebugVertexShader->Reload();
	}
	if (m_DebugFragmentShader)
	{
		m_DebugFragmentShader->Reload();
	}
	if (m_DebugPipeline)
	{
		m_DebugPipeline->Reload(false);
	}

	return true;
}

KVirtualGeometryScene::InstancePtr KVirtualGeometryScene::CreateInstance(IKEntity* entity)
{
	auto it = m_InstanceMap.find(entity);
	if (it != m_InstanceMap.end())
	{
		assert(false && "shuold not reach");
		return it->second;
	}
	else
	{
		InstancePtr instance(KNEW Instance());
		instance->index = (uint32_t)m_Instances.size();
		instance->transform = glm::mat4(0.0f);
		m_Instances.push_back(instance);
		m_InstanceMap.insert({ entity, instance });
		return instance;
	}
}

KVirtualGeometryScene::InstancePtr KVirtualGeometryScene::GetInstance(IKEntity* entity)
{
	auto it = m_InstanceMap.find(entity);
	if (it != m_InstanceMap.end())
	{
		return it->second;
	}
	return nullptr;
}

bool KVirtualGeometryScene::AddInstance(IKEntity* entity, const glm::mat4& transform, KVirtualGeometryResourceRef resource)
{
	if (entity)
	{
		InstancePtr instance = CreateInstance(entity);
		instance->prevTransform = transform;
		instance->transform = transform;
		instance->resource = resource;
		return true;
	}
	return false;
}

bool KVirtualGeometryScene::TransformInstance(IKEntity* entity, const glm::mat4& transform)
{
	if (entity)
	{
		InstancePtr instance = GetInstance(entity);
		if (instance)
		{
			instance->transform = transform;
		}
		return true;
	}
	return false;
}

bool KVirtualGeometryScene::RemoveInstance(IKEntity* entity)
{
	auto it = m_InstanceMap.find(entity);
	if (it != m_InstanceMap.end())
	{
		uint32_t index = it->second->index;
		assert(index < m_Instances.size());
		for (uint32_t i = index + 1; i < m_Instances.size(); ++i)
		{
			--m_Instances[i]->index;
		}
		m_InstanceMap.erase(it);
		m_Instances.erase(m_Instances.begin() + index);
	}
	return true;
}