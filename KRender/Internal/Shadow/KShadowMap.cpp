#include "KShadowMap.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"

#include "Internal/ECS/Component/KTransformComponent.h"

KShadowMap::KShadowMap() : 
	m_DepthBiasConstant(1.25f),
	m_DepthBiasSlope(1.75f)
{
	m_Camera.SetPosition(glm::vec3(-3000, 4600, 1800));
	m_Camera.LookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_Camera.SetPerspective(45.0f, 1.0f, 1.0f, 10000.0f);
}

KShadowMap::~KShadowMap()
{
}

bool KShadowMap::Init(IKRenderDevice* renderDevice, size_t frameInFlight, uint32_t shadowMapSize)
{
	ASSERT_RESULT(UnInit());

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateSampler(m_ShadowSampler);
	m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER);
	m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_ShadowSampler->Init(0, 0);

	ASSERT_RESULT(renderDevice->CreateRenderTarget(m_RenderTarget));
	ASSERT_RESULT(m_RenderTarget->InitFromDepthStencil(shadowMapSize, shadowMapSize, 1, false));

	ASSERT_RESULT(renderDevice->CreateRenderPass(m_RenderPass));
	m_RenderPass->SetDepthStencilAttachment(m_RenderTarget->GetFrameBuffer());
	m_RenderPass->SetClearDepthStencil({ 1.0f, 0 });
	ASSERT_RESULT(m_RenderPass->Init());

	m_CommandBuffers.resize(frameInFlight);
	for (size_t i = 0; i < frameInFlight; ++i)
	{
		IKCommandBufferPtr& buffer = m_CommandBuffers[i];
		ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
		ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
	}

	return true;
}

bool KShadowMap::UnInit()
{
	SAFE_UNINIT(m_RenderTarget);
	SAFE_UNINIT(m_RenderPass);

	for (IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_CommandBuffers.clear();

	SAFE_UNINIT(m_ShadowSampler);
	SAFE_UNINIT(m_CommandPool);

	return true;
}

bool KShadowMap::UpdateShadowMap(size_t frameIndex, IKCommandBufferPtr primaryBuffer)
{
	if(frameIndex < m_CommandBuffers.size())
	{
		// 更新CBuffer
		{
			glm::mat4 view = m_Camera.GetViewMatrix();
			glm::mat4 proj = m_Camera.GetProjectiveMatrix();
			glm::vec2 near_far = glm::vec2(m_Camera.GetNear(), m_Camera.GetFar());

			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_SHADOW);

			void* pData = KConstantGlobal::GetGlobalConstantData(CBT_SHADOW);
			const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_SHADOW);

			for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
			{
				assert(detail.offset % detail.size == 0);
				void* pWritePos = nullptr;
				if (detail.semantic == CS_SHADOW_VIEW)
				{
					assert(sizeof(view) == detail.size);
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					memcpy(pWritePos, &view, sizeof(view));
				}
				else if (detail.semantic == CS_SHADOW_PROJ)
				{
					assert(sizeof(proj) == detail.size);
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					memcpy(pWritePos, &proj, sizeof(proj));
				}
				else if (detail.semantic == CS_SHADOW_NEAR_FAR)
				{
					assert(sizeof(near_far) == detail.size);
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					memcpy(pWritePos, &near_far, sizeof(near_far));
				}
			}
			shadowBuffer->Write(pData);
		}
		// 更新RenderTarget
		{
			std::vector<KRenderComponent*> cullRes;
			KRenderGlobal::Scene.GetRenderComponent(m_Camera, cullRes);

			IKCommandBufferPtr commandBuffer = m_CommandBuffers[frameIndex];

			primaryBuffer->BeginRenderPass(m_RenderPass, SUBPASS_CONTENTS_SECONDARY);

			commandBuffer->BeginSecondary(m_RenderPass);
			commandBuffer->SetViewport(m_RenderPass->GetViewPort());

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			commandBuffer->SetDepthBias(m_DepthBiasConstant, 0, m_DepthBiasSlope);
			{
				KRenderCommandList commandList;
				for (KRenderComponent* component : cullRes)
				{
					IKEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					KRenderComponent* render = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform) && entity->GetComponent(CT_RENDER, (IKComponentBase**)&render))
					{
						KMeshPtr mesh = component->GetMesh();

						render->Visit(PIPELINE_STAGE_SHADOW_GEN, frameIndex, [&](KRenderCommand command)
						{
							const KConstantDefinition::OBJECT& final = transform->FinalTransform();

							command.objectUsage.binding = SHADER_BINDING_OBJECT;
							command.objectUsage.range = sizeof(final);
							KRenderGlobal::DynamicConstantBufferManager.Alloc(&final, command.objectUsage);

							commandList.push_back(command);
						});
					}
				}

				for (KRenderCommand& command : commandList)
				{
					IKPipelineHandlePtr handle = nullptr;
					if (command.pipeline->GetHandle(m_RenderPass, handle))
					{
						command.pipelineHandle = handle;
						commandBuffer->Render(command);
					}
				}
			}

			commandBuffer->End();

			primaryBuffer->Execute(commandBuffer);
			primaryBuffer->EndRenderPass();
		}

		return true;
	}
	return false;
}

IKRenderTargetPtr KShadowMap::GetShadowMapTarget()
{
	if(m_RenderTarget)
	{
		return m_RenderTarget;
	}
	return nullptr;
}