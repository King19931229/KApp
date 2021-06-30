#include "KRayTraceScene.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

KRayTraceScene::KRayTraceScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_Pipeline(nullptr)
{
}

KRayTraceScene::~KRayTraceScene()
{
	ASSERT_RESULT(!m_Scene);
	ASSERT_RESULT(!m_Camera);
	ASSERT_RESULT(!m_Pipeline);
}

bool KRayTraceScene::Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline)
{
	UnInit();

	if (scene && camera && pipeline)
	{
		m_Scene = scene;
		m_Pipeline = pipeline;
		m_Camera = camera;

		uint32_t numFrames = KRenderGlobal::RenderDevice->GetNumFramesInFlight();
		m_CameraBuffers.resize(numFrames);
		for (uint32_t i = 0; i < numFrames; ++i)
		{
			IKUniformBufferPtr& cameraBuffer = m_CameraBuffers[i];
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateUniformBuffer(cameraBuffer));

			Camera cam;
			cam.view = m_Camera->GetViewMatrix();
			cam.proj = m_Camera->GetProjectiveMatrix();
			cam.viewInv = glm::inverse(cam.view);
			cam.projInv = glm::inverse(cam.proj);

			ASSERT_RESULT(cameraBuffer->InitMemory(sizeof(cam), &cam));
			ASSERT_RESULT(cameraBuffer->InitDevice());
		}

		std::vector<IKEntityPtr> entites;
		scene->GetAllEntities(entites);

		pipeline->ClearBottomLevelAS();

		for (IKEntityPtr& entity : entites)
		{
			IKRenderComponent* renderComponent = nullptr;
			IKTransformComponent* transformComponent = nullptr;
			if (entity->GetComponentBase(CT_RENDER, (IKComponentBase**)&renderComponent) && entity->GetComponentBase(CT_TRANSFORM, (IKComponentBase**)&transformComponent))
			{
				std::vector<IKAccelerationStructurePtr> subAS;
				renderComponent->GetAllAccelerationStructure(subAS);
				const glm::mat4& transform = transformComponent->GetFinal();
				m_Entites[entity.get()] = std::make_tuple(subAS, transform);

				for (IKAccelerationStructurePtr as : subAS)
				{
					pipeline->AddBottomLevelAS(as, transform);
				}
			}
		}

		pipeline->Init(m_CameraBuffers);

		return true;
	}
	return false;
}

bool KRayTraceScene::UnInit()
{
	m_Scene = nullptr;
	m_Camera = nullptr;
	m_Entites.clear();
	SAFE_UNINIT(m_Pipeline);
	SAFE_UNINIT_CONTAINER(m_CameraBuffers);
	return true;
}

bool KRayTraceScene::UpdateCamera(uint32_t frameIndex)
{
	if (m_Camera && frameIndex < m_CameraBuffers.size())
	{
		IKUniformBufferPtr& cameraBuffer = m_CameraBuffers[frameIndex];
		if (cameraBuffer)
		{
			Camera cam;
			cam.view = m_Camera->GetViewMatrix();
			cam.proj = m_Camera->GetProjectiveMatrix();
			cam.viewInv = glm::inverse(cam.view);
			cam.projInv = glm::inverse(cam.proj);
			cameraBuffer->Write(&cam);
			return true;
		}
	}
	return false;
}

bool KRayTraceScene::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (m_Pipeline)
	{
		return m_Pipeline->Execute(primaryBuffer, frameIndex);
	}
	return false;
}