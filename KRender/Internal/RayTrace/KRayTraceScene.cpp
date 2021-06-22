#include "KRayTraceScene.h"
#include "Internal/KRenderGlobal.h"

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

		pipeline->Init(m_CameraBuffers);

		return true;
	}
	return false;
}

bool KRayTraceScene::UnInit()
{
	m_Scene = nullptr;
	m_Pipeline = nullptr;
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