#include "KClipmap.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Publish/KMath.h"
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

void KClipmapFootprint::Init(int32_t width, int32_t height)
{
	UnInit();

	m_Width = width;
	m_Height = height;

	std::vector<glm::vec2> verts;
	std::vector<uint16_t> idxs;

	for (int32_t y = 0; y < m_Height; ++y)
	{
		for (int32_t x = 0; x < m_Width; ++x)
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

void KClipmapFootprint::Init(int32_t width)
{
	UnInit();

	m_Width = width;
	m_Height = width;

	std::vector<glm::vec2> verts;
	std::vector<uint16_t> idxs;

	// bottom
	for (int32_t x = 0; x < m_Width; ++x)
		verts.push_back(glm::vec2(x, 0));
	// right
	for (int32_t y = 1; y < m_Height; ++y)
		verts.push_back(glm::vec2(m_Width - 1, y));
	// top
	for (int32_t x = m_Width - 1; x >= 1; --x)
		verts.push_back(glm::vec2(x - 1, m_Height - 1));
	// left
	for (int32_t y = m_Width - 1; y >= 1; --y)
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

const VertexFormat KClipmapLevel::ms_VertexFormats[] = { VF_SCREENQUAD_POS };

const KVertexDefinition::SCREENQUAD_POS_2F KClipmapLevel::ms_UpdateVertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KClipmapLevel::ms_UpdateIndices[] = { 0, 1, 2, 2, 3, 0 };

IKVertexBufferPtr KClipmapLevel::ms_UpdateVertexBuffer = nullptr;
IKIndexBufferPtr KClipmapLevel::ms_UpdateIndexBuffer = nullptr;
IKSamplerPtr KClipmapLevel::ms_Sampler = nullptr;
KShaderRef KClipmapLevel::ms_UpdateVS;
KShaderRef KClipmapLevel::ms_UpdateFS;

KVertexData KClipmapLevel::ms_UpdateVertexData;
KIndexData KClipmapLevel::ms_UpdateIndexData;

KClipmapLevel::KClipmapLevel(KClipmap* parent, int32_t levelIdx)
	: m_Parent(parent)
	, m_LevelIdx(levelIdx)
	, m_GridSize(1)
	, m_GridCount(1)
	, m_BottomLeftX(0)
	, m_BottomLeftY(0)
	, m_ScrollX(0)
	, m_ScrollY(0)
	, m_NewScrollX(0)
	, m_NewScrollY(0)
	, m_TrimLocation(TL_NONE)
	, m_ClipStartScale(glm::vec4(0))
#ifdef _DEBUG
	, m_EnableUpdateDebug(false)
#else
	, m_EnableUpdateDebug(false)
#endif
	, m_ComputeShaderUpdate(false)
	, m_DisableScroll(false)
{
	int32_t levelCount = m_Parent->GetLevelCount();

	m_GridCount = m_Parent->GetGridCount();
	m_GridSize = 1 << (levelCount - 1 - levelIdx);

	m_BottomLeftX = m_Parent->GetClipCenterX() - m_GridSize * (m_GridCount / 2);
	m_BottomLeftY = m_Parent->GetClipCenterY() - m_GridSize * (m_GridCount / 2);

	m_ClipHeightData.resize(m_GridCount * m_GridCount);
}

KClipmapLevel::~KClipmapLevel()
{
}

void KClipmapLevel::TrimUpdateRect(const KClipmapUpdateRect& trim, KClipmapUpdateRect& rect)
{
	if (rect.startX >= trim.startX && rect.endX <= trim.endX)
	{
		if (rect.startY <= trim.startY)
		{
			rect.endY = std::min(rect.endY, trim.startY);
		}
		else
		{
			rect.startY = std::max(rect.startY, trim.endY);
		}
	}
	if (rect.startY >= trim.startY && rect.endY <= trim.endY)
	{
		if (rect.startX <= trim.startX)
		{
			rect.endX = std::min(rect.endX, trim.startX);
		}
		else
		{
			rect.startX = std::max(rect.startX, trim.endX);
		}
	}
}

void KClipmapLevel::TrimUpdateRects(std::vector<KClipmapUpdateRect>& rects)
{
	for (size_t i = 0; i < rects.size(); ++i)
	{
		for (size_t j = i + 1; j < rects.size(); ++j)
		{
			TrimUpdateRect(rects[i], rects[j]);
		}
	}
}

void KClipmapLevel::UpdateTextureByRect(const std::vector<KClipmapTextureUpdateRect>& rects)
{
	// TODO Layout

	KClipmapLevelPtr upperClipmap = m_Parent->GetClipmapLevel((m_LevelIdx > 0) ? (m_LevelIdx - 1) : 0);

	for (size_t rectIdx = 0; rectIdx < rects.size(); ++rectIdx)
	{
		assert(rectIdx < 4);
		const KClipmapTextureUpdateRect& rect = rects[rectIdx];

		IKTexturePtr updateTexture = m_UpdateTextures[rectIdx];

		int32_t numRow = rect.endY - rect.startY + 1;
		int32_t numCol = rect.endX - rect.startX + 1;

		std::vector<glm::vec2> combinedDatas;
		combinedDatas.resize(numRow * numCol);

		for (int32_t j = 0; j < numRow; ++j)
		{
			int32_t y = TextureCoordYToWorldY(rect.startY + j);
			for (int32_t i = 0; i < numCol; ++i)
			{
				int32_t x = TextureCoordXToWorldX(rect.startX + i);
				float height = GetHeight(x, y);
				float upperHeight = upperClipmap->GetHeight(x, y);
				combinedDatas[j * numCol + i] = glm::vec2(height, upperHeight);
			}
		}

		updateTexture->UnInit();
		updateTexture->InitMemoryFromData(combinedDatas.data(), "ClipmapLevelUpdate_" + std::to_string(rectIdx), numCol, numRow, 1, IF_R32G32_FLOAT, false, false, false);
		updateTexture->InitDevice(false);
	}

	IKCommandBufferPtr commandBuffer = KRenderGlobal::CommandPool->Request(CBL_PRIMARY);

	if (m_ComputeShaderUpdate)
	{
		commandBuffer->BeginPrimary();
		commandBuffer->Transition(m_TextureTarget->GetFrameBuffer(), PIPELINE_STAGE_BOTTOM_OF_PIPE, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_GENERAL);

		for (size_t rectIdx = 0; rectIdx < rects.size(); ++rectIdx)
		{
			const KClipmapTextureUpdateRect& rect = rects[rectIdx];
			int32_t numRow = rect.endY - rect.startY + 1;
			int32_t numCol = rect.endX - rect.startX + 1;

			uint32_t groupX = (numCol + (UPDATE_GROUP_SIZE - 1)) / UPDATE_GROUP_SIZE;
			uint32_t groupY = (numRow + (UPDATE_GROUP_SIZE - 1)) / UPDATE_GROUP_SIZE;

			glm::ivec4 constant = glm::ivec4(rect.startX, rect.startY, numCol, numRow);

			KDynamicConstantBufferUsage usage;
			usage.binding = SHADER_BINDING_OBJECT;
			usage.range = sizeof(constant);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

			IKComputePipelinePtr& computePipeline = m_UpdateComputePipelines[rectIdx];

			commandBuffer->Transition(m_UpdateTextures[rectIdx]->GetFrameBuffer(), PIPELINE_STAGE_BOTTOM_OF_PIPE, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_GENERAL);
			computePipeline->Execute(commandBuffer, groupX, groupY, 1, &usage);
			commandBuffer->Transition(m_UpdateTextures[rectIdx]->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}

		commandBuffer->Transition(m_TextureTarget->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		commandBuffer->End();
		commandBuffer->Flush();
	}
	else
	{
		commandBuffer->BeginPrimary();
		commandBuffer->Transition(m_TextureTarget->GetFrameBuffer(), PIPELINE_STAGE_BOTTOM_OF_PIPE, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		commandBuffer->BeginDebugMarker("ClipmapUpdate", glm::vec4(1));
		commandBuffer->BeginRenderPass(m_UpdateRenderPass, SUBPASS_CONTENTS_INLINE);
		commandBuffer->SetViewport(m_UpdateRenderPass->GetViewPort());

		IKPipelineHandlePtr pipeHandle = nullptr;
		KRenderCommand command;
		for (size_t rectIdx = 0; rectIdx < rects.size(); ++rectIdx)
		{
			const KClipmapTextureUpdateRect& rect = rects[rectIdx];

			KViewPortArea updateViewport;
			updateViewport.x = rect.startX;
			updateViewport.y = rect.startY;
			updateViewport.width = rect.endX - rect.startX + 1;
			updateViewport.height = rect.endY - rect.startY + 1;

			commandBuffer->SetViewport(updateViewport);

			m_UpdatePipelines[rectIdx]->GetHandle(m_UpdateRenderPass, pipeHandle);

			struct ObjectData
			{
				glm::vec4 area;
			} objectData;

			objectData.area.x = (float)(rect.endX - rect.startX + 1);
			objectData.area.y = (float)(rect.endY - rect.startY + 1);

			KDynamicConstantBufferUsage objectUsage;
			objectUsage.binding = SHADER_BINDING_OBJECT;
			objectUsage.range = sizeof(objectData);

			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

			command.dynamicConstantUsages.push_back(objectUsage);

			command.vertexData = &ms_UpdateVertexData;
			command.indexData = &ms_UpdateIndexData;
			command.pipeline = m_UpdatePipelines[rectIdx];
			command.pipelineHandle = pipeHandle;
			command.indexDraw = true;
			commandBuffer->Render(command);
		}

		commandBuffer->EndRenderPass();
		commandBuffer->EndDebugMarker();
		commandBuffer->End();

		commandBuffer->Flush();
	}
}

void KClipmapLevel::UpdateClipStartScale()
{
	glm::vec3 clipCenter = m_Parent->GetClipWorldCenter();
	glm::vec2 baseGridSize = m_Parent->GetBaseGridSize();
	int32_t clipCenterX = m_Parent->GetClipCenterX();
	int32_t clipCenterY = m_Parent->GetClipCenterY();
	int32_t diffX = m_BottomLeftX + m_ScrollX * m_GridSize - clipCenterX;
	int32_t diffY = m_BottomLeftY + m_ScrollY * m_GridSize - clipCenterY;

	m_ClipStartScale.x = clipCenter.x + diffX * baseGridSize.x;
	m_ClipStartScale.y = clipCenter.z + diffY * baseGridSize.y;
	m_ClipStartScale.z = (float)m_GridSize * baseGridSize.x;
	m_ClipStartScale.w = (float)m_GridSize * baseGridSize.y;
}

void KClipmapLevel::InitializePipeline()
{
	for (int32_t i = 0; i < 4; ++i)
	{
		IKPipelinePtr& pipeline = m_UpdatePipelines[i];
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		IKTexturePtr& updateTexture = m_UpdateTextures[i];
		KRenderGlobal::RenderDevice->CreateTexture(updateTexture);
		updateTexture->InitMemoryFromData(nullptr, "ClipmapLevelUpdate_" + std::to_string(i), 1, 1, 1, IF_R32G32_FLOAT, false, false, false);
		updateTexture->InitDevice(false);

		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);
		pipeline->SetShader(ST_VERTEX, *ms_UpdateVS);
		pipeline->SetShader(ST_FRAGMENT, *ms_UpdateFS);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, updateTexture->GetFrameBuffer(), ms_Sampler, true);
		pipeline->Init();

		IKComputePipelinePtr& comp = m_UpdateComputePipelines[i];
		KRenderGlobal::RenderDevice->CreateComputePipeline(comp);
		comp->BindStorageImage(BINDING_IMAGE_IN, updateTexture->GetFrameBuffer(), EF_R32G32_FLOAT, COMPUTE_RESOURCE_IN, 0, true);
		comp->BindStorageImage(BINDING_IMAGE_OUT, m_TextureTarget->GetFrameBuffer(), EF_R32G32_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);
		comp->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
		comp->Init("terrain/clip_update.comp", KShaderCompileEnvironment());
	}
}

void KClipmapLevel::SetPosition(int32_t bottomLeftX, int32_t bottomLeftY, TrimLocation trim)
{
	m_BottomLeftX = bottomLeftX;
	m_BottomLeftY = bottomLeftY;
	m_ScrollX = m_NewScrollX = 0;
	m_ScrollY = m_NewScrollY = 0;
	m_TrimLocation = trim;
	UpdateClipStartScale();
}

int32_t KClipmapLevel::TextureCoordXToWorldX(int32_t i)
{
	if (m_ScrollX > 0)
	{
		if (i < m_ScrollX)
			return (m_GridCount + i) * m_GridSize + m_BottomLeftX;
		else
			return m_BottomLeftX + i * m_GridSize;
	}
	else if (m_ScrollX < 0)
	{
		if (i < m_GridCount + m_ScrollX)
			return m_BottomLeftX + i * m_GridSize;
		else
			return (i - m_GridCount) * m_GridSize + m_BottomLeftX;
	}
	else
	{
		return m_BottomLeftX + i * m_GridSize;
	}
}

int32_t KClipmapLevel::TextureCoordYToWorldY(int32_t j)
{
	if (m_ScrollY > 0)
	{
		if (j < m_ScrollY)
			return (m_GridCount + j) * m_GridSize + m_BottomLeftY;
		else
			return m_BottomLeftY + j * m_GridSize;
	}
	else if (m_ScrollY < 0)
	{
		if (j < m_GridCount + m_ScrollY)
			return m_BottomLeftY + j * m_GridSize;
		else
			return (j - m_GridCount) * m_GridSize + m_BottomLeftY;
	}
	else
	{
		return m_BottomLeftY + j * m_GridSize;
	}
}

int32_t KClipmapLevel::WorldXToTextureCoordX(int32_t x)
{
	return KMath::Mod_Positive((x - m_BottomLeftX) / m_GridSize, m_GridCount);
}

int32_t KClipmapLevel::WorldYToTextureCoordY(int32_t y)
{
	return KMath::Mod_Positive((y - m_BottomLeftY) / m_GridSize, m_GridCount);
}

void KClipmapLevel::ScrollPosition(int32_t bottomLeftX, int32_t bottomLeftY, TrimLocation trim)
{
	int32_t prevBottomLeftX = m_BottomLeftX + m_ScrollX * m_GridSize;
	int32_t prevBottomLeftY = m_BottomLeftY + m_ScrollY * m_GridSize;

	int32_t deltaX = bottomLeftX - prevBottomLeftX;
	int32_t deltaY = bottomLeftY - prevBottomLeftY;

	// 整个GridSize移动且不超过移动范围
	if ((deltaX != 0 || deltaY != 0)
		&& deltaX % m_GridSize == 0
		&& deltaY % m_GridSize == 0 
		&& abs(deltaX) < m_GridSize * m_GridCount
		&& abs(deltaY) < m_GridSize * m_GridCount)
	{
		m_NewScrollX = m_ScrollX + deltaX / m_GridSize;
		m_NewScrollY = m_ScrollY + deltaY / m_GridSize;
		m_TrimLocation = trim;
	}
	else
	{
		m_BottomLeftX = bottomLeftX;
		m_BottomLeftY = bottomLeftY;
		m_ScrollX = m_NewScrollX = 0;
		m_ScrollY = m_NewScrollY = 0;
		m_TrimLocation = trim;
	}
}

void KClipmapLevel::Init()
{
	UnInit();

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_TextureTarget);
	m_TextureTarget->InitFromColor(m_GridCount, m_GridCount, 1, 1, EF_R32G32_FLOAT);

	if (m_EnableUpdateDebug)
	{
		KRenderGlobal::RenderDevice->CreateTexture(m_DebugTexture);
		m_DebugTexture->InitMemoryFromData(nullptr, "ClipmapLevelDebug_" + std::to_string(m_LevelIdx), m_GridCount, m_GridCount, 1, IF_R32G32_FLOAT, false, false, false);
	}

	KRenderGlobal::RenderDevice->CreateRenderPass(m_UpdateRenderPass);
	m_UpdateRenderPass->SetColorAttachment(0, m_TextureTarget->GetFrameBuffer());
	m_UpdateRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_UpdateRenderPass->SetOpColor(0, LO_LOAD, SO_STORE);
	m_UpdateRenderPass->Init();

	InitializePipeline();
}

void KClipmapLevel::UnInit()
{
	SAFE_UNINIT(m_DebugTexture);
	SAFE_UNINIT(m_UpdateRenderPass);
	SAFE_UNINIT(m_TextureTarget);
	SAFE_UNINIT_CONTAINER(m_UpdateTextures);
	SAFE_UNINIT_CONTAINER(m_UpdatePipelines);
	SAFE_UNINIT_CONTAINER(m_UpdateComputePipelines);
}

void KClipmapLevel::InitShared()
{
	KRenderGlobal::RenderDevice->CreateVertexBuffer(ms_UpdateVertexBuffer);
	ms_UpdateVertexBuffer->InitMemory(ARRAY_SIZE(ms_UpdateVertices), sizeof(ms_UpdateVertices[0]), ms_UpdateVertices);
	ms_UpdateVertexBuffer->InitDevice(false);

	KRenderGlobal::RenderDevice->CreateIndexBuffer(ms_UpdateIndexBuffer);
	ms_UpdateIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_UpdateIndices), ms_UpdateIndices);
	ms_UpdateIndexBuffer->InitDevice(false);

	ms_UpdateVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, ms_UpdateVertexBuffer);
	ms_UpdateVertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	ms_UpdateVertexData.vertexCount = ARRAY_SIZE(ms_UpdateVertices);
	ms_UpdateVertexData.vertexStart = 0;

	ms_UpdateIndexData.indexBuffer = ms_UpdateIndexBuffer;
	ms_UpdateIndexData.indexCount = ARRAY_SIZE(ms_UpdateIndices);
	ms_UpdateIndexData.indexStart = 0;

	KRenderGlobal::RenderDevice->CreateSampler(ms_Sampler);
	ms_Sampler->SetAddressMode(AM_REPEAT, AM_REPEAT, AM_REPEAT);
	ms_Sampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	ms_Sampler->Init(0, 0);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "terrain/clip_update.vert", ms_UpdateVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "terrain/clip_update.frag", ms_UpdateFS, false);
}

void KClipmapLevel::UnInitShared()
{
	ms_UpdateVS.Release();
	ms_UpdateFS.Release();
	SAFE_UNINIT(ms_Sampler);
	SAFE_UNINIT(ms_UpdateVertexBuffer);
	SAFE_UNINIT(ms_UpdateIndexBuffer);
}

void KClipmapLevel::PopulateUpdateRects()
{
	m_UpdateRects.clear();

	// 增量更新
	if (m_NewScrollX != m_ScrollX || m_NewScrollY != m_ScrollY)
	{
		int32_t prevBottomLeftX = m_BottomLeftX + m_ScrollX * m_GridSize;
		int32_t prevBottomLeftY = m_BottomLeftY + m_ScrollY * m_GridSize;

		int32_t newBottomLeftX = m_BottomLeftX + m_NewScrollX * m_GridSize;
		int32_t newBottomLeftY = m_BottomLeftY + m_NewScrollY * m_GridSize;

		int32_t deltaX = newBottomLeftX - prevBottomLeftX;
		int32_t deltaY = newBottomLeftY - prevBottomLeftY;

		const int32_t gridLength = (m_GridCount - 1) * m_GridSize;

		m_ScrollX = m_NewScrollX;
		m_ScrollY = m_NewScrollY;

		if (m_ScrollX >= m_GridCount)
		{
			m_ScrollX -= m_GridCount;
			m_BottomLeftX += m_GridCount * m_GridSize;
		}
		if (m_ScrollX <= -m_GridCount)
		{
			m_ScrollX += m_GridCount;
			m_BottomLeftX -= m_GridCount * m_GridSize;
		}
		if (m_ScrollY >= m_GridCount)
		{
			m_ScrollY -= m_GridCount;
			m_BottomLeftY += m_GridCount * m_GridSize;
		}
		if (m_ScrollY <= -m_GridCount)
		{
			m_ScrollY += m_GridCount;
			m_BottomLeftY -= m_GridCount * m_GridSize;
		}

		m_NewScrollX = m_ScrollX;
		m_NewScrollY = m_ScrollY;

		if (m_DisableScroll)
		{
			KClipmapTextureUpdateRect rect;
			rect.startX = rect.startY = 0;
			rect.endX = rect.endY = m_GridCount - 1;
			m_UpdateRects = { rect };
		}
		else
		{
			if (deltaX != 0)
			{
				KClipmapMovementUpdateRect movement(newBottomLeftX, newBottomLeftY, newBottomLeftX + gridLength, newBottomLeftY + gridLength);
				if (deltaX > 0)
					movement.startX = std::max(movement.startX, prevBottomLeftX + gridLength);
				else
					movement.endX = std::min(movement.endX, prevBottomLeftX);

				KClipmapTextureUpdateRect rect;
				rect.startX = WorldXToTextureCoordX(movement.startX);
				rect.endX = WorldXToTextureCoordX(movement.endX);
				rect.startY = WorldYToTextureCoordY(movement.startY);
				rect.endY = WorldYToTextureCoordY(movement.endY);

				if (rect.startX <= rect.endX)
				{
					m_UpdateRects.push_back(KClipmapTextureUpdateRect(rect.startX, 0, rect.endX, m_GridCount - 1));
				}
				else
				{
					m_UpdateRects.push_back(KClipmapTextureUpdateRect(rect.startX, 0, m_GridCount - 1, m_GridCount - 1));
					m_UpdateRects.push_back(KClipmapTextureUpdateRect(0, 0, rect.endX, m_GridCount - 1));
				}
			}

			if (deltaY != 0)
			{
				KClipmapMovementUpdateRect movement(newBottomLeftX, newBottomLeftY, newBottomLeftX + gridLength, newBottomLeftY + gridLength);
				if (deltaY > 0)
					movement.startY = std::max(movement.startY, prevBottomLeftY + gridLength);
				else
					movement.endY = std::min(movement.endY, prevBottomLeftY);

				KClipmapTextureUpdateRect rect;
				rect.startX = WorldXToTextureCoordX(movement.startX);
				rect.endX = WorldXToTextureCoordX(movement.endX);
				rect.startY = WorldYToTextureCoordY(movement.startY);
				rect.endY = WorldYToTextureCoordY(movement.endY);

				if (rect.startY <= rect.endY)
				{
					m_UpdateRects.push_back(KClipmapTextureUpdateRect(0, rect.startY, m_GridCount - 1, rect.endY));
				}
				else
				{
					m_UpdateRects.push_back(KClipmapTextureUpdateRect(0, rect.startY, m_GridCount - 1, m_GridCount - 1));
					m_UpdateRects.push_back(KClipmapTextureUpdateRect(0, 0, m_GridCount - 1, rect.endY));
				}
			}

		}
		// TODO BUG
		// TrimUpdateRects(m_UpdateRects);
	}
	// 全量更新
	else
	{
		m_BottomLeftX += m_NewScrollX * m_GridSize;
		m_BottomLeftY += m_NewScrollY * m_GridSize;
		m_ScrollX = m_NewScrollX = 0;
		m_ScrollY = m_NewScrollY = 0;

		KClipmapTextureUpdateRect rect;
		rect.startX = rect.startY = 0;
		rect.endX = rect.endY = m_GridCount - 1;
		m_UpdateRects = { rect };
	}
}

void KClipmapLevel::UpdateHeightData()
{
	const KHeightMap& heightmap = m_Parent->GetHeightMap();
	for (KClipmapTextureUpdateRect& rect : m_UpdateRects)
	{
		for (int32_t j = rect.startY; j <= rect.endY; ++j)
		{
			int32_t y = TextureCoordYToWorldY(j);
			for (int32_t i = rect.startX; i <= rect.endX; ++i)
			{
				int32_t x = TextureCoordXToWorldX(i);
				m_ClipHeightData[j * m_GridCount + i] = heightmap.GetData(x, y);
				if (abs(GetHeight(x, y) - m_ClipHeightData[j * m_GridCount + i]) > 1e-5f)
				{
					assert(false && "should not reach");
				}
			}
		}
	}
}

float KClipmapLevel::GetClipHeight(float u, float v)
{
	if (u < 0 || u >= 1 || v < 0 || v >= 1)
		return 0;

	u = u + (float)m_ScrollX / m_GridCount;
	v = v + (float)m_ScrollY / m_GridCount;

	if (u < 0.0) u += 1.0f;
	if (u >= 1.0) u -= 1.0f;
	if (v < 0.0) v += 1.0f;
	if (v >= 1.0) v -= 1.0f;

	float x = u * m_GridCount;
	float y = v * m_GridCount;

	int32_t x_floor = std::max(0, (int32_t)floor(x));
	int32_t y_floor = std::max(0, (int32_t)floor(y));

	int32_t x_ceil = std::min((int32_t)ceil(x), m_GridCount - 1);
	int32_t y_ceil = std::min((int32_t)ceil(y), m_GridCount - 1);

	float s = x - x_floor;
	float t = y - y_floor;

	float h0 = m_ClipHeightData[y_floor * m_GridCount + x_floor];
	float h1 = m_ClipHeightData[y_floor * m_GridCount + x_ceil];
	float h2 = m_ClipHeightData[y_ceil * m_GridCount + x_floor];
	float h3 = m_ClipHeightData[y_ceil * m_GridCount + x_ceil];

	float h = (h0 * (1 - s) + h1 * s) * (1 - t) + (h2 * (1 - s) + h3 * s) * t;
	return h;
}

void KClipmapLevel::CheckHeightValid()
{
	if (m_DebugTexture)
	{

	}
}

void KClipmapLevel::UpdateTexture()
{
	UpdateTextureByRect(m_UpdateRects);
	m_UpdateRects.clear();

	if (m_EnableUpdateDebug)
	{
		std::vector<glm::vec2> textureDatas;
		textureDatas.resize(m_GridCount * m_GridCount);
		KClipmapLevelPtr upperClipmap = m_Parent->GetClipmapLevel((m_LevelIdx > 0) ? m_LevelIdx - 1 : 0);
		for (int32_t j = 0; j < m_GridCount; ++j)
		{
			int32_t y = TextureCoordYToWorldY(j);
			for (int32_t i = 0; i < m_GridCount; ++i)
			{
				int32_t x = TextureCoordXToWorldX(i);
				float height = m_ClipHeightData[j * m_GridCount + i];
				float upperHeight = upperClipmap->GetHeight(x, y);
				textureDatas[j * m_GridCount + i] = glm::vec2(height, upperHeight);
			}
		}

		m_DebugTexture->UnInit();
		m_DebugTexture->InitMemoryFromData(textureDatas.data(), "ClipmapLevelDebug_" + std::to_string(m_LevelIdx), m_GridCount, m_GridCount, 1, IF_R32G32_FLOAT, false, false, false);
		m_DebugTexture->InitDevice(false);
	}
}

float KClipmapLevel::GetHeight(int32_t x, int32_t y) const
{
	int32_t actualBottomLeftX = m_BottomLeftX + m_ScrollX * m_GridSize;
	int32_t actualBottomLeftY = m_BottomLeftY + m_ScrollY * m_GridSize;

	float idx_x = (float)(x - actualBottomLeftX) / m_GridSize;
	float idx_y = (float)(y - actualBottomLeftY) / m_GridSize;

	if (idx_x < 0 || idx_x >= m_GridCount || idx_y < 0 || idx_y >= m_GridCount)
		return 0;

	idx_x += m_ScrollX;
	idx_y += m_ScrollY;

	int32_t idx_x_floor = (int32_t)floor(idx_x);
	int32_t idx_y_floor = (int32_t)floor(idx_y);
	int32_t idx_x_ceil	= (int32_t)ceil(idx_x);
	int32_t idx_y_ceil	= (int32_t)ceil(idx_y);

	float s = idx_x - idx_x_floor;
	float t = idx_y - idx_y_floor;

	idx_x_floor = KMath::Mod_Positive(idx_x_floor, m_GridCount);
	idx_x_ceil	= KMath::Mod_Positive(idx_x_ceil, m_GridCount);
	idx_y_floor = KMath::Mod_Positive(idx_y_floor, m_GridCount);
	idx_y_ceil	= KMath::Mod_Positive(idx_y_ceil, m_GridCount);

	float h0 = m_ClipHeightData[idx_y_floor * m_GridCount + idx_x_floor];
	float h1 = m_ClipHeightData[idx_y_floor * m_GridCount + idx_x_ceil];
	float h2 = m_ClipHeightData[idx_y_ceil * m_GridCount + idx_x_floor];
	float h3 = m_ClipHeightData[idx_y_ceil * m_GridCount + idx_x_ceil];

	float h = (h0 * (1 - s) + h1 * s) * (1 - t) + (h2 * (1 - s) + h3 * s) * t;
	return h;
}

KClipmap::KClipmap()
	: m_GridCount(0)
	, m_LevelCount(0)
	, m_FinestLevel(0)
	, m_GridCenterX(0)
	, m_GridCenterY(0)
	, m_ClipCenterX(0)
	, m_ClipCenterY(0)
	, m_GridWorldCenter(glm::vec3(0))
	, m_ClipWorldCenter(glm::vec3(0))
	, m_BaseGridSize(glm::vec2(1.0f))
	, m_Size(1024.0f)
	, m_HeightScale(1024.0f)
	, m_Updated(false)
{
}

KClipmap::~KClipmap()
{
	ASSERT_RESULT(!m_VSShader);
	ASSERT_RESULT(!m_FSShader);
}

KClipmapFootprintPtr KClipmap::CreateFootprint(int32_t width, int32_t height)
{
	KClipmapFootprintPtr footptint = KClipmapFootprintPtr(KNEW KClipmapFootprint());
	footptint->Init(width, height);
	return footptint;
}

KClipmapFootprintPtr KClipmap::CreateFootprint(int32_t width)
{
	KClipmapFootprintPtr footptint = KClipmapFootprintPtr(KNEW KClipmapFootprint());
	footptint->Init(width);
	return footptint;
}

void KClipmap::InitializeFootprint()
{
	int32_t m = GetBlockCount();
	m_Footprints[FT_BLOCK] = CreateFootprint(m, m);
	m_Footprints[FT_FIXUP_HORIZONTAL] = CreateFootprint(m, 3);
	m_Footprints[FT_FIXUP_VERTICAL] = CreateFootprint(3, m);
	m_Footprints[FT_INTERIORTRIM_HORIZONTAL] = CreateFootprint((2 * m) + 1, 2);
	m_Footprints[FT_INTERIORTRIM_VERTICAL] = CreateFootprint(2, (2 * m) + 1);
	m_Footprints[FT_OUTER_DEGENERATERING] = CreateFootprint((4 * m) - 1);
	m_Footprints[FT_INNER_DEGENERATERING] = CreateFootprint(2 * m);
}

void KClipmap::InitializeFootprintPos()
{
	int32_t m = GetBlockCount() - 1;

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

	// Inner degenerated ring (order: top-left, top-right, bottom-left, bottom-right)
	// m_FootprintPos.push_back(KClipmapFootprintPos(m, m + 1, m_Footprints[FT_INNER_DEGENERATERING]));
	// m_FootprintPos.push_back(KClipmapFootprintPos(m + 1, m + 1, m_Footprints[FT_INNER_DEGENERATERING]));
	// m_FootprintPos.push_back(KClipmapFootprintPos(m, m, m_Footprints[FT_INNER_DEGENERATERING]));
	// m_FootprintPos.push_back(KClipmapFootprintPos(m + 1, m, m_Footprints[FT_INNER_DEGENERATERING]));
}

void KClipmap::InitializeClipmapLevel()
{
	m_ClipLevels.resize(m_LevelCount);
	for (int32_t i = 0; i < m_LevelCount; ++i)
	{
		m_ClipLevels[i] = KClipmapLevelPtr(KNEW KClipmapLevel(this, i));
		m_ClipLevels[i]->Init();
		m_ClipLevels[i]->PopulateUpdateRects();
		m_ClipLevels[i]->UpdateClipStartScale();
		m_ClipLevels[i]->UpdateHeightData();
	}
}

void KClipmap::InitializePipeline()
{
	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "terrain/clipmap.vert", m_VSShader, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "terrain/clipmap.frag", m_FSShader, false);

	KRenderGlobal::RenderDevice->CreateSampler(m_Sampler);

	m_Sampler->SetAddressMode(AM_REPEAT, AM_REPEAT, AM_REPEAT);
	m_Sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_Sampler->Init(0, 0);

	KRenderGlobal::RenderDevice->CreateSampler(m_MipmapSampler);
	m_MipmapSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_MipmapSampler->Init(0, 0);

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
		pipeline->SetShader(ST_VERTEX, *m_VSShader);
		pipeline->SetShader(ST_FRAGMENT, *m_FSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		IKRenderTargetPtr textureTarget = clipmapLevel->GetTextureTarget();
		IKTexturePtr texture = clipmapLevel->GetDebugTexture();

		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, textureTarget->GetFrameBuffer(), m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, texture ? texture->GetFrameBuffer() : textureTarget->GetFrameBuffer(), m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_DiffuseTexture->GetFrameBuffer(), m_MipmapSampler, true);

		pipeline->Init();
	}
}

void KClipmap::Init(const glm::vec3& center, float size, float height, int32_t gridLevel, int32_t divideLevel)
{
	UnInit();

	KClipmapLevel::InitShared();

	m_GridWorldCenter = center;
	m_ClipWorldCenter = glm::vec3(0);

	m_GridCount = (1 << gridLevel) - 1;
	m_LevelCount = divideLevel;
	m_FinestLevel = m_LevelCount - 1;

	m_Size = size;
	m_HeightScale = height;

	KRenderGlobal::RenderDevice->CreateTexture(m_DiffuseTexture);

	InitializeFootprint();
	InitializeFootprintPos();
	InitializeClipmapLevel();
	InitializePipeline();
}

void KClipmap::UnInit()
{
	for (int32_t i = 0; i < FT_COUNT; ++i)
	{
		SAFE_UNINIT(m_Footprints[i]);
	}
	m_FootprintPos.clear();

	m_VSShader.Release();
	m_FSShader.Release();

	SAFE_UNINIT_CONTAINER(m_ClipLevels);
	SAFE_UNINIT_CONTAINER(m_ClipLevelPipelines);

	SAFE_UNINIT(m_DiffuseTexture);
	SAFE_UNINIT(m_Sampler);
	SAFE_UNINIT(m_MipmapSampler);

	KClipmapLevel::UnInitShared();
	m_DebugDrawer.UnInit();

	m_Updated = false;
}

void KClipmap::LoadHeightMap(const std::string& file)
{
	m_HeightMap.Init(file.c_str());
	m_GridCenterX = (m_HeightMap.GetWidth() - 1) / 2;
	m_GridCenterY = (m_HeightMap.GetHeight() - 1) / 2;
	m_BaseGridSize.x = m_Size / (m_HeightMap.GetWidth() - 1);
	m_BaseGridSize.y = m_Size / (m_HeightMap.GetHeight() - 1);
}

void KClipmap::LoadDiffuse(const std::string& file)
{
	m_DiffuseTexture->UnInit();
	if (m_DiffuseTexture->InitMemoryFromFile(file, true, false))
	{
		m_DiffuseTexture->InitDevice(false);
		m_MipmapSampler->Init(0, m_DiffuseTexture->GetMipmaps());
	}
}

void KClipmap::Update(const KCamera* camera)
{
	const glm::vec3& cameraPos = camera->GetPosition();
	const glm::vec3 biasFromCenter = cameraPos - m_GridWorldCenter;
	const int32_t gridScale = 1 << (m_LevelCount - 1);
	const glm::vec2 baseLevelGridSize = m_BaseGridSize * (float)gridScale;

	const int32_t biasXFromCenter = (int32_t)floor(biasFromCenter.x / baseLevelGridSize.x);
	const int32_t biasYFromCenter = (int32_t)floor(biasFromCenter.z / baseLevelGridSize.y);

	const int32_t newClipCenterX = m_GridCenterX + biasXFromCenter * gridScale;
	const int32_t newClipCenterY = m_GridCenterY + biasYFromCenter * gridScale;

	m_FinestLevel = 0;

	const float cameraHeight = cameraPos.y;
	const float terrainHeight = m_HeightMap.GetData(newClipCenterX, newClipCenterY) * m_HeightScale;
	const float height = abs(cameraHeight - terrainHeight);
	const float heightScaleFactor = m_HeightScale / std::min(m_BaseGridSize.x, m_BaseGridSize.y);
	for (int32_t i = m_LevelCount - 1; i >= 0; --i)
	{
		float size = 0.4f * (m_GridCount - 1) * heightScaleFactor;
		if (height <= size)
		{
			m_FinestLevel = i;
			break;
		}
	}

	if (m_Updated && newClipCenterX == m_ClipCenterX && newClipCenterY == m_ClipCenterY)
		return;

	m_ClipCenterX = newClipCenterX;
	m_ClipCenterY = newClipCenterY;

	m_ClipWorldCenter.x = m_GridWorldCenter.x + biasXFromCenter * baseLevelGridSize.x;
	m_ClipWorldCenter.z = m_GridWorldCenter.z + biasYFromCenter * baseLevelGridSize.y;

	int32_t clipLevelX = m_ClipCenterX;
	int32_t clipLevelY = m_ClipCenterY;
	KClipmapLevel::TrimLocation trim = KClipmapLevel::TL_NONE;

	int32_t m = GetBlockCount();

	for (int32_t level = m_LevelCount - 1; level >= 0; --level)
	{
		KClipmapLevelPtr clipLevel = m_ClipLevels[level];
		int32_t clipGridSize = clipLevel->GetGridSize();

		if (level == m_LevelCount - 1)
		{
			assert(clipGridSize == 1);
			clipLevelX -= m_GridCount / 2;
			clipLevelY -= m_GridCount / 2;
			trim = KClipmapLevel::TL_NONE;
		}
		else
		{
			int32_t x = m_ClipCenterX / clipGridSize;
			int32_t y = m_ClipCenterY / clipGridSize;

			clipLevelX -= (m - 1) * clipGridSize;
			clipLevelY -= (m - 1) * clipGridSize;

			trim = KClipmapLevel::TL_TOP_RIGHT;
			/*
			if (x & 4)
			{
				clipLevelX -= clipGridSize;
				if (y & 4)
				{
					clipLevelY -= clipGridSize;
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
					clipLevelY -= clipGridSize;
					trim = KClipmapLevel::TL_BOTTOM_RIGHT;
				}
				else
				{
					trim = KClipmapLevel::TL_TOP_RIGHT;
				}
			}
			*/
		}

		// KLog::Logger->Log(LL_DEBUG, "Clipmap update level:[%d] x:[%d] y:[%d]", level, clipLevelX, clipLevelY);

		// 设置位置
		if (!m_Updated)
		{
			clipLevel->SetPosition(clipLevelX, clipLevelY, trim);
		}
		// 卷动位置
		else
		{
			clipLevel->ScrollPosition(clipLevelX, clipLevelY, trim);
		}

		clipLevel->PopulateUpdateRects();
		clipLevel->UpdateClipStartScale();
		clipLevel->UpdateHeightData();
	}

	KRenderGlobal::RenderDevice->Wait();
	for (int32_t level = 0; level < m_LevelCount; ++level)
	{
		m_ClipLevels[level]->UpdateTexture();
	}

	for (int32_t level = 0; level < m_LevelCount; ++level)
	{
		// m_ClipLevels[level]->CheckHeightValid();
	}

	m_Updated = true;
}

void KClipmap::RenderInternal(IKCommandBufferPtr primaryBuffer, IKRenderPassPtr renderPass, int32_t levelIdx, bool hollowCenter)
{
	KClipmapLevelPtr clipLevel = m_ClipLevels[levelIdx];

	glm::vec4 clipStartScale = clipLevel->GetClipStartScale();

	for (size_t footprintIdx = 0; footprintIdx < m_FootprintPos.size(); ++footprintIdx)
	{
		// [block]		0  - 11
		// [inner]		12 - 15
		//				top      left	     right       bottom
		// [fixup]		16       17          18          19
		// [trim]		20       21          22          23
		// [outer_ring] 24
		//				top-left top-right   bottom-left bottom-right
		// [inner_ring] 25       26          27          28

		if (!hollowCenter)
		{
			if (footprintIdx >= 25)
				continue;
		}
		else
		{
			if (footprintIdx >= 12 && footprintIdx <= 15)
				continue;

			if (levelIdx != m_LevelCount - 1)
			{
				KClipmapLevel::TrimLocation trim = clipLevel->GetTrimLocation();

				if (footprintIdx >= 20 && footprintIdx <= 23)
				{
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

				if (footprintIdx >= 25 && footprintIdx <= 28)
				{
					if (trim == KClipmapLevel::TL_BOTTOM_LEFT)
					{
						if (footprintIdx != 26)
							continue;
					}
					else if (trim == KClipmapLevel::TL_BOTTOM_RIGHT)
					{
						if (footprintIdx != 25)
							continue;
					}
					else if (trim == KClipmapLevel::TL_TOP_LEFT)
					{
						if (footprintIdx != 28)
							continue;
					}
					else if (trim == KClipmapLevel::TL_TOP_RIGHT)
					{
						if (footprintIdx != 27)
							continue;
					}
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
			glm::vec4 clipStartScale;
			glm::vec4 misc;
			glm::vec4 misc2;
			glm::vec4 misc3;
			glm::vec4 misc4;
		} objectData;

		objectData.clipStartScale = clipStartScale;
		objectData.misc.x = (float)footprintPos.GetPosX();
		objectData.misc.y = (float)footprintPos.GetPosY();
		objectData.misc.z = (float)m_GridCount;
		objectData.misc.w = m_HeightScale;

		objectData.misc2.x = (float)clipLevel->GetScrollX() / (m_GridCount);
		objectData.misc2.y = (float)clipLevel->GetScrollY() / (m_GridCount);
		objectData.misc2.z = (float)levelIdx;
		objectData.misc2.w = (float)m_LevelCount;

		objectData.misc3.x = (float)footprintIdx;
		objectData.misc3.y = (float)m_FootprintPos.size();
		if (footprintIdx == 24)
		{
			objectData.misc3.z = 2.0f;
		}
		else if (footprintIdx >= 25)
		{
			objectData.misc3.z = 1.0f;
		}
		else
		{
			objectData.misc3.z = 0.0f;
		}

		objectData.misc3.w = (float)clipLevel->GetGridSize();;
		objectData.misc4.x = (float)clipLevel->GetRealBottomLeftX();
		objectData.misc4.y = (float)clipLevel->GetRealBottomLeftY();
		objectData.misc4.z = (float)m_HeightMap.GetWidth();
		objectData.misc4.w = (float)m_HeightMap.GetHeight();

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = true;

		primaryBuffer->Render(command);
	}
}

bool KClipmap::Render(IKCommandBufferPtr primaryBuffer, IKRenderPassPtr renderPass)
{
	for (int32_t levelIdx = 0; levelIdx <= m_FinestLevel; ++levelIdx)
	{
		RenderInternal(primaryBuffer, renderPass, levelIdx, levelIdx != m_FinestLevel);
	}
	return true;
}

bool KClipmap::Reload()
{
	if (m_VSShader)
		(*m_VSShader)->Reload();
	if (m_FSShader)
		(*m_FSShader)->Reload();
	for (IKPipelinePtr& pipeline : m_ClipLevelPipelines)
	{
		pipeline->Reload();
	}
	return true;
}

KClipmapLevelPtr KClipmap::GetClipmapLevel(int32_t idx)
{
	if ((uint32_t)idx < m_ClipLevels.size())
		return m_ClipLevels[idx];
	return nullptr;
}

bool KClipmap::EnableDebugDraw(const KTerrainDebug& debug)
{
	m_DebugDrawer.UnInit();
	m_DebugDrawer.Init(m_ClipLevels[debug.clipmap.debugLevel]->GetTextureTarget()->GetFrameBuffer(), 0, 0, 0.5f, 0.5f);
	m_DebugDrawer.EnableDraw();
	return true;
}

bool KClipmap::DisableDebugDraw()
{
	m_DebugDrawer.DisableDraw();
	return true;
}

bool KClipmap::DebugRender(IKCommandBufferPtr primaryBuffer, IKRenderPassPtr renderPass)
{
	m_DebugDrawer.Render(renderPass, primaryBuffer);
	return true;
}