#pragma once
#include "Interface/IKTerrain.h"
#include "Clipmap/KClipmap.h"

class KNullTerrain : public IKTerrain
{
public:
	bool Reload() override { return true; }
	void LoadHeightMap(const std::string& file) override {}
	void LoadDiffuse(const std::string& file) override {}
	TerrainType GetType() const override { return TERRAIN_TYPE_NONE; }
	void Update(const KCamera* camera) override {}
	bool Render(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers) override { return true; }
	bool EnableDebugDraw(const KTerrainDebug& debug) override { return true; }
	bool DisableDebugDraw() override { return true; }
	bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) override { return true; }
};