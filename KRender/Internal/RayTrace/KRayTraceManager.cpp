#include "KRayTraceManager.h"
#include "KRayTraceScene.h"
#include "Internal/KRenderGlobal.h"

KRayTraceManager::KRayTraceManager()
{
}

KRayTraceManager::~KRayTraceManager()
{
	ASSERT_RESULT(m_Scenes.empty());
}

bool KRayTraceManager::Init()
{
	return true;
}

bool KRayTraceManager::UnInit()
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->UnInit();
	}
	m_Scenes.clear();
	return true;
}

bool KRayTraceManager::Execute(KRHICommandList& commandList)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->Execute(commandList);
	}
	return true;
}

bool KRayTraceManager::UpdateCamera()
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->UpdateCamera();
	}
	return true;
}

bool KRayTraceManager::Resize(size_t width, size_t height)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		KRayTraceScene* traceScene = (KRayTraceScene*)scene.get();
		traceScene->UpdateSize();
	}
	return true;
}

bool KRayTraceManager::Reload()
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		KRayTraceScene* traceScene = (KRayTraceScene*)scene.get();
		traceScene->Reload();
	}
	return true;
}

bool KRayTraceManager::CreateRayTraceScene(IKRayTraceScenePtr& scene)
{
	scene = IKRayTraceScenePtr(KNEW KRayTraceScene());
	m_Scenes.insert(scene);
	return true;
}

bool KRayTraceManager::RemoveRayTraceScene(IKRayTraceScenePtr& scene)
{
	auto it = m_Scenes.find(scene);
	if (it != m_Scenes.end())
		m_Scenes.erase(it);
	return true;
}

bool KRayTraceManager::GetAllRayTraceScene(std::unordered_set<IKRayTraceScenePtr>& scenes)
{
	scenes = m_Scenes;
	return true;
}

bool KRayTraceManager::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	for (IKRayTraceScenePtr scene : m_Scenes)
	{
		scene->DebugRender(renderPass, commandList);
	}
	return true;
}