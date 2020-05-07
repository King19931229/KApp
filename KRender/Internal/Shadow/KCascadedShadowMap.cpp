#include "KCascadedShadowMap.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"

#include "Internal/ECS/Component/KTransformComponent.h"

KCascadedShadowMap::KCascadedShadowMap()
	: m_DepthBiasConstant(1.25f),
	m_DepthBiasSlope(1.75f)
{
	m_Camera.SetPosition(glm::vec3(-3000, 4600, 1800));
	m_Camera.LookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_Camera.SetOrtho(2000.0f, 2000.0f, -1000.0f, 1000.0f);
}

KCascadedShadowMap::~KCascadedShadowMap()
{
}

bool KCascadedShadowMap::Init(IKRenderDevice* renderDevice, size_t frameInFlight, size_t numCascaded, size_t shadowMapSize)
{
	ASSERT_RESULT(UnInit());

	renderDevice->CreateSampler(m_ShadowSampler);
	m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER);
	m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_ShadowSampler->Init(0, 0);

	if (numCascaded <= 4)
	{
		m_CascadedTargets.resize(numCascaded);
		for (RenderTargetList& targets : m_CascadedTargets)
		{
			targets.resize(frameInFlight);
			for (IKRenderTargetPtr& target : targets)
			{
				ASSERT_RESULT(renderDevice->CreateRenderTarget(target));
				ASSERT_RESULT(target->InitFromDepthStencil(shadowMapSize, shadowMapSize, false));
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool KCascadedShadowMap::UnInit()
{
	for (RenderTargetList& frameTargets : m_CascadedTargets)
	{
		for (IKRenderTargetPtr& target : frameTargets)
		{
			SAFE_UNINIT(target);
		}
		frameTargets.clear();
	}
	m_CascadedTargets.clear();

	SAFE_UNINIT(m_ShadowSampler);

	return true;
}

bool KCascadedShadowMap::UpdateShadowMap(IKRenderDevice* renderDevice, IKCommandBuffer* commandBuffer, size_t frameIndex)
{
	if (frameIndex < m_CascadedTargets.size())
	{
		RenderTargetList& targets = m_CascadedTargets[frameIndex];

		// 更新CBuffer
		{
			glm::mat4 view = m_Camera.GetViewMatrix();
			glm::mat4 proj = m_Camera.GetProjectiveMatrix();
			glm::vec2 near_far = glm::vec2(m_Camera.GetNear(), m_Camera.GetFar());

			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CASCADED_SHADOW);

			void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CASCADED_SHADOW);
			const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CASCADED_SHADOW);

			for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
			{
				void* pWritePos = nullptr;
				if (detail.semantic == CS_CASCADED_SHADOW_VIEW)
				{

				}
				else if (detail.semantic == CS_CASCADED_SHADOW_PROJ)
				{

				}
				else if (detail.semantic == CS_CASCADED_SHADOW_FRUSTRUM)
				{

				}
				else if (detail.semantic == CS_CASCADED_SHADOW_NEAR_FAR)
				{

				}
			}
			shadowBuffer->Write(pData);
		}
		// 更新RenderTarget
		{
			std::vector<KRenderComponent*> cullRes;
			KRenderGlobal::Scene.GetRenderComponent(m_Camera, cullRes);
			
			for(size_t cascaded = 0; cascaded < targets.size(); ++cascaded)
			{
				IKRenderTargetPtr shadowTarget = targets[frameIndex];

				commandBuffer->SetViewport(shadowTarget);
				// Set depth bias (aka "Polygon offset")
				// Required to avoid shadow mapping artefacts
				commandBuffer->SetDepthBias(m_DepthBiasConstant, 0, m_DepthBiasSlope);
				{
					KRenderCommandList commandList;
					for (KRenderComponent* component : cullRes)
					{
						IKEntity* entity = component->GetEntityHandle();
						KTransformComponent* transform = nullptr;
						if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
						{
							KMeshPtr mesh = component->GetMesh();

							mesh->Visit(PIPELINE_STAGE_SHADOW_GEN, frameIndex, [&](KRenderCommand command)
							{
								command.SetObjectData(transform->FinalTransform());
								commandList.push_back(command);
							});
						}
					}

					for (KRenderCommand& command : commandList)
					{
						IKPipelineHandlePtr handle = nullptr;
						if (command.pipeline->GetHandle(shadowTarget, handle))
						{
							command.pipelineHandle = handle;
							commandBuffer->Render(command);
						}
					}
				}
			}
		}

		return true;
	}
	return false;
}

IKRenderTargetPtr KCascadedShadowMap::GetShadowMapTarget(size_t cascadedIndex, size_t frameIndex)
{
	if (cascadedIndex < m_CascadedTargets.size())
	{
		RenderTargetList& list = m_CascadedTargets[cascadedIndex];
		if (frameIndex < list.size())
		{
			return list[frameIndex];
		}
	}
	return nullptr;
}