#include "KTerrain.h"

KTerrain::KTerrain()
	: m_Soul({ nullptr })
	, m_Type(TERRAIN_TYPE_CLIPMAP)
{	
}

KTerrain::~KTerrain()
{
	ASSERT_RESULT(!m_Soul.clipmap);
}

bool KTerrain::Create(const glm::vec3& center, float size, const KTerrainContext& context)
{
	Destroy();

	if (context.type == TERRAIN_TYPE_CLIPMAP)
	{
		m_Soul.clipmap = new KClipmap();
		m_Soul.clipmap->Init(center, size, context.clipmap.gridLevel, context.clipmap.divideLevel);
	}

	return true;
}

bool KTerrain::Destroy()
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			m_Soul.clipmap->UnInit();
			SAFE_DELETE(m_Soul.clipmap);
		}
	}
	return true;
}

bool KTerrain::Reload()
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			m_Soul.clipmap->Reload();
		}
	}
	return true;
}

void KTerrain::LoadHeightMap(const std::string& file)
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			m_Soul.clipmap->LoadHeightMap(file);
		}
	}
}

void KTerrain::Update(const KCamera* camera)
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			m_Soul.clipmap->Update(camera->GetPosition());
		}
	}
}

bool KTerrain::Render(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers)
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			return m_Soul.clipmap->Render(renderPass, buffers);
		}
	}
	return false;
}

bool KTerrain::EnableDebugDraw(const KTerrainDebug& debug)
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			m_Soul.clipmap->EnableDebugDraw(debug);
		}
		return true;
	}
	return false;
}

bool KTerrain::DisableDebugDraw()
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			m_Soul.clipmap->DisableDebugDraw();
		}
		return true;
	}
	return false;
}

bool KTerrain::GetDebugRenderCommand(KRenderCommandList& commands)
{
	if (m_Type == TERRAIN_TYPE_CLIPMAP)
	{
		if (m_Soul.clipmap)
		{
			m_Soul.clipmap->GetDebugRenderCommand(commands);
		}
		return true;
	}
	return false;
}