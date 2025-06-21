#include "KRayTraceScene.h"
#include "KRayTraceManager.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

KRayTraceScene::KRayTraceScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_DebugClip(glm::mat4(1.0f))
	, m_Width(1024)
	, m_Height(1024)
	, m_LastDirtyFrame(0)
	, m_LastRecreateFrame(0)
	, m_ImageScale(1.0f)
	, m_DebugEnable(false)
	, m_AutoUpdateImageSize(false)
	, m_bNeedRecreateAS(false)
{
	m_OnSceneChangedFunc = std::bind(&KRayTraceScene::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
	m_OnRenderComponentChangedFunc = std::bind(&KRayTraceScene::OnRenderComponentChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KRayTraceScene::~KRayTraceScene()
{
	ASSERT_RESULT(!m_Scene);
	ASSERT_RESULT(!m_Camera);
	ASSERT_RESULT(m_RaytracePipelineInfos.empty());
}

uint32_t KRayTraceScene::AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform, IKMaterialTextureBindingPtr textureBinding)
{
	uint32_t handle = m_Handles.NewHandle();
	IKAccelerationStructure::BottomASTransformTuple tuple;
	tuple.as = as;
	tuple.transform = transform;
	tuple.tex = textureBinding;
	m_BottomASMap[handle] = tuple;
	return handle;
}

bool KRayTraceScene::RemoveBottomLevelAS(uint32_t handle)
{
	auto it = m_BottomASMap.find(handle);
	if (it != m_BottomASMap.end())
	{
		m_BottomASMap.erase(it);
		m_Handles.ReleaseHandle(handle);
	}
	return true;
}

bool KRayTraceScene::TransformBottomLevelAS(uint32_t handle, const glm::mat4& transform)
{
	auto it = m_BottomASMap.find(handle);
	if (it != m_BottomASMap.end())
	{
		it->second.transform = transform;
		return true;
	}
	return false;
}

bool KRayTraceScene::ClearBottomLevelAS()
{
	m_BottomASMap.clear();
	m_Handles.Clear();
	return true;
}

void KRayTraceScene::UpdateAccelerationStructure()
{
	std::vector<IKAccelerationStructure::BottomASTransformTuple> bottomASs;
	bottomASs.reserve(m_BottomASMap.size());
	for (auto it = m_BottomASMap.begin(), itEnd = m_BottomASMap.end(); it != itEnd; ++it)
	{
		bottomASs.push_back(it->second);
	}
	ASSERT_RESULT(m_TopDown->UpdateTopDown(bottomASs));
}

void KRayTraceScene::CreateAccelerationStructure()
{
	std::vector<IKAccelerationStructure::BottomASTransformTuple> bottomASs;
	bottomASs.reserve(m_BottomASMap.size());
	for (auto it = m_BottomASMap.begin(), itEnd = m_BottomASMap.end(); it != itEnd; ++it)
	{
		bottomASs.push_back(it->second);
	}
	m_TopDown->SetDebugName((m_Scene->GetName() + "_TLAS").c_str());
	ASSERT_RESULT(m_TopDown->InitTopDown(bottomASs));
}

void KRayTraceScene::DestroyAccelerationStructure()
{
	m_TopDown->UnInit();
}

void KRayTraceScene::RecreateAS()
{
	if (m_bNeedRecreateAS)
	{
		m_LastRecreateFrame = KRenderGlobal::CurrentFrameNum;
		m_bNeedRecreateAS = false;

		UpdateAccelerationStructure();

		for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
		{
			IKRayTracePipelinePtr& pipeline = it->pipeline;
			pipeline->MarkASUpdated();
		}
	}
}

bool KRayTraceScene::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
	{
		return it->debugDrawer.Render(renderPass, commandList);
	}
	return true;
}

void KRayTraceScene::OnSceneChanged(EntitySceneOp op, IKEntity* entity)
{
	bool bottomUpChanged = false;

	if (op != ESO_REMOVE)
	{
		KRenderComponent* renderComponent = nullptr;
		KTransformComponent* transformComponent = nullptr;

		ASSERT_RESULT(entity->GetComponent(CT_RENDER, &renderComponent));
		ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));

		std::vector<IKAccelerationStructurePtr> subAS;
		std::vector<IKMaterialTextureBindingPtr> mtlTex;

		renderComponent->GetAllAccelerationStructure(subAS);
		renderComponent->GetAllTextrueBinding(mtlTex);

		const glm::mat4& transform = transformComponent->GetFinal_RenderThread();

		if (op == ESO_ADD)
		{
			if (subAS.size() == mtlTex.size())
			{
				for (uint32_t idx = 0; idx < subAS.size(); ++idx)
				{
						uint32_t handle = AddBottomLevelAS(subAS[idx], transform, mtlTex[idx]);
						m_ASHandles[entity].insert(handle);					
				}
				bottomUpChanged = true;
				renderComponent->RegisterCallback(&m_OnRenderComponentChangedFunc);
			}
		}
		else if (op == ESO_TRANSFORM)
		{
			auto it = m_ASHandles.find(entity);
			if (it != m_ASHandles.end())
			{
				for (uint32_t handle : it->second)
				{
					TransformBottomLevelAS(handle, transform);
				}
				bottomUpChanged = true;
			}
		}
	}
	else
	{
		auto it = m_ASHandles.find(entity);
		if (it != m_ASHandles.end())
		{
			for (uint32_t handle : it->second)
			{
				RemoveBottomLevelAS(handle);
			}
			m_ASHandles.erase(it);
			bottomUpChanged = true;
		}
	}

	if (m_TopDown && bottomUpChanged)
	{
		m_LastDirtyFrame = KRenderGlobal::CurrentFrameNum;
		m_bNeedRecreateAS = true;
	}
}

void KRayTraceScene::OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init)
{
	IKEntity* entity = renderComponent->GetEntityHandle();
	if (!entity)
	{
		return;
	}

	KTransformComponent* transformComponent = nullptr;
	ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));

	bool bottomUpChanged = false;

	if (init)
	{
		std::vector<IKAccelerationStructurePtr> subAS;
		std::vector<IKMaterialTextureBindingPtr> mtlTex;

		renderComponent->GetAllAccelerationStructure(subAS);
		renderComponent->GetAllTextrueBinding(mtlTex);

		const glm::mat4& transform = transformComponent->GetFinal_RenderThread();

		if (subAS.size() == mtlTex.size())
		{
			for (uint32_t idx = 0; idx < subAS.size(); ++idx)
			{
				uint32_t handle = AddBottomLevelAS(subAS[idx], transform, mtlTex[idx]);
				m_ASHandles[entity].insert(handle);
			}
			bottomUpChanged = true;
		}
	}
	else
	{
		auto it = m_ASHandles.find(entity);
		if (it != m_ASHandles.end())
		{
			for (uint32_t handle : it->second)
			{
				RemoveBottomLevelAS(handle);
			}
			m_ASHandles.erase(it);
			bottomUpChanged = true;
		}
	}

	if (m_TopDown && bottomUpChanged)
	{
		m_LastDirtyFrame = KRenderGlobal::CurrentFrameNum;
		m_bNeedRecreateAS = true;
	}
}

bool KRayTraceScene::Init(IKRenderScene* scene, const KCamera* camera)
{
	UnInit();

	if (scene && camera)
	{
		m_Scene = scene;
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

		cameraBuffer->SetDebugName((scene->GetName() + "RayTraceCameraBuffer").c_str());

		std::vector<IKEntity*> entites;
		scene->GetAllEntities(entites);

		for (IKEntity*& entity : entites)
		{
			OnSceneChanged(ESO_ADD, entity);
		}

		m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);

		KRenderDeviceProperties *deviceProperty = nullptr;
		KRenderGlobal::RenderDevice->QueryProperty(&deviceProperty);
		if (deviceProperty && deviceProperty->raytraceSupport)
		{
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateAccelerationStructure(m_TopDown));
			CreateAccelerationStructure();
		}

		m_bNeedRecreateAS = false;

		return true;
	}
	return false;
}

bool KRayTraceScene::AddRaytracePipeline(IKRayTracePipelinePtr& pipeline)
{
	if (pipeline)
	{
		auto it = std::find_if(m_RaytracePipelineInfos.begin(), m_RaytracePipelineInfos.end(), [pipeline](const RayTracePipelineInfo& info)
		{
			return info.pipeline == pipeline;
		});
		if (it == m_RaytracePipelineInfos.end())
		{
			pipeline->Init(m_CameraBuffer,m_TopDown, m_Width, m_Height);
			KRTDebugDrawer debugDrawer;
			debugDrawer.Init(pipeline->GetStorageTarget()->GetFrameBuffer(), 0, 0, 1, 1);
			m_RaytracePipelineInfos.push_back({ pipeline, std::move(debugDrawer) });
		}
		return true;
	}
	return false;
}

bool KRayTraceScene::RemoveRaytracePipeline(IKRayTracePipelinePtr& pipeline)
{
	auto it = std::find_if(m_RaytracePipelineInfos.begin(), m_RaytracePipelineInfos.end(), [pipeline](const RayTracePipelineInfo& info)
	{
		return info.pipeline == pipeline;
	});
	if (it != m_RaytracePipelineInfos.end())
	{
		SAFE_UNINIT(it->pipeline);
		it->debugDrawer.UnInit();
		m_RaytracePipelineInfos.erase(it);
		return true;
	}
	return false;
}

bool KRayTraceScene::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
		m_Scene = nullptr;
	}

	m_Camera = nullptr;
	m_ASHandles.clear();
	for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
	{
		SAFE_UNINIT(it->pipeline);
		it->debugDrawer.UnInit();
	}
	m_RaytracePipelineInfos.clear();

	SAFE_UNINIT(m_CameraBuffer);
	SAFE_UNINIT(m_TopDown);

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
	for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
	{
		it->debugDrawer.EnableDraw();
	}
	return true;
}

bool KRayTraceScene::DisableDebugDraw()
{
	for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
	{
		it->debugDrawer.DisableDraw();
	}
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
		IKSwapChain* chain = KRenderGlobal::MainWindow->GetSwapChain();
		if (chain->GetWidth() && chain->GetHeight())
		{
			m_Width = static_cast<uint32_t>(static_cast<float>(chain->GetWidth()) * m_ImageScale);
			m_Height = static_cast<uint32_t>(static_cast<float>(chain->GetHeight()) * m_ImageScale);
			for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
			{
				it->pipeline->ResizeImage(m_Width, m_Height);
			}
		}
	}
}

void KRayTraceScene::Reload()
{
	for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
	{
		it->pipeline->Reload();
	}
}

bool KRayTraceScene::Execute(KRHICommandList& commandList)
{
	if (m_bNeedRecreateAS)
	{
		FLUSH_INFLIGHT_RENDER_JOB();
		RecreateAS();
	}

	for (auto it = m_RaytracePipelineInfos.begin(); it != m_RaytracePipelineInfos.end(); ++it)
	{
		commandList.Execute(it->pipeline);
	}
	return true;
}

const KCamera* KRayTraceScene::GetCamera()
{
	return m_Camera;
}

IKAccelerationStructurePtr KRayTraceScene::GetTopDownAS()
{
	return m_TopDown;
}