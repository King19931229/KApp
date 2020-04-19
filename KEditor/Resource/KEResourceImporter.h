#pragma once

#include "KBase/Interface/Entity/IKEntity.h"
#include "KRender/Publish/KCamera.h"

class KEResourceImporter
{
protected:
	float m_EntityDropDistance;
	static const char* ms_SupportedMeshExts[];
	static bool IsSupportedMesh(const char* ext);

	bool DropPosition(const KCamera* camera, const KAABBBox& localBound, glm::vec3& pos);
public:
	KEResourceImporter();
	~KEResourceImporter();

	IKEntityPtr Drop(const KCamera* camera, const std::string& path);

	inline void SetEntityDropDistance(float distance) { m_EntityDropDistance = distance; }
	inline float GetEntityDropDistance() const { return m_EntityDropDistance; }
};