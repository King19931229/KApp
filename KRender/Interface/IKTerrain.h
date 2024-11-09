#pragma once
#include "KRender/Publish/KCamera.h"
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderCommand.h"

enum TerrainType
{
	TERRAIN_TYPE_CLIPMAP,
	TERRAIN_TYPE_NONE
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

struct KTerrainDebug
{
	union
	{
		struct
		{
			uint32_t debugLevel;
		}clipmap;
	};
};

struct IKTerrain
{
	virtual ~IKTerrain() {}
	virtual bool Reload() = 0;

	virtual void LoadHeightMap(const std::string& file) = 0;
	virtual void LoadDiffuse(const std::string& file) = 0;

	virtual TerrainType GetType() const = 0;

	virtual void Update(const KCamera* camera) = 0;
	virtual bool Render(class KRHICommandList& commandList, IKRenderPassPtr renderPass) = 0;

	virtual bool EnableDebugDraw(const KTerrainDebug& debug) = 0;
	virtual bool DisableDebugDraw() = 0;
	virtual bool DebugRender(class KRHICommandList& commandList, IKRenderPassPtr renderPass) = 0;
};

typedef std::shared_ptr<IKTerrain> IKTerrainPtr;