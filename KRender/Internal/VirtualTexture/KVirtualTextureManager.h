#pragma once
#include "Interface/IKTexture.h"
#include "KVirtualTexture.h"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <queue>

struct KVirtualTexturePhysicalTilePayload
{
	KVirtualTextureTileNode* ownerNode = nullptr;
	uint32_t pixelCount = 0;
	uint32_t useFrameIndex = -1;
};

struct KVirtualTexturePhysicalTile
{
	KVirtualTexturePhysicalLocation physicalLocation;
	KVirtualTexturePhysicalTilePayload payload;
	KVirtualTexturePhysicalTile* prev = nullptr;
	KVirtualTexturePhysicalTile* next = nullptr;
};

struct KVirtualTexturePhysicalUpdate
{
	std::string sourceTexture;
	KVirtualTexturePhysicalLocation location;
};

class KVirtualTextureManager
{
protected:
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

	std::vector<IKRenderTargetPtr> m_FeedbackTargets;
	std::vector<IKRenderTargetPtr> m_FeedbackDepths;
	std::vector<IKRenderTargetPtr> m_ResultReadbackTargets;
	std::vector<IKRenderPassPtr> m_FeedbackPasses;

	IKStorageBufferPtr m_VirtualTextrueDescriptionBuffer;

	std::vector<IKStorageBufferPtr> m_FeedbackResultBuffers;
	std::vector<IKComputePipelinePtr> m_InitFeedbackBufferPipelines;
	std::vector<IKComputePipelinePtr> m_MergeFeedbackBufferPipelines;

	std::vector<std::vector<KTextureRef>> m_PendingSourceTextures;

	IKRenderTargetPtr m_PhysicalContentTarget;
	std::vector<IKRenderPassPtr> m_UploadContentPasses;
	std::vector<std::vector<KVirtualTexturePhysicalUpdate>> m_PendingContentUpdate;

	KShaderRef m_QuadVS;
	KShaderRef m_UploadFS;
	KSamplerRef m_PhysicalUpdateSampler;
	KSamplerRef	m_PhysicalRenderSampler;

	IKPipelinePtr m_UploadContentPipeline;

	KRTDebugDrawer m_FeedbackDebugDrawer;
	KRTDebugDrawer m_PhysicalDebugDrawer;

	uint32_t m_TileSize;
	uint32_t m_TileDimension;
	uint32_t m_PaddingSize;
	uint32_t m_SizeWithoutPadding;
	uint32_t m_SizeWithPadding;
	uint32_t m_NumMips;
	uint32_t m_TileNum;

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	uint32_t m_CurrentTargetBinding = 0;

	uint32_t m_VirtualIDCounter = 0;
	std::queue<uint32_t> m_RecyledVirtualIDs;

	void RemoveTileFromList(KVirtualTexturePhysicalTile* tile, KVirtualTexturePhysicalTile*& head);
	void AddTileToList(KVirtualTexturePhysicalTile* tile, KVirtualTexturePhysicalTile* &head);
	void LRUSortTile();
	void UpdateBuffer();

	uint32_t AcquireVirtualID();
	void RecyleVirtualID(uint32_t ID);

	void HandleFeedbackResult();
public:
	KVirtualTextureManager();
	~KVirtualTextureManager();

	bool Init(uint32_t tileSize, uint32_t tileNum, uint32_t paddingSize);
	bool UnInit();

	bool Acqiure(const std::string& path, uint32_t tileNum, KVirtualTextureResourceRef& ref);
	bool Update(IKCommandBufferPtr primaryBuffer, const std::vector<IKEntity*>& cullRes);

	bool ReloadShader();

	void Resize(uint32_t width, uint32_t height);

	KVirtualTexturePhysicalTile* RequestPhysical();
	bool ReturnPhysical(KVirtualTexturePhysicalTile* tile);

	void UploadToPhysical(const std::string& sourceTexture, KVirtualTexturePhysicalLocation location);

	bool& GetFeedbackDebugDrawEnable() { return m_FeedbackDebugDrawer.GetEnable(); }
	bool FeedbackDebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);

	bool& GetPhysicalDrawEnable() { return m_PhysicalDebugDrawer.GetEnable(); }
	bool PhysicalDebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);

	bool TableDebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);

	IKStorageBufferPtr GetVirtualTextrueDescriptionBuffer();
	IKFrameBufferPtr GetPhysicalTextureFramebuffer(uint32_t index);
	KSamplerRef GetPhysicalTextureSampler(uint32_t index);

	uint32_t GetTileSize() const { return m_TileSize; }
	uint32_t GetTileDimension() const { return m_TileDimension; }
	uint32_t GetSizeWithoutPadding() const { return m_SizeWithoutPadding; }
	uint32_t GetSizeWithPadding() const { return m_SizeWithPadding; }
	uint32_t GetNumMips() const { return m_NumMips; }
};