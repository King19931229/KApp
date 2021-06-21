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
			IKUniformBufferPtr& uniformBuffer = m_CameraBuffers[i];
			KRenderGlobal::RenderDevice->CreateUniformBuffer(uniformBuffer);
		}

		return true;
	}
	return false;
}

bool KRayTraceScene::UnInit()
{
	m_Scene = nullptr;
	m_Pipeline = nullptr;
	return true;
}