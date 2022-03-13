#include "KRayTraceScene.h"
#include "KRayTraceManager.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

KRayTraceScene::KRayTraceScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_Pipeline(nullptr)
	, m_DebugClip(glm::mat4(1.0f))
	, m_Width(1024)
	, m_Height(1024)
	, m_ImageScale(1.0f)
	, m_DebugEnable(false)
	, m_AutoUpdateImageSize(false)
{
	m_OnSceneChangedFunc = std::bind(&KRayTraceScene::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KRayTraceScene::~KRayTraceScene()
{
	ASSERT_RESULT(!m_Scene);
	ASSERT_RESULT(!m_Camera);
	ASSERT_RESULT(!m_Pipeline);
}

bool KRayTraceScene::GetDebugRenderCommand(KRenderCommandList& commands)
{
	return m_DebugDrawer.GetDebugRenderCommand(commands);
}

void KRayTraceScene::OnSceneChanged(EntitySceneOp op, IKEntityPtr entity)
{
	if (m_Pipeline)
	{
		IKRenderComponent* renderComponent = nullptr;
		IKTransformComponent* transformComponent = nullptr;

		if (op == ESO_REMOVE || op == ESO_MOVE)
		{
			auto it = m_ASHandles.find(entity);
			if (it != m_ASHandles.end())
			{
				for (uint32_t handle : it->second)
				{
					m_Pipeline->RemoveBottomLevelAS(handle);
				}
				m_ASHandles.erase(it);
				m_Pipeline->MarkASNeedUpdate();
			}
		}

		if (op != ESO_REMOVE)
		{
			if (entity->GetComponentBase(CT_RENDER, (IKComponentBase**)&renderComponent) && entity->GetComponentBase(CT_TRANSFORM, (IKComponentBase**)&transformComponent))
			{
				std::vector<IKAccelerationStructurePtr> subAS;
				renderComponent->GetAllAccelerationStructure(subAS);
				const glm::mat4& transform = transformComponent->GetFinal();

				for (IKAccelerationStructurePtr as : subAS)
				{
					uint32_t handle = m_Pipeline->AddBottomLevelAS(as, transform);
					m_ASHandles[entity].insert(handle);
					m_Pipeline->MarkASNeedUpdate();
				}
			}
		}
	}
}

bool KRayTraceScene::Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline)
{
	UnInit();

	if (scene && camera && pipeline)
	{
		m_Scene = scene;
		m_Pipeline = pipeline;
		m_Camera = camera;

		IKUniformBufferPtr& cameraBuffer = m_CameraBuffer;
		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateUniformBuffer(cameraBuffer));

		Camera cam;
		cam.view = m_Camera->GetViewMatrix();
		cam.proj = m_Camera->GetProjectiveMatrix();
		cam.viewInv = glm::inverse(cam.view);
		cam.projInv = glm::inverse(cam.proj);
		cam.parameters = glm::vec4(m_Camera->GetNear(), m_Camera->GetFar(), m_Camera->GetFov(), m_Camera->GetAspect());

		ASSERT_RESULT(cameraBuffer->InitMemory(sizeof(cam), &cam));
		ASSERT_RESULT(cameraBuffer->InitDevice());

		std::vector<IKEntityPtr> entites;
		scene->GetAllEntities(entites);

		pipeline->ClearBottomLevelAS();

		for (IKEntityPtr& entity : entites)
		{
			OnSceneChanged(ESO_ADD, entity);
		}

		pipeline->Init(m_CameraBuffer, m_Width, m_Height);
		m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);

		m_DebugDrawer.Init(m_Pipeline->GetStorageTarget(), 0, 0, 1, 1);

		return true;
	}
	return false;
}

bool KRayTraceScene::UnInit()
{
	m_DebugDrawer.UnInit();
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
	}
	m_Scene = nullptr;
	m_Camera = nullptr;
	m_ASHandles.clear();
	SAFE_UNINIT(m_Pipeline);
	SAFE_UNINIT(m_CameraBuffer);
	return true;
}

bool KRayTraceScene::UpdateCamera()
{
	IKUniformBufferPtr& cameraBuffer = m_CameraBuffer;
	if (cameraBuffer)
	{
		Camera cam;
		cam.view = m_Camera->GetViewMatrix();
		cam.proj = m_Camera->GetProjectiveMatrix();
		cam.viewInv = glm::inverse(cam.view);
		cam.projInv = glm::inverse(cam.proj);
		cameraBuffer->Write(&cam);
	}
	return true;
}

bool KRayTraceScene::EnableDebugDraw()
{
	m_DebugDrawer.EnableDraw();
	return true;
}

bool KRayTraceScene::DisableDebugDraw()
{
	m_DebugDrawer.DisableDraw();
	return true;
}

bool KRayTraceScene::EnableAutoUpdateImageSize(float scale)
{
	m_ImageScale = scale;
	m_AutoUpdateImageSize = true;
	UpdateSize();
	return true;
}

bool KRayTraceScene::EnableCustomImageSize(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;
	m_AutoUpdateImageSize = false;
	return true;
}

void KRayTraceScene::UpdateSize()
{
	if (m_AutoUpdateImageSize)
	{
		IKSwapChain* chain = KRenderGlobal::RenderDevice->GetSwapChain();
		if (chain->GetWidth() && chain->GetHeight())
		{
			m_Width = static_cast<uint32_t>(static_cast<float>(chain->GetWidth()) * m_ImageScale);
			m_Height = static_cast<uint32_t>(static_cast<float>(chain->GetHeight()) * m_ImageScale);
			if (m_Pipeline)
			{
				m_Pipeline->ResizeImage(m_Width, m_Height);
			}
		}
	}
}

void KRayTraceScene::ReloadShader()
{
	if (m_Pipeline)
	{
		m_Pipeline->ReloadShader();
	}
}

bool KRayTraceScene::Execute(IKCommandBufferPtr primaryBuffer)
{
	if (m_Pipeline)
	{
		return m_Pipeline->Execute(primaryBuffer);
	}
	return false;
}

IKRayTracePipeline* KRayTraceScene::GetRayTracePipeline()
{
	return m_Pipeline ? m_Pipeline.get() : nullptr;
}

const KCamera* KRayTraceScene::GetCamera()
{
	return m_Camera;
}