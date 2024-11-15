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

bool KShadowMap::Init(IKRenderDevice* renderDevice, uint32_t shadowMapSize)
{
	ASSERT_RESULT(UnInit());

	renderDevice->CreateSampler(m_ShadowSampler);
	m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_ShadowSampler->Init(0, 0);

	ASSERT_RESULT(renderDevice->CreateRenderTarget(m_RenderTarget));
	ASSERT_RESULT(m_RenderTarget->InitFromDepthStencil(shadowMapSize, shadowMapSize, 1, false));

	ASSERT_RESULT(renderDevice->CreateRenderPass(m_RenderPass));
	m_RenderPass->SetDepthStencilAttachment(m_RenderTarget->GetFrameBuffer());
	m_RenderPass->SetClearDepthStencil({ 1.0f, 0 });
	ASSERT_RESULT(m_RenderPass->Init());

	return true;
}

bool KShadowMap::UnInit()
{
	SAFE_UNINIT(m_RenderTarget);
	SAFE_UNINIT(m_RenderPass);
	SAFE_UNINIT(m_ShadowSampler);

	return true;
}

bool KShadowMap::UpdateShadowMap(IKCommandBufferPtr primaryBuffer)
{
	// 更新CBuffer
	{
		glm::mat4 view = m_Camera.GetViewMatrix();
		glm::mat4 proj = m_Camera.GetProjectiveMatrix();
		glm::vec4 parameters = glm::vec4(m_Camera.GetNear(), m_Camera.GetFar(), m_Camera.GetFov(), m_Camera.GetAspect());

		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_SHADOW);

		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_SHADOW);
		const KConstantDefinition::ConstantBufferDetail& details = KConstantDefinition::GetConstantBufferDetail(CBT_SHADOW);

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
			else if (detail.semantic == CS_SHADOW_CAMERA_PARAMETERS)
			{
				assert(sizeof(parameters) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				memcpy(pWritePos, &parameters, sizeof(parameters));
			}
		}
		shadowBuffer->Write(pData);
	}
	// 更新RenderTarget
	{
		std::vector<IKEntity*> cullRes;
		KRenderGlobal::Scene.GetVisibleEntities(m_Camera, cullRes);

		primaryBuffer->BeginDebugMarker("SM", glm::vec4(1));
		primaryBuffer->BeginRenderPass(m_RenderPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_RenderPass->GetViewPort());

		// Set depth bias (aka "Polygon offset")
		// Required to avoid shadow mapping artefacts
		primaryBuffer->SetDepthBias(m_DepthBiasConstant, 0, m_DepthBiasSlope);
		{
			KRenderCommandList commandList;
			for (IKEntity* entity : cullRes)
			{
				KTransformComponent* transform = nullptr;
				KRenderComponent* render = nullptr;
				if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform) && entity->GetComponent(CT_RENDER, (IKComponentBase**)&render))
				{
					const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();
					for (KMaterialSubMeshPtr materialSubMesh : materialSubMeshes)
					{
						KRenderCommand command;
						if (materialSubMesh->GetRenderCommand(RENDER_STAGE_SHADOW_GEN, command))
						{
							const KConstantDefinition::OBJECT & final = transform->FinalTransform();

							KDynamicConstantBufferUsage objectUsage;
							objectUsage.binding = SHADER_BINDING_OBJECT;
							objectUsage.range = sizeof(final);

							KRenderGlobal::DynamicConstantBufferManager.Alloc(&final, objectUsage);

							command.dynamicConstantUsages.push_back(objectUsage);

							commandList.push_back(command);
						}
					}
				}
			}

			for (KRenderCommand& command : commandList)
			{
				IKPipelineHandlePtr handle = nullptr;
				if (command.pipeline->GetHandle(m_RenderPass, handle))
				{
					command.pipelineHandle = handle;
					primaryBuffer->Render(command);
				}
			}
		}

		primaryBuffer->EndRenderPass();
		primaryBuffer->EndDebugMarker();
	}

	return true;
}