#pragma once
#include "KRender/Publish/KCamera.h"
#include <string>
#include <memory>

enum TerrainType
{
	TERRAIN_TYPE_CLIPMAP
};

struct KTerrainContext
{
	TerrainType type;
	union
	{
		struct
		{
			uint32_t gridLevel;
			uint32_t divideLevel;
		}clipmap;
	};
};

struct IKTerrain
{
	virtual ~IKTerrain() {}

	virtual bool Create(const glm::vec3& center, float size, const KTerrainContext& context) = 0;
	virtual bool Destroy() = 0;
	virtual void LoadHeightMap(const std::string& file) = 0;
	virtual void Update(const KCamera* camera) = 0;	
	virtual TerrainType GetType() const = 0;
};

typedef std::shared_ptr<IKTerrain> IKTerrainPtr;