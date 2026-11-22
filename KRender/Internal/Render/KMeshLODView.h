#pragma once
#include "Internal/Asset/KSubMesh.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include <unordered_map>

class KCamera;
class KMaterialSubMesh;

struct KMeshLODSelectionStats
{
	std::array<uint64_t, KSubMesh::LOD_COUNT> subMeshes{};
	std::array<uint64_t, KSubMesh::LOD_COUNT> triangles{};
};

class KMeshLODView
{
public:
	using StaticLODSelection = std::unordered_map<IKEntity::IDType, std::vector<uint8_t>>;
private:
	struct EntitySelection
	{
		std::weak_ptr<KMaterialSubMesh> binding;
		std::vector<uint8_t> history;
		std::vector<uint8_t> levels;
		uint64_t lastVisibleFrame = 0;
	};

	std::unordered_map<IKEntity::IDType, EntitySelection> m_Entities;
	KMeshLODSelectionStats m_Stats;
	StaticLODSelection m_StaticLevels;
	uint64_t m_Frame = 0;
	int m_ForcedLevel = -1;
public:
	void Update(const std::vector<IKEntity*>& entities, const KCamera& camera, int forcedLevel);
	const std::vector<uint8_t>* GetLevels(IKEntity::IDType entity) const;
	const KMeshLODSelectionStats& GetStats() const { return m_Stats; }
	const StaticLODSelection& GetStaticLevels() const { return m_StaticLevels; }
};
