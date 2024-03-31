#pragma once
#include "Interface/IKTexture.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "Internal/Object/KDebugDrawer.h"

struct KVirtualTexturePhysicalLocation
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

inline bool operator<(const KVirtualTexturePhysicalLocation& lhs, const KVirtualTexturePhysicalLocation& rhs)
{
	if (lhs.x != rhs.x)
		return lhs.x < rhs.x;
	if (lhs.y != rhs.y)
		return lhs.y < rhs.y;
	if (lhs.mip != rhs.mip)
		return lhs.mip < rhs.mip;
}

inline bool operator==(const KVirtualTexturePhysicalLocation& lhs, const KVirtualTexturePhysicalLocation& rhs)
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
	struct hash<KVirtualTexturePhysicalLocation>
	{
		size_t operator()(const KVirtualTexturePhysicalLocation& location) const
		{
			std::size_t hash = 0;
			KHash::HashCombine(hash, location.x);
			KHash::HashCombine(hash, location.y);
			KHash::HashCombine(hash, location.mip);
			return hash;
		}
	};
}

struct KVirtualTextureTileNode;
typedef std::shared_ptr<KVirtualTextureTileNode> KVirtualTextureTileNodePtr;

struct KVirtualTextureTileLocation
{
	KVirtualTexturePhysicalLocation physicalLocation;
	uint32_t mip = 0;
};

struct KVirtualTextureTileNode
{
	uint32_t sx = 0;
	uint32_t sy = 0;
	uint32_t ex = 0;
	uint32_t ey = 0;
	uint32_t mip = 0;
	KVirtualTextureTileNode* children[4] = { nullptr, nullptr, nullptr, nullptr };
	KVirtualTexturePhysicalLocation physicalLocation;

	KVirtualTextureTileNode(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint32_t mipLevel);
	~KVirtualTextureTileNode();
	KVirtualTextureTileLocation GetTile(uint32_t x, uint32_t y, uint32_t mipLevel);
	KVirtualTextureTileNode* GetNode(uint32_t x, uint32_t y, uint32_t mipLevel);
};

class KVirtualTexture
{
protected:
	std::string m_Path;
	
	KVirtualTextureTileNode* m_RootNode = nullptr;
	uint32_t m_TileNum = 0;

	IKTexturePtr m_TableTexture;
	std::vector<uint32_t> m_TableInfo;

	KRTDebugDrawer m_TableDebugDrawer;
public:
	KVirtualTexture();
	~KVirtualTexture();

	bool Init(const std::string& path, uint32_t tileNum);
	bool UnInit();

	bool FeedbackRender(IKCommandBufferPtr primaryBuffer, IKRenderPassPtr renderPass, const std::vector<IKEntity*>& cullRes);

	IKTexturePtr GetTableTexture() { return m_TableTexture; }
};

typedef KReferenceHolder<KVirtualTexture*> KVirtualTextureResourceRef;