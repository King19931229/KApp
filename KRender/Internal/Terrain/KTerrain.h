#pragma once
#include "Interface/IKTerrain.h"
#include "Clipmap/KClipmap.h"

class KTerrain : public IKTerrain
{
protected:
	union
	{
		KClipmap* clipmap;
	}m_Soul;
	TerrainType m_Type;
public:
	KTerrain();
	~KTerrain();

	bool Create(const glm::vec3& center, float size, const KTerrainContext& context) override;
	bool Destroy() override;
	bool Reload() override;

	void LoadHeightMap(const std::string& file) override;

	TerrainType GetType() const override { return m_Type; }

	void Update(const KCamera* camera) override;
	bool Render(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers) override;
};