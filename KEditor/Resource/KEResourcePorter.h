#pragma once

#include "KBase/Interface/Entity/IKEntity.h"
#include "KBase/Publish/KStringUtil.h"
#include "KRender/Publish/KCamera.h"
#include <stack>

class KEResourcePorter
{
protected:
	float m_EntityDropDistance;

	static const char* ms_SupportedMeshExts[];
	static bool IsSupportedMeshAsset(const char* ext);

	bool DropPosition(const KCamera* camera, const KAABBBox& localBound, glm::vec3& pos);
	bool GetBaseName(const std::string& fullName, std::string& baseName);
public:
	KEResourcePorter();
	~KEResourcePorter();

	bool InitEntity(const std::string& path, IKEntityPtr& entity);
	bool UnInitEntity(IKEntityPtr& entity);

	bool Convert(const std::string& assetPath, const std::string& meshPath);
	bool ModelDrop(const KCamera* camera, const std::string& path);
	bool MaterialDrop(size_t x, size_t y, const std::string& path);

	inline void SetEntityDropDistance(float distance) { m_EntityDropDistance = distance; }
	inline float GetEntityDropDistance() const { return m_EntityDropDistance; }
};