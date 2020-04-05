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

bool KShadowMap::Init(IKRenderDevice* renderDevice,	size_t frameInFlight, size_t shadowMapSize)
{
	ASSERT_RESULT(UnInit());

	renderDevice->CreateSampler(m_ShadowSampler);
	m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER);
	m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_ShadowSampler->Init(0, 0);

	size_t numImages = frameInFlight;
	m_RenderTargets.resize(numImages);

	for(size_t i = 0; i < numImages; ++i)
	{
		IKRenderTargetPtr& target = m_RenderTargets[i];
		ASSERT_RESULT(renderDevice->CreateRenderTarget(target));
		ASSERT_RESULT(target->InitFromDepthStencil(shadowMapSize, shadowMapSize, false));
	}

	return true;
}

bool KShadowMap::UnInit()
{
	for(IKRenderTargetPtr target : m_RenderTargets)
	{
		target->UnInit();
		target = nullptr;
	}
	m_RenderTargets.clear();

	if(m_ShadowSampler)
	{
		m_ShadowSampler->UnInit();
		m_ShadowSampler = nullptr;
	}

	return true;
}

bool KShadowMap::UpdateShadowMap(IKRenderDevice* renderDevice, IKCommandBuffer* commandBuffer, size_t frameIndex)
{
	if(frameIndex < m_RenderTargets.size())
	{
		// 更新CBuffer
		{
			glm::mat4 view = m_Camera.GetViewMatrix();
			glm::mat4 proj = m_Camera.GetProjectiveMatrix();
			glm::vec2 near_far = glm::vec2(m_Camera.GetNear(), m_Camera.GetFar());

			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_SHADOW);
			void* pWritePos = nullptr;
			void* pData = KConstantGlobal::GetGlobalConstantData(CBT_SHADOW);	
			const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_SHADOW);
			for(KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
			{
				if(detail.semantic == CS_SHADOW_VIEW)
				{
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					assert(sizeof(view) == detail.size);
					memcpy(pWritePos, &view, sizeof(view));
				}
				else if(detail.semantic == CS_SHADOW_PROJ)
				{
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					assert(sizeof(proj) == detail.size);
					memcpy(pWritePos, &proj, sizeof(proj));
				}
				else if(detail.semantic == CS_SHADOW_NEAR_FAR)
				{
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					assert(sizeof(near_far) == detail.size);
					memcpy(pWritePos, &near_far, sizeof(near_far));
				}
			}
			shadowBuffer->Write(pData);
		}
		// 更新RenderTarget
		{
			std::vector<KRenderComponent*> cullRes;
			KRenderGlobal::Scene.GetRenderComponent(m_Camera, cullRes);

			IKRenderTargetPtr shadowTarget = m_RenderTargets[frameIndex];

			commandBuffer->SetViewport(shadowTarget);
			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			commandBuffer->SetDepthBias(m_DepthBiasConstant, 0, m_DepthBiasSlope);
			{
				KRenderCommandList commandList;
				for(KRenderComponent* component : cullRes)
				{
					IKEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if(entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_SHADOW_GEN, frameIndex, [&](KRenderCommand command)
						{
							command.SetObjectData(transform->FinalTransform());
							commandList.push_back(command);
						});
					}
				}

				for(KRenderCommand& command : commandList)
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

		return true;
	}
	return false;
}

IKRenderTargetPtr KShadowMap::GetShadowMapTarget(size_t frameIndex)
{
	if(frameIndex < m_RenderTargets.size())
	{
		return m_RenderTargets[frameIndex];
	}
	return nullptr;
}