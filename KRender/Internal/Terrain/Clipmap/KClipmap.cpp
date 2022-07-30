#include "KClipmap.h"
#include "KBase/Interface/IKLog.h"
#include <Internal/KRenderGlobal.h>
#include "glm/glm.hpp"

KClipmapFootprint::KClipmapFootprint()
	: m_Width(0)
	, m_Height(0)
{
}

KClipmapFootprint::~KClipmapFootprint()
{
	ASSERT_RESULT(!m_VertexBuffer);
	ASSERT_RESULT(!m_IndexBuffer);
}

void KClipmapFootprint::Init(uint32_t width, uint32_t height)
{
	UnInit();

	m_Width = width;
	m_Height = height;

	std::vector<glm::vec2> verts;
	std::vector<uint16_t> idxs;

	for (uint32_t y = 0; y < m_Height; ++y)
	{
		for (uint32_t x = 0; x < m_Width; ++x)
		{
			verts.push_back(glm::vec2(x, y));

			if (y < m_Height - 1)
			{
				idxs.push_back(y * m_Width + x);
				// 组建0大小三角形
				if (x == 0) idxs.push_back(y * m_Width + x);
				idxs.push_back((y + 1) * m_Width + x);
				// 组建0大小三角形
				if(x == m_Width - 1) idxs.push_back((y + 1) * m_Width + x);
			}
		}
	}

	KRenderGlobal::RenderDevice->CreateVertexBuffer(m_VertexBuffer);
	KRenderGlobal::RenderDevice->CreateIndexBuffer(m_IndexBuffer);

	m_VertexBuffer->InitMemory(verts.size(), sizeof(glm::vec2), verts.data());
	m_VertexBuffer->InitDevice(false);

	m_IndexBuffer->InitMemory(IT_16, idxs.size(), idxs.data());
	m_IndexBuffer->InitDevice(false);

	m_VertexData.vertexBuffers = { m_VertexBuffer };
	m_VertexData.vertexStart = 0;
	m_VertexData.vertexCount = (uint32_t)verts.size();
	m_VertexData.vertexFormats = { VF_TERRAIN_POS };

	m_IndexData.indexBuffer = m_IndexBuffer;
	m_IndexData.indexStart = 0;
	m_IndexData.indexCount = (uint32_t)idxs.size();
}

void KClipmapFootprint::UnInit()
{
	m_VertexData.Destroy();
	m_IndexData.Destroy();
	m_IndexBuffer = nullptr;
	m_VertexBuffer = nullptr;
}

KClipmapLevel::KClipmapLevel(KClipmap* parent, uint32_t levelIdx)
	: m_Parent(parent)
	, m_LevelIdx(levelIdx)
	, m_GridSize(1)
	, m_BottomLeftX(0)
	, m_BottomLeftY(0)
	, m_TrimPosition(TP_NONE)
{
	uint32_t levelCount = m_Parent->GetLevelCount();
	uint32_t gridCount = m_Parent->GetGridCount();

	m_GridSize = 1 << (levelCount - 1 - levelIdx);

	m_BottomLeftX = m_Parent->GetClipCenterX() - m_GridSize * (gridCount / 2);
	m_BottomLeftY = m_Parent->GetClipCenterY() - m_GridSize * (gridCount / 2);

	m_ClipHeightData.resize(gridCount * gridCount);
}

KClipmapLevel::~KClipmapLevel()
{}

void KClipmapLevel::SetPosition(int32_t x, int32_t y, TrimPosition trimPos)
{
	m_BottomLeftX = x;
	m_BottomLeftY = y;
	m_TrimPosition = trimPos;
}

void KClipmapLevel::UpdateHeightData()
{
	const KHeightMap& heightmap = m_Parent->GetHeightMap();
	uint32_t gridCount = m_Parent->GetGridCount();	

	for (uint32_t j = 0; j < gridCount; ++j)
	{
		int32_t y = m_BottomLeftY + j * m_GridSize;
		for (uint32_t i = 0; i < gridCount; ++i)
		{
			int32_t x = m_BottomLeftX + i * m_GridSize;
			m_ClipHeightData[j * gridCount + i] = heightmap.GetData(x, y);
		}
	}
}

KClipmap::KClipmap()
	: m_GridCount(0)
	, m_LevelCount(0)
	, m_GridCenterX(0)
	, m_GridCenterY(0)
	, m_ClipCenterX(0)
	, m_ClipCenterY(0)
	, m_GridWorldCenter(glm::vec3(0))
	, m_ClipWorldCenter(glm::vec3(0))
	, m_Size(1024.0f)
	, m_GridWorldSize(1)
{
}

KClipmap::~KClipmap()
{
}

KClipmapFootprintPtr KClipmap::CreateFootprint(uint32_t width, uint32_t height)
{
	KClipmapFootprintPtr footptint = KClipmapFootprintPtr(new KClipmapFootprint());
	footptint->Init(width, height);
	return footptint;
}

void KClipmap::InitializeFootprint()
{
	uint32_t m = m_GridCount / 4;

	m_Footprints[FT_BLOCK] = CreateFootprint(m, m);
	m_Footprints[FT_FIXUP_HORIZONTAL] = CreateFootprint(m, 3);
	m_Footprints[FT_FIXUP_VERTICAL] = CreateFootprint(3, m);
	m_Footprints[FT_INTERIORTRIM_HORIZONTAL] = CreateFootprint((2 * m) + 1, 2);
	m_Footprints[FT_INTERIORTRIM_VERTICAL] = CreateFootprint(2, (2 * m) + 1);
	m_Footprints[FT_OUTER_DEGENERATERING] = CreateFootprint((4 * m) - 1, (4 * m) - 1);
}

void KClipmap::InitializeFootprintPos()
{
	uint32_t blockCount = m_GridCount / 4;
	uint32_t& m = blockCount;

	// B1 - B4
	m_FootprintPos.push_back(KClipmapFootprintPos(0, 3 * m + 2, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(m, 3 * m + 2, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(2 * m + 2, 3 * m + 2, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(3 * m + 2, 3 * m + 2, m_Footprints[FT_BLOCK]));

	// B5 & B6
	m_FootprintPos.push_back(KClipmapFootprintPos(0, 2 * m + 2, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(3 * m + 2, 2 * m + 2, m_Footprints[FT_BLOCK]));

	// B7 & B8
	m_FootprintPos.push_back(KClipmapFootprintPos(0, m, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(3 * m + 2, m, m_Footprints[FT_BLOCK]));

	// B9 - B12
	m_FootprintPos.push_back(KClipmapFootprintPos(0, 0, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(m, 0, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(2 * m + 2, 0, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(3 * m + 2, 0, m_Footprints[FT_BLOCK]));

	// Inner blocks (only used in finest level)
	m_FootprintPos.push_back(KClipmapFootprintPos(m + 1, 2 * m + 1, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(2 * m + 1, 2 * m + 1, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(m + 1, m + 1, m_Footprints[FT_BLOCK]));
	m_FootprintPos.push_back(KClipmapFootprintPos(2 * m + 1, m + 1, m_Footprints[FT_BLOCK]));

	// Fix-up footprints (order: top, left, right, bottom)
	m_FootprintPos.push_back(KClipmapFootprintPos(2 * m, 3 * m + 2, m_Footprints[FT_FIXUP_VERTICAL]));
	m_FootprintPos.push_back(KClipmapFootprintPos(0, 2 * m, m_Footprints[FT_FIXUP_HORIZONTAL]));
	m_FootprintPos.push_back(KClipmapFootprintPos(3 * m + 2, 2 * m, m_Footprints[FT_FIXUP_HORIZONTAL]));
	m_FootprintPos.push_back(KClipmapFootprintPos(2 * m, 0, m_Footprints[FT_FIXUP_VERTICAL]));

	// Interior trims (order: top, left, right, bottom)
	m_FootprintPos.push_back(KClipmapFootprintPos(m, 3 * m + 1, m_Footprints[FT_INTERIORTRIM_HORIZONTAL]));
	m_FootprintPos.push_back(KClipmapFootprintPos(m, m, m_Footprints[FT_INTERIORTRIM_VERTICAL]));
	m_FootprintPos.push_back(KClipmapFootprintPos(3 * m + 1, m, m_Footprints[FT_INTERIORTRIM_VERTICAL]));
	m_FootprintPos.push_back(KClipmapFootprintPos(m, m, m_Footprints[FT_INTERIORTRIM_HORIZONTAL]));

	// Outer degenerated ring
	m_FootprintPos.push_back(KClipmapFootprintPos(0, 0, m_Footprints[FT_OUTER_DEGENERATERING]));
}

void KClipmap::InitializeClipmapLevel()
{
	m_ClipLevels.resize(m_LevelCount);
	for (uint32_t i = 0; i < m_LevelCount; ++i)
	{
		m_ClipLevels[i] = KClipmapLevelPtr(new KClipmapLevel(this, i));
		m_ClipLevels[i]->UpdateHeightData();
	}
}

void KClipmap::Init(const glm::vec3& center, float size, uint32_t gridLevel, uint32_t divideLevel)
{
	UnInit();

	m_GridWorldCenter = center;
	m_ClipWorldCenter = glm::vec3(0);

	m_GridCount = (1 << gridLevel) - 1;
	m_LevelCount = divideLevel;

	m_Size = size;
	m_GridWorldSize = m_Size / m_GridCount;

	// 临时随便赋个值 具体还是需要高度图与相机计算
	m_GridCenterX = (1 << (m_LevelCount - 1)) * (m_GridCount / 2);
	m_GridCenterY = (1 << (m_LevelCount - 1)) * (m_GridCount / 2);
	m_ClipCenterX = m_GridCenterX;
	m_ClipCenterY = m_GridCenterY;

	InitializeFootprint();
	InitializeFootprintPos();
	InitializeClipmapLevel();
}

void KClipmap::UnInit()
{
	for (uint32_t i = 0; i < FT_COUNT; ++i)
	{
		SAFE_UNINIT(m_Footprints[i]);
	}
	m_FootprintPos.clear();

	for (size_t i = 0; i < m_ClipLevels.size(); ++i)
	{
	}
	m_ClipLevels.clear();
}

void KClipmap::LoadHeightMap(const std::string& file)
{
	m_HeightMap.Init(file.c_str());
	m_GridCenterX = m_HeightMap.GetWidth() / 2;
	m_GridCenterY = m_HeightMap.GetHeight() / 2;
}

void KClipmap::Update(const glm::vec3& cameraPos)
{
	const glm::vec3 biasFromCenter = cameraPos - m_GridWorldCenter;

	int32_t biasXFromCenter = (int32_t)floor(biasFromCenter.x / m_GridWorldSize);
	int32_t biasYFromCenter = (int32_t)floor(biasFromCenter.z / m_GridWorldSize);

	m_ClipCenterX = m_GridCenterX + biasXFromCenter;
	m_ClipCenterY = m_GridCenterY + biasYFromCenter;

	m_ClipWorldCenter.x = m_GridWorldCenter.x + biasXFromCenter * m_GridWorldSize;
	m_ClipWorldCenter.z = m_GridWorldCenter.z + biasYFromCenter * m_GridWorldSize;

	int32_t clipLevelX = m_ClipCenterX;
	int32_t clipLevelY = m_ClipCenterY;
	KClipmapLevel::TrimPosition trimPos = KClipmapLevel::TP_NONE;

	uint32_t blockCount = m_GridCount / 4;
	uint32_t& m = blockCount;

	for (int32_t level = m_LevelCount - 1; level >= 0; --level)
	{
		KClipmapLevelPtr clipLevel = m_ClipLevels[level];
		uint32_t clipGridSize = clipLevel->GetGridSize();

		if (level == m_LevelCount - 1)
		{
			clipLevelX -= m_GridCount / 2;
			clipLevelY -= m_GridCount / 2;
			trimPos = KClipmapLevel::TP_NONE;
		}
		else
		{
			uint32_t x = m_ClipCenterX / clipLevel->GetGridSize();
			uint32_t y = m_ClipCenterY / clipLevel->GetGridSize();

			clipLevelX -= (m - 1) * clipLevel->GetGridSize();
			clipLevelY -= (m - 1) * clipLevel->GetGridSize();

			if (x & 4)
			{
				clipLevelX -= clipLevel->GetGridSize();
				if (y & 4)
				{
					clipLevelY -= clipLevel->GetGridSize();
					trimPos = KClipmapLevel::TP_BOTTOM_LEFT;
				}
				else
				{
					trimPos = KClipmapLevel::TP_TOP_LEFT;
				}
			}
			else
			{
				if (y & 4)
				{
					clipLevelY -= clipLevel->GetGridSize();
					trimPos = KClipmapLevel::TP_BOTTOM_RIGHT;
				}
				else
				{
					trimPos = KClipmapLevel::TP_TOP_RIGHT;
				}
			}
		}

		// KLog::Logger->Log(LL_DEBUG, "Clipmap update level:[%d] x:[%d] y:[%d]", level, clipLevelX, clipLevelY);

		clipLevel->SetPosition(clipLevelX, clipLevelY, trimPos);
		clipLevel->UpdateHeightData();
	}
}