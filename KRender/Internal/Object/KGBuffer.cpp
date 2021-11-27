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
	ASSERT_RESULT(m_RenderPass == nullptr);
	ASSERT_RESULT(m_GBufferSampler == nullptr);
	ASSERT_RESULT(m_CommandBuffers.empty());
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

	m_CommandBuffers.resize(frameInFlight);
	for (size_t i = 0; i < frameInFlight; ++i)
	{
		IKCommandBufferPtr& buffer = m_CommandBuffers[i];
		ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
		ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
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
	SAFE_UNINIT(m_RenderPass);

	for (IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_CommandBuffers.clear();

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
		SAFE_UNINIT(m_RenderPass);
		ASSERT_RESULT(m_RenderTarget[RT_NORMAL]->InitFromColor(width, height, 1, EF_R32G32B32A32_FLOAT));
		ASSERT_RESULT(m_RenderTarget[RT_POSITION]->InitFromColor(width, height, 1, EF_R32G32B32A32_FLOAT));
		ASSERT_RESULT(m_RenderTarget[RT_MOTION]->InitFromColor(width, height, 1, EF_R32G32_FLOAT));
		ASSERT_RESULT(m_RenderTarget[RT_DIFFUSE]->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM));
		ASSERT_RESULT(m_RenderTarget[RT_SPECULAR]->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM));

		ASSERT_RESULT(m_RenderDevice->CreateRenderPass(m_RenderPass));
		m_RenderPass->SetColorAttachment(RT_NORMAL, m_RenderTarget[RT_NORMAL]->GetFrameBuffer());
		m_RenderPass->SetColorAttachment(RT_POSITION, m_RenderTarget[RT_POSITION]->GetFrameBuffer());
		m_RenderPass->SetColorAttachment(RT_MOTION, m_RenderTarget[RT_MOTION]->GetFrameBuffer());
		m_RenderPass->SetColorAttachment(RT_DIFFUSE, m_RenderTarget[RT_DIFFUSE]->GetFrameBuffer());
		m_RenderPass->SetColorAttachment(RT_SPECULAR, m_RenderTarget[RT_SPECULAR]->GetFrameBuffer());

		m_RenderPass->SetClearColor(RT_NORMAL, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_RenderPass->SetClearColor(RT_POSITION, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_RenderPass->SetClearColor(RT_MOTION, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_RenderPass->SetClearColor(RT_DIFFUSE, { 0.0f, 0.0f, 0.0f, 1.0f });
		m_RenderPass->SetClearColor(RT_SPECULAR, { 0.0f, 0.0f, 0.0f, 1.0f });

		m_RenderDevice->CreateRenderTarget(m_DepthStencilTarget);
		m_DepthStencilTarget->InitFromDepthStencil(width, height, 1, true);
		m_RenderPass->SetDepthStencilAttachment(m_DepthStencilTarget->GetFrameBuffer());
		m_RenderPass->SetClearDepthStencil({ 1.0f, 0 });

		ASSERT_RESULT(m_RenderPass->Init());

		return true;
	}
	return false;
}

bool KGBuffer::UpdateGBuffer(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (m_Camera)
	{
		std::vector<KRenderCommand> commands;

		m_Statistics.Reset();

		std::vector<KRenderComponent*> cullRes;
		KRenderGlobal::Scene.GetRenderComponent(*m_Camera, cullRes);

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

						++m_Statistics.drawcalls;

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
							m_Statistics.faces += command.indexData->indexCount / 3;
						}
						else
						{
							m_Statistics.faces += command.vertexData->vertexCount / 3;
						}

						command.pipeline->GetHandle(m_RenderPass, command.pipelineHandle);

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

						++m_Statistics.drawcalls;

						const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[0];

						KConstantDefinition::OBJECT objectData;
						objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
						objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));
						command.objectUsage.binding = SHADER_BINDING_OBJECT;
						command.objectUsage.range = sizeof(objectData);
						KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

						if (command.indexDraw)
						{
							m_Statistics.faces += command.indexData->indexCount / 3;
						}
						else
						{
							m_Statistics.faces += command.vertexData->vertexCount / 3;
						}

						command.pipeline->GetHandle(m_RenderPass, command.pipelineHandle);

						if (command.Complete())
						{
							commands.push_back(std::move(command));
						}
					});
				}
			}
		}

		IKCommandBufferPtr commandBuffer = m_CommandBuffers[frameIndex];

		primaryBuffer->BeginRenderPass(m_RenderPass, SUBPASS_CONTENTS_SECONDARY);

		commandBuffer->BeginSecondary(m_RenderPass);
		commandBuffer->SetViewport(m_RenderPass->GetViewPort());

		{
			for (KRenderCommand& command : commands)
			{
				IKPipelineHandlePtr handle = nullptr;
				if (command.pipeline->GetHandle(m_RenderPass, handle))
				{
					commandBuffer->Render(command);
				}
			}
		}

		commandBuffer->End();

		primaryBuffer->Execute(commandBuffer);
		primaryBuffer->EndRenderPass();

		KRenderGlobal::Statistics.UpdateRenderStageStatistics(KRenderGlobal::ALL_STAGE_NAMES[RENDER_STAGE_GBUFFER], m_Statistics);

		return true;
	}
	return false;
}