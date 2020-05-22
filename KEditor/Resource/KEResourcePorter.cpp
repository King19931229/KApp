#include "KEResourcePorter.h"

#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KStringUtil.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

#include "KEngine/Interface/IKEngine.h"

#include <assert.h>

KEResourcePorter::KEResourcePorter()
	: m_EntityDropDistance(30.0f)
{
}

KEResourcePorter::~KEResourcePorter()
{
}

const char* KEResourcePorter::ms_SupportedMeshExts[] =
{
	".obj",
	".3ds",
	".fbx"
};

bool KEResourcePorter::IsSupportedMesh(const char* ext)
{
	for (const char* meshExt : ms_SupportedMeshExts)
	{
		if (strcmp(meshExt, ext) == 0)
		{
			return true;
		}
	}
	return false;
}

bool KEResourcePorter::DropPosition(const KCamera* camera, const KAABBBox& localBound, glm::vec3& pos)
{
	if (camera)
	{
		const glm::mat4& viewMat = camera->GetViewMatrix();
		KAABBBox viewBound;
		localBound.Transform(viewMat, viewBound);
		float zExtend = viewBound.GetExtend().z;
		pos = camera->GetPosition() + (zExtend * 0.5f + m_EntityDropDistance) * camera->GetForward();
		return true;
	}
	return false;
}

bool KEResourcePorter::InitEntity(const std::string& path, IKEntityPtr& entity, bool hostVisible)
{
	if (entity)
	{
		std::string name;
		std::string ext;
		if (KFileTool::SplitExt(path, name, ext))
		{
			KStringUtil::Lower(ext, ext);
			const char* cExt = ext.c_str();

			entity->UnRegisterAllComponent();

			IKRenderComponent* renderComponent = nullptr;
			if (strcmp(cExt, ".mesh") == 0)
			{
				if (entity->RegisterComponent(CT_RENDER, &renderComponent))
				{
					renderComponent->SetPathMesh(path.c_str());
					renderComponent->SetHostVisible(hostVisible);
					renderComponent->Init();
				}
			}
			else if (IsSupportedMesh(cExt))
			{
				if (entity->RegisterComponent(CT_RENDER, &renderComponent))
				{
					renderComponent->SetPathAsset(path.c_str());
					renderComponent->SetHostVisible(hostVisible);
					renderComponent->Init();
				}
			}

			if (renderComponent)
			{
				IKTransformComponent* transformComponent = nullptr;
				if (entity->RegisterComponent(CT_TRANSFORM, &transformComponent))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool KEResourcePorter::UnInitEntity(IKEntityPtr& entity)
{
	if (entity)
	{
		IKEnginePtr engine = KEngineGlobal::Engine;
		engine->Wait();

		entity->UnRegisterAllComponent();
		return true;
	}
	return false;
}

bool KEResourcePorter::Convert(const std::string& assetPath, const std::string& meshPath)
{
	bool bSuccess = false;

	IKEntityPtr entity = KECS::EntityManager->CreateEntity();
	if (InitEntity(assetPath, entity, true))
	{
		IKRenderComponent* renderComponent = nullptr;
		if (entity->GetComponent(CT_RENDER, &renderComponent))
		{
			bSuccess = renderComponent->SaveAsMesh(meshPath.c_str());
		}
	}

	if (entity)
	{
		entity->UnRegisterAllComponent();
		KECS::EntityManager->ReleaseEntity(entity);
	}

	return bSuccess;
}

IKEntityPtr KEResourcePorter::Drop(const KCamera* camera, const std::string& path)
{
	if (camera)
	{
		IKEntityPtr entity = KECS::EntityManager->CreateEntity();

		if (InitEntity(path, entity))
		{
			IKRenderComponent* renderComponent = nullptr;
			if (entity->GetComponent(CT_RENDER, &renderComponent))
			{
				KAABBBox localBound;
				if (renderComponent->GetLocalBound(localBound))
				{
					IKTransformComponent* transformComponent = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, &transformComponent))
					{
						glm::vec3 pos(0.0f);
						DropPosition(camera, localBound, pos);
						transformComponent->SetPosition(pos);
						return entity;
					}
				}
			}
		}

		if (entity)
		{
			entity->UnRegisterAllComponent();
			KECS::EntityManager->ReleaseEntity(entity);
		}
	}

	return nullptr;
}