#include "KVirtualTextureManager.h"
#include "KBase/Publish/KMath.h"
#include "Internal/KRenderGlobal.h"

KVirtualTextureManager::KVirtualTextureManager()
	: m_FreeTileHead(nullptr)
	, m_UsedTileHead(nullptr)
	, m_TileSize(0)
	, m_TileDimension(0)
	, m_PaddingSize(0)
	, m_SizeWithoutPadding(0)
	, m_SizeWithPadding(0)
	, m_NumMips(0)
	, m_TileNum(0)
{
}

KVirtualTextureManager::~KVirtualTextureManager()
{
}

bool KVirtualTextureManager::Init(uint32_t tileSize, uint32_t tileDimension, uint32_t paddingSize)
{
	UnInit();

	if (!KMath::IsPowerOf2(tileSize))
	{
		tileSize = KMath::SmallestPowerOf2GreaterEqualThan(tileSize);
	}

	m_TileSize = tileSize;
	m_TileDimension = tileDimension;
	m_PaddingSize = paddingSize;

	m_SizeWithoutPadding = tileSize * tileDimension;
	m_SizeWithPadding = (tileSize + paddingSize) * tileDimension;

	m_NumMips = (uint32_t)std::log2(m_TileDimension) + 1;

	KRenderGlobal::RenderDevice->CreateTexture(m_PhysicalTexture);

	m_PhysicalTexture->InitMemoryFromData(nullptr, "VirtualTexturePhysical", m_SizeWithPadding, m_SizeWithPadding, 1, IF_R8G8B8A8, false, true, false);
	m_PhysicalTexture->InitDevice(false);

	m_TileNum = 0;
	for (uint32_t mip = 0; mip < m_NumMips; ++mip)
	{
		uint32_t num = m_TileDimension >> mip;
		m_TileNum += num * num;
	}
	m_PhysicalTiles.resize(m_TileNum);

	{
		uint32_t tileIndex = 0;
		for (uint32_t mip = 0; mip < m_NumMips; ++mip)
		{
			uint32_t num = m_TileDimension >> mip;
			for (uint32_t x = 0; x < num; ++x)
			{
				for (uint32_t y = 0; y < num; ++y)
				{
					m_PhysicalTiles[tileIndex].physicalLocation.x = x;
					m_PhysicalTiles[tileIndex].physicalLocation.y = y;
					m_PhysicalTiles[tileIndex].physicalLocation.mip = mip;
					m_LocationToTile[m_PhysicalTiles[tileIndex].physicalLocation] = &m_PhysicalTiles[tileIndex];
					tileIndex++;
				}
			}
		}
	}

	for (uint32_t i = 0; i < m_TileNum; ++i)
	{
		m_PhysicalTiles[i].prev = &m_PhysicalTiles[(i + m_TileNum - 1) % m_TileNum];
		m_PhysicalTiles[i].next = &m_PhysicalTiles[(i + 1) % m_TileNum];
		m_PhysicalTiles[i].refCount = 0;
	}
	m_UsedTileHead = nullptr;
	m_FreeTileHead = &m_PhysicalTiles[0];

	return true;
}

bool KVirtualTextureManager::UnInit()
{
	m_UsedTiles.clear();
	m_LocationToTile.clear();
	m_PhysicalTiles.clear();
	m_TextureMap.clear();

	m_FreeTileHead = nullptr;
	m_UsedTileHead = nullptr;

	SAFE_UNINIT(m_PhysicalTexture);
	return true;
}

void KVirtualTextureManager::RemoveTileFromList(KVirtualTexturePhysicalTile* tile, KVirtualTexturePhysicalTile*& head)
{
	assert(head);
	if (tile == head)
	{
		if (tile->next == tile)
		{
			head = nullptr;
		}
		else
		{
			head = tile->next;
		}
	}
	tile->prev->next = tile->next;
	tile->next->prev = tile->prev;
}

void KVirtualTextureManager::AddTileToList(KVirtualTexturePhysicalTile* tile, KVirtualTexturePhysicalTile* &head)
{
	if (head)
	{
		tile->next = head;
		tile->prev = head->prev;

		head->prev->next = tile;
		head->prev = tile;
	}
	else
	{
		tile->prev = tile->next = tile;
	}
	head = tile;
}

void KVirtualTextureManager::LRUSortTile()
{
	std::vector<KVirtualTexturePhysicalTile*> tiles(m_UsedTiles.begin(), m_UsedTiles.end());

	std::sort(tiles.begin(), tiles.end(), [](KVirtualTexturePhysicalTile* lhs, KVirtualTexturePhysicalTile* rhs)
	{
		return lhs->refCount < rhs->refCount;
	});

	for (KVirtualTexturePhysicalTile* tile : tiles)
	{
		RemoveTileFromList(tile, m_UsedTileHead);
		if (tile->refCount > 0)
		{
			AddTileToList(tile, m_UsedTileHead);
		}
		else
		{
			AddTileToList(tile, m_FreeTileHead);
		}
	}
}

bool KVirtualTextureManager::Update()
{
	LRUSortTile();
	return true;
}

void KVirtualTextureManager::Resize(uint32_t width, uint32_t height)
{
	for (auto& pair : m_TextureMap)
	{
		pair.second->Resize(width, height);
	}
}

bool KVirtualTextureManager::Acqiure(const std::string& path, uint32_t tileNum, KVirtualTextureResourceRef& ref)
{
	TextureInfo textureInfo;
	textureInfo.path = path;

	auto it = m_TextureMap.find(textureInfo);
	if (it != m_TextureMap.end())
	{
		ref = it->second;
	}
	else
	{
		KVirtualTexture* virutalTexture = KNEW KVirtualTexture();
		if (virutalTexture->Init(path, tileNum))
		{
			ref = KVirtualTextureResourceRef(virutalTexture, [](KVirtualTexture* texture) { texture->UnInit(); });
		}
		else
		{
			ref.Release();
			SAFE_DELETE(virutalTexture);
		}
	};

	return ref.Get() != nullptr;
}

KVirtualTexturePhysicalLocation KVirtualTextureManager::RequestPhysical()
{
	KVirtualTexturePhysicalTile* tile = nullptr;

	if (m_FreeTileHead)
	{
		tile = m_FreeTileHead;
		RemoveTileFromList(tile, m_FreeTileHead);
		m_UsedTiles.insert(tile);
		AddTileToList(tile, m_UsedTileHead);
	}
	else
	{
		tile = m_UsedTileHead->prev;
		while (tile != m_UsedTileHead && tile->refCount != 0)
		{
			tile = tile->prev;
		}
	}

	tile->refCount = 1;

	return tile->physicalLocation;
}

bool KVirtualTextureManager::ReturnPhysical(KVirtualTexturePhysicalLocation location)
{
	auto it = m_LocationToTile.find(location);
	if (it != m_LocationToTile.end())
	{
		KVirtualTexturePhysicalTile* tile = it->second;
		if (m_UsedTiles.find(tile) != m_UsedTiles.end())
		{
			RemoveTileFromList(tile, m_UsedTileHead);
			m_UsedTiles.erase(tile);
			AddTileToList(tile, m_FreeTileHead);
			tile->refCount = 0;
			return true;
		}
	}
	return false;
}