#pragma once
#include "Interface/IKBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKRenderCommand.h"
#include "Interface/IKTerrain.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Terrain/KHeightMap.h"

class KClipmapFootprint
{
protected:
	int32_t m_Width;
	int32_t m_Height;
	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;
	KVertexData m_VertexData;
	KIndexData m_IndexData;
	void CreateData(const std::vector<glm::vec2>& verts, const std::vector<uint16_t>& idxs);
public:
	KClipmapFootprint();
	~KClipmapFootprint();

	void Init(int32_t width, int32_t height);
	void Init(int32_t width);
	void UnInit();

	const KVertexData& GetVertexData() const { return m_VertexData; }
	const KIndexData& GetIndexData() const { return m_IndexData; }
};
typedef std::shared_ptr<KClipmapFootprint> KClipmapFootprintPtr;

class KClipmapFootprintPos
{
protected:
	int32_t m_PosX;
	int32_t m_PosY;
	KClipmapFootprintPtr m_Footprint;
public:
	KClipmapFootprintPos(int32_t x, int32_t y, KClipmapFootprintPtr footprint)
		: m_PosX(x)
		, m_PosY(y)
		, m_Footprint(footprint)
	{}
	~KClipmapFootprintPos()
	{}

	int32_t GetPosX() const { return m_PosX; }
	int32_t GetPosY() const { return m_PosY; }
	KClipmapFootprintPtr GetFootPrint() { return m_Footprint; }
};

class KClipmap;

struct KClipmapUpdateRect
{
	int32_t startX, startY;
	int32_t endX, endY;
	KClipmapUpdateRect()
	{
		startX = startY = 0;
		endX = endY = 0;
	}
	KClipmapUpdateRect(int32_t sx, int32_t sy, int32_t ex, int32_t ey)
	{
		startX = sx;
		startY = sy;
		endX = ex;
		endY = ey;
	}
};
typedef KClipmapUpdateRect KClipmapTextureUpdateRect;
typedef KClipmapUpdateRect KClipmapMovementUpdateRect;

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
	static const VertexFormat ms_VertexFormats[1];
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_UpdateVertices[4];
	static const uint16_t ms_UpdateIndices[6];
	static IKVertexBufferPtr ms_UpdateVertexBuffer;
	static IKIndexBufferPtr ms_UpdateIndexBuffer;
	static IKCommandBufferPtr ms_CommandBuffer;
	static IKCommandPoolPtr ms_CommandPool;
	static KVertexData ms_UpdateVertexData;
	static KIndexData ms_UpdateIndexData;
	static IKSamplerPtr ms_Sampler;

	IKTexturePtr m_UpdateTextures[4];
	IKPipelinePtr m_UpdatePipelines[4];
	IKRenderTargetPtr m_TextureTarget;
	IKTexturePtr m_Texture;
	IKRenderPassPtr m_UpdateRenderPass;
	IKShaderPtr m_UpdateVS;
	IKShaderPtr m_UpdateFS;

	KClipmap* m_Parent;
	int32_t m_LevelIdx;
	int32_t m_GridSize;
	int32_t m_GridCount;

	int32_t m_BottomLeftX;
	int32_t m_BottomLeftY;
	int32_t m_ScrollX;
	int32_t m_ScrollY;
	int32_t m_NewScrollX;
	int32_t m_NewScrollY;

	TrimLocation m_TrimLocation;
	glm::vec4 m_WorldStartScale;
	std::vector<float> m_ClipHeightData;
	std::vector<KClipmapTextureUpdateRect> m_UpdateRects;

	void TrimUpdateRect(const KClipmapUpdateRect& trim, KClipmapUpdateRect& rect);
	void TrimUpdateRects(std::vector<KClipmapUpdateRect>& rects);
	void UpdateTextureByRect(const std::vector<KClipmapTextureUpdateRect>& rects);
	void InitializePipeline();

	int32_t TextureCoordXToWorldX(int32_t i);
	int32_t TextureCoordYToWorldY(int32_t j);
	int32_t WorldXToTextureCoordX(int32_t x);
	int32_t WorldYToTextureCoordY(int32_t y);

	float GetClipHeight(float u, float v);
public:
	KClipmapLevel(KClipmap* parent, int32_t levelIdx);
	~KClipmapLevel();

	void SetPosition(int32_t bottomLeftX, int32_t bottomLeftY, TrimLocation trim);
	void ScrollPosition(int32_t bottomLeftX, int32_t bottomLeftY, TrimLocation trim);

	void PopulateUpdateRects();
	void UpdateHeightData();
	void UpdateWorldStartScale();
	void UpdateTexture();
	void CheckHeightValid();

	void Init();
	void UnInit();

	static void InitShared();
	static void UnInitShared();

	IKRenderTargetPtr GetTextureTarget() { return m_TextureTarget; }
	IKTexturePtr GetTexture() { return m_Texture; }
	float GetHeight(int32_t x, int32_t y) const;

	int32_t GetGridSize() const { return m_GridSize; }
	int32_t GetScrollX() const { return m_ScrollX; }
	int32_t GetScrollY() const { return m_ScrollY; }
	int32_t GetRealBottomLeftX() const { return m_BottomLeftX + m_ScrollX * m_GridSize; }
	int32_t GetRealBottomLeftY() const { return m_BottomLeftY + m_ScrollY * m_GridSize; }
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
		FT_INNER_DEGENERATERING,
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

	int32_t m_GridCount;
	int32_t m_LevelCount;
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

	KClipmapFootprintPtr CreateFootprint(int32_t width, int32_t height);
	KClipmapFootprintPtr CreateFootprint(int32_t widtht);
	void InitializeFootprint();
	void InitializeFootprintPos();
	void InitializeClipmapLevel();
	void InitializePipeline();
public:
	KClipmap();
	~KClipmap();

	void Init(const glm::vec3& center, float size, int32_t gridLevel, int32_t divideLevel);
	void UnInit();

	void LoadHeightMap(const std::string& file);

	void Update(const glm::vec3& cameraPos);
	bool Render(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);
	void Reload();

	int32_t GetBlockCount() const { return (m_GridCount + 1) / 4; }
	int32_t GetGridCount() const { return m_GridCount; }
	int32_t GetLevelCount() const { return m_LevelCount; }
	int32_t GetClipCenterX() const { return m_ClipCenterX; }
	int32_t GetClipCenterY() const { return m_ClipCenterY; }
	glm::vec2 GetBaseGridSize() const { return m_BaseGridSize; }
	const glm::vec3& GetGridWorldCenter() const { return m_GridWorldCenter; }
	const glm::vec3& GetClipWorldCenter() const { return m_ClipWorldCenter; }

	KClipmapLevelPtr GetClipmapLevel(int32_t idx);

	const KHeightMap& GetHeightMap() const { return m_HeightMap; }
	float GetSize() const { return m_Size; }
	float GetHeightScale() const { return m_HeightScale; }
};