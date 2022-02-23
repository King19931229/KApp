#include "KGBuffer.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKStatistics.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Dispatcher/KRenderUtil.h"

KGBuffer::KGBuffer()
	: m_RenderDevice(nullptr)
	, m_Camera(nullptr)
{}

KGBuffer::~KGBuffer()
{
	ASSERT_RESULT(m_DepthStencilTarget == nullptr);	
	ASSERT_RESULT(m_MainPass == nullptr);
	ASSERT_RESULT(m_GBufferSampler == nullptr);
	for (uint32_t i = 0; i < GBUFFER_STAGE_COUNT; ++i)
	{
		ASSERT_RESULT(m_CommandBuffers[i].empty());
	}
}

bool KGBuffer::Init(IKRenderDevice* renderDevice, const KCamera* camera, uint32_t width, uint32_t height, size_t frameInFlight)
{
	ASSERT_RESULT(UnInit());
	
	m_RenderDevice = renderDevice;
	m_Camera = camera;

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateSampler(m_GBufferSampler);
	m_GBufferSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_GBufferSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_GBufferSampler->Init(0, 0);

	Resize(width, height);

	for (uint32_t i = 0; i < GBUFFER_STAGE_COUNT; ++i)
	{
		m_CommandBuffers[i].resize(frameInFlight);
		for (size_t frameIndex = 0; frameIndex < frameInFlight; ++frameIndex)
		{
			IKCommandBufferPtr& buffer = m_CommandBuffers[i][frameIndex];
			ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
			ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
		}
	}

	return true;
}

bool KGBuffer::UnInit()
{
	for (uint32_t i = 0; i < RT_COUNT; ++i)
	{
		SAFE_UNINIT(m_RenderTarget[i]);
	}
	SAFE_UNINIT(m_DepthStencilTarget);
	SAFE_UNINIT(m_PreZPass);
	SAFE_UNINIT(m_MainPass);

	for (uint32_t i = 0; i < GBUFFER_STAGE_COUNT; ++i)
	{
		for (IKCommandBufferPtr& buffer : m_CommandBuffers[i])
		{
			SAFE_UNINIT(buffer);
		}
		m_CommandBuffers[i].clear();
	}

	SAFE_UNINIT(m_GBufferSampler);
	SAFE_UNINIT(m_CommandPool);

	m_RenderDevice = nullptr;
	m_Camera = nullptr;

	return true;
}

bool KGBuffer::Resize(uint32_t width, uint32_t height)
{
	if (width && height && m_RenderDevice)
	{
		auto SAFE_CREATE_RT = [this](IKRenderTargetPtr& target)
		{
			if (target)
			{
				target->UnInit();
			}
			else
			{
				ASSERT_RESULT(m_RenderDevice->CreateRenderTarget(target));
			}
		};

		for (uint32_t i = 0; i < RT_COUNT; ++i)
		{
			SAFE_CREATE_RT(m_RenderTarget[i]);
		}

		SAFE_UNINIT(m_DepthStencilTarget);

		ASSERT_RESULT(m_RenderDevice->CreateRenderTarget(m_DepthStencilTarget));
		ASSERT_RESULT(m_DepthStencilTarget->InitFromDepthStencil(width, height, 1, true));

		SAFE_UNINIT(m_PreZPass);
		SAFE_UNINIT(m_MainPass);

		ASSERT_RESULT(m_RenderTarget[RT_NORMAL]->InitFromColor(width, height, 1, EF_R32G32B32A32_FLOAT));
		ASSERT_RESULT(m_RenderTarget[RT_POSITION]->InitFromColor(width, height, 1, EF_R32G32B32A32_FLOAT));
		ASSERT_RESULT(m_RenderTarget[RT_MOTION]->InitFromColor(width, height, 1, EF_R32G32_FLOAT));
		ASSERT_RESULT(m_RenderTarget[RT_DIFFUSE]->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM));
		ASSERT_RESULT(m_RenderTarget[RT_SPECULAR]->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM));

		ASSERT_RESULT(m_RenderDevice->CreateRenderPass(m_PreZPass));
		m_PreZPass->SetDepthStencilAttachment(m_DepthStencilTarget->GetFrameBuffer());

		ASSERT_RESULT(m_PreZPass->Init());

		ASSERT_RESULT(m_RenderDevice->CreateRenderPass(m_MainPass));
		m_MainPass->SetColorAttachment(RT_NORMAL, m_RenderTarget[RT_NORMAL]->GetFrameBuffer());
		m_MainPass->SetColorAttachment(RT_POSITION, m_RenderTarget[RT_POSITION]->GetFrameBuffer());
		m_MainPass->SetColorAttachment(RT_MOTION, m_RenderTarget[RT_MOTION]->GetFrameBuffer());
		m_MainPass->SetColorAttachment(RT_DIFFUSE, m_RenderTarget[RT_DIFFUSE]->GetFrameBuffer());
		m_MainPass->SetColorAttachment(RT_SPECULAR, m_RenderTarget[RT_SPECULAR]->GetFrameBuffer());

		m_MainPass->SetClearColor(RT_NORMAL, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_MainPass->SetClearColor(RT_POSITION, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_MainPass->SetClearColor(RT_MOTION, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_MainPass->SetClearColor(RT_DIFFUSE, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_MainPass->SetClearColor(RT_SPECULAR, { 0.0f, 0.0f, 0.0f, 1.0f });
		
		m_MainPass->SetDepthStencilAttachment(m_DepthStencilTarget->GetFrameBuffer());
		m_MainPass->SetClearDepthStencil({ 1.0f, 0 });
		m_MainPass->SetOpDepthStencil(LO_LOAD, SO_DONT_CARE, LO_LOAD, SO_DONT_CARE);

		ASSERT_RESULT(m_MainPass->Init());

		return true;
	}
	return false;
}

bool KGBuffer::UpdatePreDepth(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (m_Camera)
	{
		std::vector<KRenderCommand> commands;

		KRenderStageStatistics& statistics = m_Statistics[GBUFFER_STAGE_PRE_Z];
		statistics.Reset();

		std::vector<KRenderComponent*> cullRes;
		KRenderGlobal::Scene.GetRenderComponent(*m_Camera, false, cullRes);

		KRenderUtil::MeshInstanceGroup meshGroups;
		KRenderUtil::CalculateInstanceGroupByMesh(cullRes, meshGroups);

		for (auto& meshPair : meshGroups)
		{
			KMeshPtr mesh = meshPair.first;
			KRenderUtil::InstanceGroupPtr instanceGroup = meshPair.second;

			KRenderComponent* render = instanceGroup->render;
			std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = instanceGroup->instance;

			ASSERT_RESULT(render);
			ASSERT_RESULT(instances.size() > 0);

			if (instances.size() > 1)
			{
				render->Visit(PIPELINE_STAGE_PRE_Z_INSTANCE, frameIndex, [&](KRenderCommand& command)
				{
					++statistics.drawcalls;

					KVertexData* vertexData = const_cast<KVertexData*>(command.vertexData);

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

					for (size_t idx = 0; idx < instances.size(); ++idx)
					{
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
					}

					command.pipeline->GetHandle(m_PreZPass, command.pipelineHandle);

					if (command.Complete())
					{
						commands.push_back(std::move(command));
					}
				});
			}
			else
			{
				render->Visit(PIPELINE_STAGE_PRE_Z, frameIndex, [&](KRenderCommand& command)
				{
					for (size_t idx = 0; idx < instances.size(); ++idx)
					{
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

						const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[idx];

						KConstantDefinition::OBJECT objectData;
						objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
						objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));
						command.objectUsage.binding = SHADER_BINDING_OBJECT;
						command.objectUsage.range = sizeof(objectData);

						KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

						command.pipeline->GetHandle(m_PreZPass, command.pipelineHandle);

						if (command.Complete())
						{
							commands.push_back(std::move(command));
						}
					}
				});
			}
		}

		IKCommandBufferPtr commandBuffer = m_CommandBuffers[GBUFFER_STAGE_PRE_Z][frameIndex];

		primaryBuffer->BeginDebugMarker("PreZ", glm::vec4(0, 1, 0, 0));
		primaryBuffer->BeginRenderPass(m_PreZPass, SUBPASS_CONTENTS_SECONDARY);

		commandBuffer->BeginSecondary(m_PreZPass);
		commandBuffer->SetViewport(m_PreZPass->GetViewPort());

		{
			for (KRenderCommand& command : commands)
			{
				IKPipelineHandlePtr handle = nullptr;
				if (command.pipeline->GetHandle(m_MainPass, handle))
				{
					commandBuffer->Render(command);
				}
			}
		}

		commandBuffer->End();

		primaryBuffer->Execute(commandBuffer);
		primaryBuffer->EndRenderPass();
		primaryBuffer->EndDebugMarker();

		KRenderGlobal::Statistics.UpdateRenderStageStatistics(KRenderGlobal::ALL_STAGE_NAMES[RENDER_STAGE_PRE_Z], statistics);

		return true;
	}
	return false;
}

bool KGBuffer::UpdateGBuffer(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (m_Camera)
	{
		std::vector<KRenderCommand> commands;

		KRenderStageStatistics& statistics = m_Statistics[GBUFFER_STAGE_DEFAULT];
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

				if (instances.size() > 1)
				{
					render->Visit(PIPELINE_STAGE_GBUFFER_INSTANCE, frameIndex, [&](KRenderCommand& _command)
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

						command.pipeline->GetHandle(m_MainPass, command.pipelineHandle);

						if (command.Complete())
						{
							commands.push_back(std::move(command));
						}
					});
				}
				else
				{
					render->Visit(PIPELINE_STAGE_GBUFFER, frameIndex, [&](KRenderCommand& _command)
					{
						KRenderCommand command = std::move(_command);

						if (!KRenderUtil::AssignShadingParameter(command, material.get(), false))
						{
							return;
						}

						++statistics.drawcalls;

						const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[0];

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

						command.pipeline->GetHandle(m_MainPass, command.pipelineHandle);

						if (command.Complete())
						{
							commands.push_back(std::move(command));
						}
					});
				}
			}
		}

		IKCommandBufferPtr commandBuffer = m_CommandBuffers[GBUFFER_STAGE_DEFAULT][frameIndex];

		primaryBuffer->BeginDebugMarker("GBuffer", glm::vec4(0, 1, 0, 0));
		primaryBuffer->BeginRenderPass(m_MainPass, SUBPASS_CONTENTS_SECONDARY);

		commandBuffer->BeginSecondary(m_MainPass);
		commandBuffer->SetViewport(m_MainPass->GetViewPort());

		{
			for (KRenderCommand& command : commands)
			{
				IKPipelineHandlePtr handle = nullptr;
				if (command.pipeline->GetHandle(m_MainPass, handle))
				{
					commandBuffer->Render(command);
				}
			}
		}

		commandBuffer->End();

		primaryBuffer->Execute(commandBuffer);
		primaryBuffer->EndRenderPass();
		primaryBuffer->EndDebugMarker();

		KRenderGlobal::Statistics.UpdateRenderStageStatistics(KRenderGlobal::ALL_STAGE_NAMES[RENDER_STAGE_GBUFFER], statistics);

		return true;
	}
	return false;
}