#pragma once
#include "KBase/Publish/KConfig.h"
#include "Internal/ECS/KEntity.h"
#include "Publish/KCamera.h"
#include "glm/glm.hpp"

enum SceneManagerType
{
	SCENE_MANGER_TYPE_OCTREE
};

class KScene
{
public:
	bool Init(SceneManagerType type);
	bool UnInit();

	bool GetVisibleEntity(const KCamera* camera, std::vector<KEntity*>& visibles);

	bool Load(const char* filename);
	bool Save(const char* filename);
};