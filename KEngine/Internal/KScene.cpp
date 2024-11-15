#include "KScene.h"
#include "KBase/Interface/IKXML.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Publish/KMath.h"
#include "KEngine.h"

KScene::KScene()
	: m_RenderScene(nullptr)
{
	m_TransformCallback = std::bind(&KScene::OnTransformChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
}

void KScene::OnTransformChanged(IKTransformComponent* comp, const glm::vec3& pos, const glm::vec3& scale, const glm::quat& rotate)
{
	IKEntity* entity = comp->GetEntityHandle();
	if (m_RenderScene)
	{
		m_RenderScene->Transform(entity);
	}
}

KScene::~KScene()
{
	ASSERT_RESULT(m_Entities.empty());
}

bool KScene::Init(IKRenderScene* renderScene)
{
	m_RenderScene = renderScene;
	return true;
}

bool KScene::UnInit()
{
	m_RenderScene = nullptr;
	m_Entities.clear();
	return true;
}

bool KScene::Add(IKEntityPtr entity)
{
	if (m_RenderScene)
	{
		m_Entities.insert(entity);
		IKTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_TRANSFORM, &transform))
		{
			transform->RegisterTransformChangeCallback(&m_TransformCallback);
		}
		return m_RenderScene->Add(entity.get());
	}
	return false;
}

bool KScene::Remove(IKEntityPtr entity)
{
	if (m_RenderScene)
	{
		m_Entities.erase(entity);
		IKTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_TRANSFORM, &transform))
		{
			transform->UnRegisterTransformChangeCallback(&m_TransformCallback);
		}
		return m_RenderScene->Remove(entity.get());
	}
	return false;
}

bool KScene::Transform(IKEntityPtr entity)
{
	if (m_RenderScene)
	{
		return m_RenderScene->Transform(entity.get());
	}
	return false;
}

const KScene::EntitySetType& KScene::GetEntities() const
{
	return m_Entities;
}

bool KScene::CreateTerrain(const glm::vec3& center, float size, float height, const KTerrainContext& context)
{
	if (m_RenderScene)
	{
		return m_RenderScene->CreateTerrain(center, size, height, context);
	}
	return false;
}

bool KScene::DestroyTerrain()
{
	if (m_RenderScene)
	{
		return m_RenderScene->DestroyTerrain();
	}
	return false;
}

IKTerrainPtr KScene::GetTerrain()
{
	if (m_RenderScene)
	{
		return m_RenderScene->GetTerrain();
	}
	return nullptr;
}

bool KScene::Clear()
{
	for (IKEntityPtr entity : m_Entities)
	{
		m_RenderScene->Remove(entity.get());
	}
	m_Entities.clear();
	return true;
}

bool KScene::Pick(const KCamera& camera, size_t x, size_t y,
	size_t screenWidth, size_t screenHeight, std::vector<IKEntity*>& result)
{
	if (m_RenderScene)
	{
		return m_RenderScene->Pick(camera, x, y, screenWidth, screenHeight, result);
	}
	result.clear();
	return false;
}

bool KScene::CloestPick(const KCamera& camera, size_t x, size_t y,
	size_t screenWidth, size_t screenHeight, IKEntity*& result)
{
	if (m_RenderScene)
	{
		return m_RenderScene->CloestPick(camera, x, y, screenWidth, screenHeight, result);
	}
	result = nullptr;
	return false;
}

bool KScene::RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntity*>& result)
{
	if (m_RenderScene)
	{
		return m_RenderScene->RayPick(origin, dir, result);
	}
	result.clear();
	return false;
}

bool KScene::CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntity*& result)
{
	if (m_RenderScene)
	{
		return m_RenderScene->CloestRayPick(origin, dir, result);
	}
	result = nullptr;
	return false;
}

const char* KScene::msSceneKey = "scene";
const char* KScene::msCameraKey = "camera";
const char* KScene::msEntityKey = "entity";

bool KScene::Save(const char* filename)
{
	IKXMLDocumentPtr root = GetXMLDocument();
	root->NewDeclaration(R"(xml version="1.0" encoding="utf-8")");

	IKXMLElementPtr sceneEle = root->NewElement(msSceneKey);
	for (IKEntityPtr entity : m_Entities)
	{
		IKXMLElementPtr entityEle = sceneEle->NewElement(msEntityKey);
		entity->Save(entityEle);
	}

	KCamera* camera = KEngineGlobal::Engine->GetRenderCore()->GetCamera();
	std::string viewMatrixText;
	if (KMath::ToString(camera->GetViewMatrix(), viewMatrixText))
	{
		IKXMLElementPtr cameraEle = root->NewElement(msCameraKey);
		cameraEle->SetText(viewMatrixText.c_str());
	}

	return root->SaveFile(filename);
}

bool KScene::Load(const char* filename)
{
	Clear();

	IKXMLDocumentPtr root = GetXMLDocument();

	if (root->ParseFromFile(filename))
	{
		IKXMLElementPtr sceneEle = root->FirstChildElement(msSceneKey);
		if (sceneEle && !sceneEle->IsEmpty())
		{
			IKXMLElementPtr entityEle = sceneEle->FirstChildElement(msEntityKey);
			while (entityEle && !entityEle->IsEmpty())
			{
				IKEntityPtr entity = KECS::EntityManager->CreateEntity();
				if (entity->Load(entityEle))
				{
					Add(entity);
				}
				else
				{
					KECS::EntityManager->ReleaseEntity(entity);
				}
				entityEle = entityEle->NextSiblingElement(msEntityKey);
			}
		}

		IKXMLElementPtr cameraEle = root->FirstChildElement(msCameraKey);
		if (cameraEle && !cameraEle->IsEmpty())
		{
			std::string viewMatrixText = cameraEle->GetText();
			

			glm::mat4 viewMatrix;
			if (KMath::FromString(viewMatrixText, viewMatrix))
			{
				KCamera* camera = KEngineGlobal::Engine->GetRenderCore()->GetCamera();
				camera->SetViewMatrix(viewMatrix);
			}
		}

		return true;
	}
	return false;
}