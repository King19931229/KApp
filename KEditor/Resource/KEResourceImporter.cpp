#include "KEResourceImporter.h"

#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KStringUtil.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

#include <assert.h>

KEResourceImporter::KEResourceImporter()
	: m_EntityDropDistance(30.0f)
{
}

KEResourceImporter::~KEResourceImporter()
{
}

const char* KEResourceImporter::ms_SupportedMeshExts[] =
{
	".obj",
	".3ds",
	".fbx"
};

bool KEResourceImporter::IsSupportedMesh(const char* ext)
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

bool KEResourceImporter::DropPosition(const KCamera* camera, const KAABBBox& localBound, glm::vec3& pos)
{
	if (camera)
	{
		const glm::mat4& viewMat = camera->GetViewMatrix();
		KAABBBox viewBound;
		localBound.Transform(viewMat, viewBound);
		float zExtend = viewBound.GetExtend().z;
		pos = camera->GetPostion() + (zExtend * 0.5f + m_EntityDropDistance) * camera->GetForward();
		return true;
	}
	return false;
}

IKEntityPtr KEResourceImporter::Drop(const KCamera* camera, const std::string& path)
{
	if (camera)
	{
		std::string name;
		std::string ext;
		if (KFileTool::SplitExt(path, name, ext))
		{
			KStringUtil::Lower(ext, ext);
			const char* cExt = ext.c_str();

			IKEntityPtr entity = nullptr;
			IKRenderComponent* renderComponent = nullptr;
			if (strcmp(cExt, ".mesh") == 0)
			{
				entity = KECS::EntityManager->CreateEntity();
				if (entity->RegisterComponent(CT_RENDER, &renderComponent))
				{
					renderComponent->SetPathMesh(path.c_str());
					renderComponent->Init();
				}
			}
			else if (IsSupportedMesh(cExt))
			{
				entity = KECS::EntityManager->CreateEntity();
				if (entity->RegisterComponent(CT_RENDER, &renderComponent))
				{
					renderComponent->SetPathAsset(path.c_str());
					renderComponent->Init();
				}
			}

			if (entity && renderComponent)
			{
				KAABBBox localBound;
				if (renderComponent->GetLocalBound(localBound))
				{
					IKTransformComponent* transformComponent = nullptr;
					if (entity->RegisterComponent(CT_TRANSFORM, &transformComponent))
					{
						glm::vec3 pos(0.0f);
						DropPosition(camera, localBound, pos);
						transformComponent->SetPosition(pos);
						return entity;
					}
				}
			}

			if (entity)
			{
				entity->UnRegisterAllComponent();
				KECS::EntityManager->ReleaseEntity(entity);
			}
		}
	}
	return nullptr;
}