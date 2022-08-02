#pragma once
#include "Interface/IKBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKRenderCommand.h"
#include "Interface/IKTerrain.h"
#include "Internal/Terrain/KHeightMap.h"

class KClipmapFootprint
{
protected:
	uint32_t m_Width;
	uint32_t m_Height;
	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;
	KVertexData m_VertexData;
	KIndexData m_IndexData;
	void CreateData(const std::vector<glm::vec2>& verts, const std::vector<uint16_t>& idxs);
public:
	KClipmapFootprint();
	~KClipmapFootprint();

	void Init(uint32_t width, uint32_t height);
	void Init(uint32_t width);
	void UnInit();

	const KVertexData& GetVertexData() const { return m_VertexData; }
	const KIndexData& GetIndexData() const { return m_IndexData; }
};
typedef std::shared_ptr<KClipmapFootprint> KClipmapFootprintPtr;

class KClipmapFootprintPos
{
protected:
	uint32_t m_PosX;
	uint32_t m_PosY;
	KClipmapFootprintPtr m_Footprint;
public:
	KClipmapFootprintPos(uint32_t x, uint32_t y, KClipmapFootprintPtr footprint)
		: m_PosX(x)
		, m_PosY(y)
		, m_Footprint(footprint)
	{}
	~KClipmapFootprintPos()
	{}

	uint32_t GetPosX() const { return m_PosX; }
	uint32_t GetPosY() const { return m_PosY; }
	KClipmapFootprintPtr GetFootPrint() { return m_Footprint; }
};

class KClipmap;

class KClipmapLevel
{
public:
	enum TrimLocation
	{
		TL_BOTTOM_LEFT,
		TL_BOTTOM_RIGHT,
		TL_TOP_RIGHT,
		TL_TOP_LEFT,
		TL_NONE
	};
protected:
	KClipmap* m_Parent;
	uint32_t m_LevelIdx;
	uint32_t m_GridSize;
	uint32_t m_GridCount;
	int32_t m_BottomLeftX;
	int32_t m_BottomLeftY;
	uint32_t m_TextureBiasX;
	uint32_t m_TextureBiasY;
	TrimLocation m_TrimLocation;
	glm::vec4 m_WorldStartScale;
	std::vector<float> m_ClipHeightData;
	IKTexturePtr m_Texture;
public:
	KClipmapLevel(KClipmap* parent, uint32_t levelIdx);
	~KClipmapLevel();

	void SetPosition(int32_t bottomLeftX, int32_t bottomLeftY, TrimLocation trim);
	void UpdateHeightData();
	void UpdateTexture();

	void Init();
	void UnInit();

	IKTexturePtr GetTexture() { return m_Texture; }
	float GetHeight(int32_t x, int32_t y) const;

	uint32_t GetGridSize() const { return m_GridSize; }
	uint32_t GetTextureBiasX() const { return m_TextureBiasX; }
	uint32_t GetTextureBiasY() const { return m_TextureBiasY; }
	TrimLocation GetTrimLocation() const { return m_TrimLocation; }
	const glm::vec4& GetWorldStartScale() const { return m_WorldStartScale; }
};
typedef std::shared_ptr<KClipmapLevel> KClipmapLevelPtr;


class KClipmap
{
protected:
	enum FootprintType
	{
		FT_BLOCK,
		FT_FIXUP_HORIZONTAL,
		FT_FIXUP_VERTICAL,
		FT_INTERIORTRIM_HORIZONTAL,
		FT_INTERIORTRIM_VERTICAL,
		FT_OUTER_DEGENERATERING,
		FT_COUNT
	};

	KClipmapFootprintPtr m_Footprints[FT_COUNT];
	std::vector<KClipmapFootprintPos> m_FootprintPos;
	std::vector<KClipmapLevelPtr> m_ClipLevels;
	std::vector<IKPipelinePtr> m_ClipLevelPipelines;

	IKShaderPtr m_VSShader;
	IKShaderPtr m_FSShader;
	IKSamplerPtr m_Sampler;

	IKCommandBufferPtr m_CommandBuffer;
	IKCommandPoolPtr m_CommandPool;

	KHeightMap m_HeightMap;

	uint32_t m_GridCount;
	uint32_t m_LevelCount;
	int32_t m_GridCenterX;
	int32_t m_GridCenterY;
	int32_t m_ClipCenterX;
	int32_t m_ClipCenterY;
	glm::vec3 m_GridWorldCenter;
	glm::vec3 m_ClipWorldCenter;
	glm::vec2 m_BaseGridSize;
	float m_Size;
	float m_HeightScale;
	bool m_Updated;

	static const VertexFormat ms_VertexFormats[1];

	KClipmapFootprintPtr CreateFootprint(uint32_t width, uint32_t height);
	KClipmapFootprintPtr CreateFootprint(uint32_t widtht);
	void InitializeFootprint();
	void InitializeFootprintPos();
	void InitializeClipmapLevel();
	void InitializePipeline();
public:
	KClipmap();
	~KClipmap();

	void Init(const glm::vec3& center, float size, uint32_t gridLevel, uint32_t divideLevel);
	void UnInit();

	void LoadHeightMap(const std::string& file);

	void Update(const glm::vec3& cameraPos);
	bool Render(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);

	uint32_t GetBlockCount() const { return (m_GridCount + 1) / 4; }
	uint32_t GetGridCount() const { return m_GridCount; }
	uint32_t GetLevelCount() const { return m_LevelCount; }
	int32_t GetClipCenterX() const { return m_ClipCenterX; }
	int32_t GetClipCenterY() const { return m_ClipCenterY; }
	glm::vec2 GetBaseGridSize() const { return m_BaseGridSize; }
	const glm::vec3& GetGridWorldCenter() const { return m_GridWorldCenter; }
	const glm::vec3& GetClipWorldCenter() const { return m_ClipWorldCenter; }

	KClipmapLevelPtr GetClipmapLevel(uint32_t idx);

	const KHeightMap& GetHeightMap() const { return m_HeightMap; }
	float GetSize() const { return m_Size; }
};