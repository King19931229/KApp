#include "KVirtualTexture.h"
#include "Internal/KRenderGlobal.h"

KVirtualTextureTileNode::KVirtualTextureTileNode(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint32_t mipLevel)
{
	sx = startX;
	sy = startY;
	ex = endX;
	ey = endY;
	mip = mipLevel;
}

KVirtualTextureTileNode::~KVirtualTextureTileNode()
{
	for (uint32_t i = 0; i < 4; ++i)
	{
		SAFE_DELETE(children[i]);
	}
}

KVirtualTexturePhysicalLocation KVirtualTextureTileNode::GetTile(uint32_t x, uint32_t y, uint32_t mipLevel)
{
	KVirtualTexturePhysicalLocation location;
	if (mipLevel < mip)
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			if (children[i])
			{
				location = children[i]->GetTile(x, y, mip);
				if (location.IsValid())
				{
					return location;
				}
			}
		}
	}
	if (x >= sx && x < ex && y >= sy && y < ey)
	{
		location = physicalLocation;
	}
	return location;
}

KVirtualTextureTileNode* KVirtualTextureTileNode::GetNode(uint32_t x, uint32_t y, uint32_t mipLevel)
{
	if (mipLevel < mip)
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			if (!children[i])
			{
				uint32_t sizeX = (ex - sx) / 2;
				uint32_t sizeY = (ey - sy) / 2;
				uint32_t startX = sx + sizeX * (uint32_t)((i & 1) > 0);
				uint32_t startY = sy + sizeY * (uint32_t)((i & 2) > 0);
				children[i] = new KVirtualTextureTileNode(startX, startY, startX + sizeX, startY + sizeY, mip - 1);
			}
			if (children[i]->GetNode(x, y, mipLevel))
			{
				return children[i];
			}
		}
		assert(false && "shuold not reach");
		return nullptr;
	}
	if (x >= sx && x < ex && y >= sy && y < ey)
	{
		return this;
	}
	return nullptr;
}

KVirtualTexture::KVirtualTexture()
	: m_RootNode(nullptr)
	, m_TileNum(0)
{}

KVirtualTexture::~KVirtualTexture()
{}

void KVirtualTexture::Resize(uint32_t width, uint32_t height)
{
	if (!m_FeedbackTarget)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_FeedbackTarget);
	}
	if (!m_FeedbackDepth)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_FeedbackDepth);
	}
	if (!m_FeedbackPass)
	{
		KRenderGlobal::RenderDevice->CreateRenderPass(m_FeedbackPass);
	}

	m_FeedbackTarget->UnInit();
	m_FeedbackDepth->UnInit();
	m_FeedbackPass->UnInit();

	m_FeedbackTarget->InitFromColor(width, height, 1, 1, EF_R8G8B8A8_UNORM);
	m_FeedbackDepth->InitFromDepthStencil(width, height, 1, false);

	m_FeedbackPass->SetColorAttachment(0, m_FeedbackTarget->GetFrameBuffer());
	m_FeedbackPass->SetDepthStencilAttachment(m_FeedbackDepth->GetFrameBuffer());
	m_FeedbackPass->Init();
}

bool KVirtualTexture::Init(const std::string& path, uint32_t tileNum)
{
	UnInit();

	m_Path = path;
	m_TileNum = tileNum;

	KRenderGlobal::RenderDevice->CreateTexture(m_TableTexture);
	m_TableTexture->InitMemoryFromData(nullptr, m_Path + "_PageTable", tileNum, tileNum, 1, IF_R8G8B8A8, false, false, false);
	m_TableTexture->InitDevice(false);

	m_TableInfo.resize(tileNum * tileNum);

	uint32_t tileSize = KRenderGlobal::VirtualTextureManager.GetTileSize();
	uint32_t mipLevel = (uint32_t)std::log2(tileNum);

	m_RootNode = new KVirtualTextureTileNode(0, 0, tileSize * tileNum, tileSize * tileNum, mipLevel);

	return true;
}

bool KVirtualTexture::UnInit()
{
	SAFE_DELETE(m_RootNode);
	SAFE_UNINIT(m_TableTexture);

	SAFE_UNINIT(m_FeedbackTarget);
	SAFE_UNINIT(m_FeedbackDepth);
	SAFE_UNINIT(m_FeedbackPass);

	m_TableInfo.clear();
	return true;
}