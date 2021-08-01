#include "KGBuffer.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKStatistics.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Dispatcher/KInstancePreparer.h"

KGBuffer::KGBuffer()
	: m_RenderDevice(nullptr)
	, m_Camera(nullptr)
{}

KGBuffer::~KGBuffer()
{
	ASSERT_RESULT(m_RenderTarget0 == nullptr);
	ASSERT_RESULT(m_RenderTarget1 == nullptr);
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
	SAFE_UNINIT(m_RenderTarget0);
	SAFE_UNINIT(m_RenderTarget1);
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
		SAFE_UNINIT(m_RenderTarget0);
		SAFE_UNINIT(m_RenderTarget1);
		SAFE_UNINIT(m_DepthStencilTarget);
		SAFE_UNINIT(m_RenderPass);

		ASSERT_RESULT(m_RenderDevice->CreateRenderTarget(m_RenderTarget0));
		ASSERT_RESULT(m_RenderTarget0->InitFromColor(width, height, 1, EF_R16G16B16A16_FLOAT));

		ASSERT_RESULT(m_RenderDevice->CreateRenderTarget(m_RenderTarget1));
		ASSERT_RESULT(m_RenderTarget1->InitFromColor(width, height, 1, EF_R16G16B16A16_FLOAT));

		ASSERT_RESULT(m_RenderDevice->CreateRenderPass(m_RenderPass));
		m_RenderPass->SetColorAttachment(0, m_RenderTarget0->GetFrameBuffer());
		m_RenderPass->SetColorAttachment(1, m_RenderTarget1->GetFrameBuffer());
		m_RenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 1.0f });

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

		KInstancePreparer::MeshGroups meshGroups;
		KInstancePreparer::CalculateByMesh(cullRes, meshGroups);

		for (auto& pair : meshGroups)
		{
			KMeshPtr mesh = pair.first;
			KInstancePreparer::InstanceGroupPtr instanceGroup = pair.second;

			KRenderComponent* render = instanceGroup->render;
			std::vector<KConstantDefinition::OBJECT>& instances = instanceGroup->instance;

			ASSERT_RESULT(render);
			ASSERT_RESULT(!instances.empty());

			if (instances.size() > 1)
			{
				render->Visit(PIPELINE_STAGE_GBUFFER_INSTANCE, frameIndex, [&](KRenderCommand& _command)
				{
					KRenderCommand command = std::move(_command);

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

					++m_Statistics.drawcalls;

					const KConstantDefinition::OBJECT & final = instances[0];

					command.objectUsage.binding = SHADER_BINDING_OBJECT;
					command.objectUsage.range = sizeof(final);
					KRenderGlobal::DynamicConstantBufferManager.Alloc(&final, command.objectUsage);

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