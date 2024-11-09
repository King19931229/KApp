#pragma once
#include "Interface/IKTerrain.h"

class KNullTerrain : public IKTerrain
{
public:
	bool Reload() override { return true; }
	void LoadHeightMap(const std::string& file) override {}
	void LoadDiffuse(const std::string& file) override {}
	TerrainType GetType() const override { return TERRAIN_TYPE_NONE; }
	void Update(const KCamera* camera) override {}
	bool Render(class KRHICommandList& commandList, IKRenderPassPtr renderPass) override { return true; }
	bool EnableDebugDraw(const KTerrainDebug& debug) override { return true; }
	bool DisableDebugDraw() override { return true; }
	bool DebugRender(class KRHICommandList& commandList, IKRenderPassPtr renderPass) override { return true; }
};