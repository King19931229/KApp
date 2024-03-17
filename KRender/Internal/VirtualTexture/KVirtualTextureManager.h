#pragma once
#include "Interface/IKTexture.h"
#include "KVirtualTexture.h"
#include <unordered_map>
#include <unordered_set>
#include <map>

struct KVirtualTexturePhysicalTile
{
	KVirtualTexturePhysicalLocation physicalLocation;
	KVirtualTexturePhysicalTile* prev = nullptr;
	KVirtualTexturePhysicalTile* next = nullptr;
	uint32_t refCount;
};

class KVirtualTextureManager
{
protected:
	std::unordered_map<KVirtualTexturePhysicalLocation, KVirtualTexturePhysicalTile*> m_LocationToTile;
	std::unordered_set<KVirtualTexturePhysicalTile*> m_UsedTiles;
	std::vector<KVirtualTexturePhysicalTile> m_PhysicalTiles;

	struct TextureInfo
	{
		std::string path;
		bool operator<(const TextureInfo& rhs) const
		{
			return path < rhs.path;
		}
	};
	typedef std::map<TextureInfo, KVirtualTextureResourceRef> TextureMap;
	TextureMap m_TextureMap;

	KVirtualTexturePhysicalTile* m_FreeTileHead;
	KVirtualTexturePhysicalTile* m_UsedTileHead;
	IKTexturePtr m_PhysicalTexture;
	uint32_t m_TileSize;
	uint32_t m_TileDimension;
	uint32_t m_PaddingSize;
	uint32_t m_SizeWithoutPadding;
	uint32_t m_SizeWithPadding;
	uint32_t m_NumMips;
	uint32_t m_TileNum;

	void RemoveTileFromList(KVirtualTexturePhysicalTile* tile, KVirtualTexturePhysicalTile*& head);
	void AddTileToList(KVirtualTexturePhysicalTile* tile, KVirtualTexturePhysicalTile* &head);
	void LRUSortTile();
public:
	KVirtualTextureManager();
	~KVirtualTextureManager();

	bool Init(uint32_t tileSize, uint32_t tileNum, uint32_t paddingSize);
	bool UnInit();

	bool Acqiure(const std::string& path, uint32_t tileNum, KVirtualTextureResourceRef& ref);
	bool Update();

	void Resize(uint32_t width, uint32_t height);

	KVirtualTexturePhysicalLocation RequestPhysical();
	bool ReturnPhysical(KVirtualTexturePhysicalLocation location);

	uint32_t GetTileSize() const { return m_TileSize; }
	uint32_t GetTileDimension() const { return m_TileDimension; }
	uint32_t GetSizeWithoutPadding() const { return m_SizeWithoutPadding; }
	uint32_t GetSizeWithPadding() const { return m_SizeWithPadding; }
	uint32_t GetNumMips() const { return m_NumMips; }
};