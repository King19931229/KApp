#include "KRayTraceScene.h"
#include "KRayTraceManager.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

KRayTraceScene::KRayTraceScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_Pipeline(nullptr)
	, m_DebugPipeline(nullptr)
	, m_DebugClip(glm::mat4(1.0f))
	, m_DebugEnable(false)
{
}

KRayTraceScene::~KRayTraceScene()
{
	ASSERT_RESULT(!m_Scene);
	ASSERT_RESULT(!m_Camera);
	ASSERT_RESULT(!m_Pipeline);
}

bool KRayTraceScene::GetDebugRenderCommand(KRenderCommandList& commands)
{
	if (m_Pipeline)
	{
		if (m_DebugEnable)
		{
			m_DebugClip = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
			m_DebugClip = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f)) * m_DebugClip;
			m_DebugClip = glm::scale(glm::mat4(1.0f), glm::vec3(m_DebugRect.w, m_DebugRect.h, 1.0f)) * m_DebugClip;
			m_DebugClip = glm::translate(glm::mat4(1.0f), glm::vec3(m_DebugRect.x, m_DebugRect.y, 0.0f)) * m_DebugClip;
			m_DebugClip = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)) * m_DebugClip;
			m_DebugClip = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)) * m_DebugClip;

			KRenderCommand command;

			IKRenderTargetPtr rayTraceTarget = m_Pipeline->GetStorageTarget();

			if (!m_DebugPipeline)
			{
				IKPipelinePtr& pipeline = m_DebugPipeline;

				KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

				pipeline->SetVertexBinding(KRenderGlobal::RayTraceManager.ms_VertexFormats, ARRAY_SIZE(KRenderGlobal::RayTraceManager.ms_VertexFormats));
				pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

				pipeline->SetBlendEnable(true);
				pipeline->SetCullMode(CM_BACK);
				pipeline->SetFrontFace(FF_CLOCKWISE);

				pipeline->SetDepthFunc(CF_ALWAYS, false, false);
				pipeline->SetShader(ST_VERTEX, KRenderGlobal::RayTraceManager.m_DebugVertexShader);
				pipeline->SetShader(ST_FRAGMENT, KRenderGlobal::RayTraceManager.m_DebugFragmentShader);
				pipeline->SetSampler(SHADER_BINDING_TEXTURE0, rayTraceTarget, KRenderGlobal::RayTraceManager.m_DebugSampler);

				ASSERT_RESULT(pipeline->Init());
			}

			command.vertexData = &KRenderGlobal::RayTraceManager.m_DebugVertexData;
			command.indexData = &KRenderGlobal::RayTraceManager.m_DebugIndexData;
			command.pipeline = m_DebugPipeline;
			command.indexDraw = true;

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(m_DebugClip);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&m_DebugClip, command.objectUsage);

			commands.push_back(std::move(command));
		}
		return true;
	}
	return false;
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
	SAFE_UNINIT(m_DebugPipeline);
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

bool KRayTraceScene::EnableDebugDraw(float x, float y, float width, float height)
{
	m_DebugRect.x = x;
	m_DebugRect.y = y;
	m_DebugRect.w = width;
	m_DebugRect.h = height;
	m_DebugEnable = true;
	return true;
}

bool KRayTraceScene::DisableDebugDraw()
{
	m_DebugEnable = false;
	return true;
}

bool KRayTraceScene::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (m_Pipeline)
	{
		return m_Pipeline->Execute(primaryBuffer, frameIndex);
	}
	return false;
}