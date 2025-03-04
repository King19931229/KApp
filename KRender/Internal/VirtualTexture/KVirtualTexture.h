#pragma once
#include "Interface/IKTexture.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "Internal/Object/KDebugDrawer.h"
#include "Internal/Render/KRHICommandList.h"
#include <queue>

struct KVirtualTextureTile
{
	uint32_t x = -1;
	uint32_t y = -1;
	uint32_t mip = -1;

	bool IsValid() const
	{
		return x != -1 && y != -1 && mip != -1;
	}
	void Invalidate()
	{
		x = y = mip = -1;
	}
};

typedef KVirtualTextureTile KVirtualTexturePhysicalLocation;

inline bool operator<(const KVirtualTextureTile& lhs, const KVirtualTextureTile& rhs)
{
	if (lhs.mip != rhs.mip)
		return lhs.mip < rhs.mip;
	if (lhs.x != rhs.x)
		return lhs.x < rhs.x;
	return lhs.y < rhs.y;
}

inline bool operator==(const KVirtualTextureTile& lhs, const KVirtualTextureTile& rhs)
{
	if (lhs.x != rhs.x)
		return false;
	if (lhs.y != rhs.y)
		return false;
	if (lhs.mip != rhs.mip)
		return false;
	return true;
}

namespace std
{
	template<>
	struct hash<KVirtualTextureTile>
	{
		size_t operator()(const KVirtualTextureTile& location) const
		{
			assert(location.x < (1 << 14));
			assert(location.y < (1 << 14));
			assert(location.mip < (1 << 4));
			std::size_t hash = (location.y) | (location.x << 14) | (location.mip << 28);
			return hash;
		}
	};
}

struct KVirtualTextureTileLocation
{
	KVirtualTexturePhysicalLocation physicalLocation;
	uint32_t mip = 0;
};

struct KVirtualTexturePhysicalTile;

struct KVirtualTextureTileNode
{
	enum DataLoadStatus
	{
		TILE_UNLOADED,
		TILE_LOADING,
		TILE_LOADED
	};

	uint32_t sx = 0;
	uint32_t sy = 0;
	uint32_t ex = 0;
	uint32_t ey = 0;
	uint32_t mip = 0;
	uint32_t loadStatus = TILE_UNLOADED;

	KVirtualTextureTileNode* children[4] = { nullptr, nullptr, nullptr, nullptr };
	KVirtualTexturePhysicalTile* physicalTile = nullptr;

	KVirtualTextureTileNode(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint32_t mipLevel);
	~KVirtualTextureTileNode();

	KVirtualTextureTileNode* GetNodeWithDataLoaded(uint32_t x, uint32_t y, uint32_t mipLevel);
	KVirtualTextureTileNode* GetNode(uint32_t x, uint32_t y, uint32_t mipLevel);

	void LRUTouch(uint32_t x, uint32_t y, uint32_t mipLevel);

	void ReturnPhysicalTileRecursively();
};

struct KVirtualTextureTileRequest
{
	KVirtualTextureTile tile;
	uint32_t count = 0;
};

struct KVirtualTextureTableUpdate
{
	uint32_t id = -1;
	uint32_t data = -1;
	uint32_t padding[2] = { 0 };
};

enum VirtualTextureBinding
{
	#define VIRTUAL_TEXTURE_BINDING(SEMANTIC) VIRTUAL_TEXTURE_BINDING_##SEMANTIC,
	#include "KVirtualTextureBinding.inl"
	#undef VIRTUAL_TEXTURE_BINDING
};

class KVirtualTexture
{
protected:
	std::string m_Path;
	std::string m_Ext;
	
	KVirtualTextureTileNode* m_RootNode = nullptr;
	uint32_t m_TileNum = 0;
	uint32_t m_MaxMipLevel = 0;
	uint32_t m_MaxUpdatePerFrame = 0;

	IKTexturePtr m_TableTexture;
	std::vector<uint32_t> m_TableInfo;
	std::vector<KVirtualTextureTableUpdate> m_PendingTableUpdates;

	std::vector<IKComputePipelinePtr> m_MipUpdateComputePipelines;
	std::vector<IKStorageBufferPtr> m_MipUpdateStorages;

	std::vector<std::vector<IKStorageBufferPtr>> m_TableUpdateStorages;
	std::vector<std::vector<IKComputePipelinePtr>> m_TableUpdateComputePipelines;

	uint32_t m_VirtualID = 0;

	struct PendingTileUpdateCompare
	{
		bool operator()(const KVirtualTextureTileNode* lhs, const KVirtualTextureTileNode* rhs);
	};

	std::unordered_map<KVirtualTextureTile, uint32_t> m_HashedTileRequests;
	std::priority_queue<KVirtualTextureTileNode*, std::deque<KVirtualTextureTileNode*>, PendingTileUpdateCompare> m_PendingTileUpdates;

	KRTDebugDrawer m_TableDebugDrawer;
public:
	KVirtualTexture();
	~KVirtualTexture();

	bool Init(const std::string& path, uint32_t tileNum, uint32_t virtualId);
	bool UnInit();

	bool FeedbackRender(KRHICommandList& commandList, IKRenderPassPtr renderPass, uint32_t targetBinding, const std::vector<IKEntity*>& cullRes);
	bool UpdateTexture(KRHICommandList& commandList);

	void BeginRequest();
	void AddRequest(const KVirtualTextureTile& tile, uint32_t count);
	void EndRequest();
	void ProcessPendingUpdate();

	bool& GetPhysicalDrawEnable() { return m_TableDebugDrawer.GetEnable(); }
	bool TableDebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);

	IKTexturePtr GetTableTexture() { return m_TableTexture; }
	uint32_t GetVirtualID() const { return m_VirtualID; }

	uint32_t GetTileNum() const { return m_TileNum; }
	uint32_t GetMaxMipLevel() const { return m_MaxMipLevel; }
};

typedef KReferenceHolder<KVirtualTexture*> KVirtualTextureResourceRef;