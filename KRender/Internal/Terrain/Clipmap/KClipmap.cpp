#include "KClipmap.h"
#include "KBase/Interface/IKLog.h"
#include <Internal/KRenderGlobal.h>
#include "glm/glm.hpp"

const VertexFormat KClipmap::ms_VertexFormats[] = { VF_TERRAIN_POS };

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

void KClipmapFootprint::CreateData(const std::vector<glm::vec2>& verts, const std::vector<uint16_t>& idxs)
{
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

	CreateData(verts, idxs);
}

void KClipmapFootprint::Init(uint32_t width)
{
	UnInit();

	m_Width = width;
	m_Height = width;

	std::vector<glm::vec2> verts;
	std::vector<uint16_t> idxs;

	// bottom
	for (uint32_t x = 0; x < m_Width; ++x)
		verts.push_back(glm::vec2(x, 0));
	// right
	for (uint32_t y = 1; y < m_Height; ++y)
		verts.push_back(glm::vec2(m_Width - 1, y));
	// top
	for (uint32_t x = m_Width - 1; x >= 1; --x)
		verts.push_back(glm::vec2(x - 1, m_Height - 1));
	// left
	for (uint32_t y = m_Width - 1; y >= 1; --y)
		verts.push_back(glm::vec2(0, y - 1));

	for (uint16_t idx = 0; idx < (uint16_t)verts.size(); ++idx)
	{
		idxs.push_back(idx);
		if (idx != 0 && idx % (m_Width - 1) == 0)
		{
			idxs.push_back(idx);
		}
	}
	idxs.push_back(0);

	CreateData(verts, idxs);
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
	, m_GridCount(1)
	, m_BottomLeftX(0)
	, m_BottomLeftY(0)
	, m_TextureBiasX(0)
	, m_TextureBiasY(0)
	, m_TrimLocation(TL_NONE)
	, m_WorldStartScale(glm::vec4(0))
	, m_Texture(nullptr)
{
	uint32_t levelCount = m_Parent->GetLevelCount();

	m_GridCount = m_Parent->GetGridCount();
	m_GridSize = 1 << (levelCount - 1 - levelIdx);

	m_BottomLeftX = m_Parent->GetClipCenterX() - m_GridSize * (m_GridCount / 2);
	m_BottomLeftY = m_Parent->GetClipCenterY() - m_GridSize * (m_GridCount / 2);

	m_ClipHeightData.resize(m_GridCount * m_GridCount);
}

KClipmapLevel::~KClipmapLevel()
{
	ASSERT_RESULT(!m_Texture);
}

void KClipmapLevel::SetPosition(int32_t bottomLeftX, int32_t bottomLeftY, TrimLocation trim)
{
	m_BottomLeftX = bottomLeftX;
	m_BottomLeftY = bottomLeftY;
	m_TrimLocation = trim;

	glm::vec3 clipCenter = m_Parent->GetClipWorldCenter();
	glm::vec2 baseGridSize = m_Parent->GetBaseGridSize();
	int32_t clipCenterX = m_Parent->GetClipCenterX();
	int32_t clipCenterY = m_Parent->GetClipCenterY();
	int32_t diffX = m_BottomLeftX - clipCenterX;
	int32_t diffY = m_BottomLeftY - clipCenterY;

	m_WorldStartScale.x = clipCenter.x + diffX * baseGridSize.x;
	m_WorldStartScale.y = clipCenter.z + diffY * baseGridSize.y;
	m_WorldStartScale.z = (float)m_GridSize * baseGridSize.x;
	m_WorldStartScale.w = (float)m_GridSize * baseGridSize.y;
}

void KClipmapLevel::Init()
{
	UnInit();
	KRenderGlobal::RenderDevice->CreateTexture(m_Texture);

	m_Texture->InitMemoryFromData(nullptr, m_GridCount, m_GridCount, 1, IF_R32G32_FLOAT, false, false, false);
	m_Texture->InitDevice(false);
}

void KClipmapLevel::UnInit()
{
	SAFE_UNINIT(m_Texture);
}

void KClipmapLevel::UpdateTexture()
{
	KRenderGlobal::RenderDevice->Wait();

	std::vector<glm::vec2> textureDatas;
	textureDatas.resize(m_GridCount * m_GridCount);

	KClipmapLevelPtr upperClipmap = (m_LevelIdx > 0) ? m_Parent->GetClipmapLevel(m_LevelIdx - 1) : nullptr;

	for (uint32_t j = 0; j < m_GridCount; ++j)
	{
		int32_t y = m_BottomLeftY + j * m_GridSize;
		for (uint32_t i = 0; i < m_GridCount; ++i)
		{
			int32_t x = m_BottomLeftX + i * m_GridSize;
			float height = m_ClipHeightData[j * m_GridCount + i];
			float upperHeight = upperClipmap ? upperClipmap->GetHeight(x, y) : height;
			textureDatas[j * m_GridCount + i] = glm::vec2(height, upperHeight);
		}
	}

	m_Texture->UnInit();
	m_Texture->InitMemoryFromData(textureDatas.data(), m_GridCount, m_GridCount, 1, IF_R32G32_FLOAT, false, false, false);
	m_Texture->InitDevice(false);
}

void KClipmapLevel::UpdateHeightData()
{
	const KHeightMap& heightmap = m_Parent->GetHeightMap();

	for (uint32_t j = 0; j < m_GridCount; ++j)
	{
		int32_t y = m_BottomLeftY + j * m_GridSize;
		for (uint32_t i = 0; i < m_GridCount; ++i)
		{
			int32_t x = m_BottomLeftX + i * m_GridSize;
			m_ClipHeightData[j * m_GridCount + i] = heightmap.GetData(x, y);
			ASSERT_RESULT(abs(GetHeight(x, y) - m_ClipHeightData[j * m_GridCount + i]) < 1e-5f);
		}
	}
}

float KClipmapLevel::GetHeight(int32_t x, int32_t y) const
{
	float clip_x = (float)(x - m_BottomLeftX) / m_GridSize;
	float clip_y = (float)(y - m_BottomLeftY) / m_GridSize;

	if (clip_x < 0 || clip_x >= m_GridCount || clip_y < 0 || clip_y >= m_GridCount)
		return 0;

	int32_t clip_x_floor = (int32_t)floor(clip_x);
	int32_t clip_y_floor = (int32_t)floor(clip_y);
	int32_t clip_x_ceil = (int32_t)ceil(clip_x);
	int32_t clip_y_ceil = (int32_t)ceil(clip_y);

	float s = clip_x - clip_x_floor;
	float t = clip_y - clip_y_floor;

	float h0 = m_ClipHeightData[clip_y_floor * m_GridCount + clip_x_floor];
	float h1 = m_ClipHeightData[clip_y_floor * m_GridCount + clip_x_ceil];
	float h2 = m_ClipHeightData[clip_y_ceil * m_GridCount + clip_x_floor];
	float h3 = m_ClipHeightData[clip_y_ceil * m_GridCount + clip_x_ceil];

	float h = (h0 * (1 - s) + h1 * s) * (1 - t) + (h2 * (1 - s) + h3 * s) * t;
	return h;
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
	, m_BaseGridSize(glm::vec2(1.0f))
	, m_Size(1024.0f)
	, m_HeightScale(256.0f)
	, m_Updated(false)
{
}

KClipmap::~KClipmap()
{
	ASSERT_RESULT(!m_VSShader);
	ASSERT_RESULT(!m_FSShader);
}

KClipmapFootprintPtr KClipmap::CreateFootprint(uint32_t width, uint32_t height)
{
	KClipmapFootprintPtr footptint = KClipmapFootprintPtr(new KClipmapFootprint());
	footptint->Init(width, height);
	return footptint;
}

KClipmapFootprintPtr KClipmap::CreateFootprint(uint32_t width)
{
	KClipmapFootprintPtr footptint = KClipmapFootprintPtr(new KClipmapFootprint());
	footptint->Init(width);
	return footptint;
}

void KClipmap::InitializeFootprint()
{
	uint32_t m = GetBlockCount();
	m_Footprints[FT_BLOCK] = CreateFootprint(m, m);
	m_Footprints[FT_FIXUP_HORIZONTAL] = CreateFootprint(m, 3);
	m_Footprints[FT_FIXUP_VERTICAL] = CreateFootprint(3, m);
	m_Footprints[FT_INTERIORTRIM_HORIZONTAL] = CreateFootprint((2 * m) + 1, 2);
	m_Footprints[FT_INTERIORTRIM_VERTICAL] = CreateFootprint(2, (2 * m) + 1);
	m_Footprints[FT_OUTER_DEGENERATERING] = CreateFootprint((4 * m) - 1);
}

void KClipmap::InitializeFootprintPos()
{
	uint32_t m = GetBlockCount() - 1;

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
		m_ClipLevels[i]->Init();
		m_ClipLevels[i]->UpdateHeightData();
	}
}

void KClipmap::InitializePipeline()
{
	KRenderGlobal::RenderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	KRenderGlobal::RenderDevice->CreateCommandBuffer(m_CommandBuffer);
	m_CommandBuffer->Init(m_CommandPool, CBL_SECONDARY);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "terrain/clipmap.vert", m_VSShader, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "terrain/clipmap.frag", m_FSShader, false);

	KRenderGlobal::RenderDevice->CreateSampler(m_Sampler);

	m_Sampler->SetAddressMode(AM_REPEAT, AM_REPEAT, AM_REPEAT);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->Init(0, 0);

	m_ClipLevelPipelines.resize(m_ClipLevels.size());
	for (size_t i = 0; i < m_ClipLevels.size(); ++i)
	{
		KClipmapLevelPtr& clipmapLevel = m_ClipLevels[i];
		IKPipelinePtr& pipeline = m_ClipLevelPipelines[i];

		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_STRIP);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);
		pipeline->SetShader(ST_VERTEX, m_VSShader);
		pipeline->SetShader(ST_FRAGMENT, m_FSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		IKTexturePtr texture = clipmapLevel->GetTexture();

		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, texture->GetFrameBuffer(), m_Sampler, true);

		pipeline->Init();
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

	InitializeFootprint();
	InitializeFootprintPos();
	InitializeClipmapLevel();
	InitializePipeline();
}

void KClipmap::UnInit()
{
	for (uint32_t i = 0; i < FT_COUNT; ++i)
	{
		SAFE_UNINIT(m_Footprints[i]);
	}
	m_FootprintPos.clear();

	KRenderGlobal::ShaderManager.Release(m_VSShader);
	KRenderGlobal::ShaderManager.Release(m_FSShader);

	SAFE_UNINIT_CONTAINER(m_ClipLevels);
	SAFE_UNINIT_CONTAINER(m_ClipLevelPipelines);

	SAFE_UNINIT(m_Sampler);

	SAFE_UNINIT(m_CommandBuffer);
	SAFE_UNINIT(m_CommandPool);

	m_Updated = false;
}

void KClipmap::LoadHeightMap(const std::string& file)
{
	m_HeightMap.Init(file.c_str());
	m_GridCenterX = (m_HeightMap.GetWidth() - 1) / 2;
	m_GridCenterY = (m_HeightMap.GetHeight() - 1) / 2;
	m_BaseGridSize.x = m_Size / m_HeightMap.GetWidth();
	m_BaseGridSize.y = m_Size / m_HeightMap.GetHeight();
}

void KClipmap::Update(const glm::vec3& cameraPos)
{
	const glm::vec3 biasFromCenter = cameraPos - m_GridWorldCenter;
	const uint32_t gridScale = 1 << (m_LevelCount - 1);
	const glm::vec2 baseLevelGridSize = m_BaseGridSize * (float)gridScale;

	int32_t biasXFromCenter = (int32_t)floor(biasFromCenter.x / baseLevelGridSize.x);
	int32_t biasYFromCenter = (int32_t)floor(biasFromCenter.z / baseLevelGridSize.y);

	int32_t newClipCenterX = m_GridCenterX + biasXFromCenter * gridScale;
	int32_t newClipCenterY = m_GridCenterY + biasYFromCenter * gridScale;

	if (m_Updated && newClipCenterX == m_ClipCenterX && newClipCenterY == m_ClipCenterY)
		return;

	m_ClipCenterX = newClipCenterX;
	m_ClipCenterY = newClipCenterY;

	m_ClipWorldCenter.x = m_GridWorldCenter.x + biasXFromCenter * baseLevelGridSize.x;
	m_ClipWorldCenter.z = m_GridWorldCenter.z + biasYFromCenter * baseLevelGridSize.y;

	int32_t clipLevelX = m_ClipCenterX;
	int32_t clipLevelY = m_ClipCenterY;
	KClipmapLevel::TrimLocation trim = KClipmapLevel::TL_NONE;

	uint32_t m = GetBlockCount();

	for (int32_t level = m_LevelCount - 1; level >= 0; --level)
	{
		KClipmapLevelPtr clipLevel = m_ClipLevels[level];
		uint32_t clipGridSize = clipLevel->GetGridSize();

		if (level == m_LevelCount - 1)
		{
			clipLevelX -= m_GridCount / 2;
			clipLevelY -= m_GridCount / 2;
			trim = KClipmapLevel::TL_NONE;
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
					trim = KClipmapLevel::TL_BOTTOM_LEFT;
				}
				else
				{
					trim = KClipmapLevel::TL_TOP_LEFT;
				}
			}
			else
			{
				if (y & 4)
				{
					clipLevelY -= clipLevel->GetGridSize();
					trim = KClipmapLevel::TL_BOTTOM_RIGHT;
				}
				else
				{
					trim = KClipmapLevel::TL_TOP_RIGHT;
				}
			}
		}

		// KLog::Logger->Log(LL_DEBUG, "Clipmap update level:[%d] x:[%d] y:[%d]", level, clipLevelX, clipLevelY);

		clipLevel->SetPosition(clipLevelX, clipLevelY, trim);
		clipLevel->UpdateHeightData();
	}

	for (int32_t level = m_LevelCount - 1; level >= 0; --level)
	{
		m_ClipLevels[level]->UpdateTexture();
	}

	m_Updated = true;
}

bool KClipmap::Render(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers)
{
	m_CommandBuffer->BeginSecondary(renderPass);
	m_CommandBuffer->SetViewport(renderPass->GetViewPort());

	for (uint32_t levelIdx = 0; levelIdx < m_LevelCount; ++levelIdx)
	{
		KClipmapLevelPtr& clipLevel = m_ClipLevels[levelIdx];
		glm::vec4 worldStartScale = clipLevel->GetWorldStartScale();

		for (size_t footprintIdx = 0; footprintIdx < m_FootprintPos.size(); ++footprintIdx)
		{
			// [block]  0 - 11
			//          top left right bottom
			// [inner]  12  13   14    15
			// [fixup]  16  17   18    19
			// [trim]   20  21   22    23
			// [outer]  22
			if (levelIdx != m_LevelCount - 1)
			{
				if (footprintIdx >= 12 && footprintIdx <= 15)
					continue;

				if (footprintIdx >= 20)
				{
					KClipmapLevel::TrimLocation trim = clipLevel->GetTrimLocation();
					ASSERT_RESULT(trim != KClipmapLevel::TL_NONE);
					if (trim == KClipmapLevel::TL_BOTTOM_LEFT)
					{
						if (footprintIdx == 20 || footprintIdx == 22)
							continue;
					}
					else if (trim == KClipmapLevel::TL_BOTTOM_RIGHT)
					{
						if (footprintIdx == 20 || footprintIdx == 21)
							continue;
					}
					else if (trim == KClipmapLevel::TL_TOP_LEFT)
					{
						if (footprintIdx == 22 || footprintIdx == 23)
							continue;
					}
					else if (trim == KClipmapLevel::TL_TOP_RIGHT)
					{
						if (footprintIdx == 21 || footprintIdx == 23)
							continue;
					}
				}
			}

			KClipmapFootprintPos& footprintPos = m_FootprintPos[footprintIdx];
			KClipmapFootprintPtr footprint = footprintPos.GetFootPrint();

			KRenderCommand command;

			command.vertexData = &footprint->GetVertexData();
			command.indexData = &footprint->GetIndexData();
			command.pipeline = m_ClipLevelPipelines[levelIdx];

			struct TerrainObjectData
			{
				glm::vec4 worldStartScale;
				glm::vec4 misc;
				glm::vec4 misc2;
			} objectData;

			objectData.worldStartScale = worldStartScale;
			objectData.misc.x = (float)footprintPos.GetPosX();
			objectData.misc.y = (float)footprintPos.GetPosY();
			objectData.misc.z = (float)m_GridCount;
			objectData.misc.w = m_HeightScale;

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(objectData);

			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

			command.pipeline->GetHandle(renderPass, command.pipelineHandle);
			command.indexDraw = true;

			m_CommandBuffer->Render(command);
		}
	}

	m_CommandBuffer->End();

	buffers.push_back(m_CommandBuffer);
	return true;
}

KClipmapLevelPtr KClipmap::GetClipmapLevel(uint32_t idx)
{
	if (idx < m_ClipLevels.size())
		return m_ClipLevels[idx];
	return nullptr;
}