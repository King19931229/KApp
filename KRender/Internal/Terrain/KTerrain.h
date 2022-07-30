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
	void LoadHeightMap(const std::string& file) override;
	void Update(const KCamera* camera) override;

	TerrainType GetType() const override { return m_Type; }
};