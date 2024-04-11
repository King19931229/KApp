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
	, m_Width(0)
	, m_Height(0)
	, m_VirtualIDCounter(0)
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
	m_SizeWithPadding = (tileSize + 2 * paddingSize) * tileDimension;

	m_NumMips = (uint32_t)std::log2(m_TileDimension) + 1;

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

	uint32_t frameNum = KRenderGlobal::NumFramesInFlight;
	m_FeedbackTargets.resize(frameNum);
	m_FeedbackDepths.resize(frameNum);
	m_FeedbackPasses.resize(frameNum);
	m_ResultReadbackTargets.resize(frameNum);
	m_PendingSourceTextures.resize(frameNum);

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_PhysicalContentTarget);
	m_PhysicalContentTarget->InitFromColor(m_SizeWithPadding, m_SizeWithPadding, 1, m_NumMips, EF_R8G8B8A8_UNORM);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "virtualtexture/physical_upload.frag", m_UploadFS, false);

	KSamplerDescription desc;
	desc.minFilter = FM_NEAREST;
	desc.magFilter = FM_NEAREST;
	KRenderGlobal::SamplerManager.Acquire(desc, m_PhysicalUpdateSampler);

	desc.minFilter = FM_LINEAR;
	desc.magFilter = FM_LINEAR;
	desc.maxMipmap = m_NumMips - 1;
	KRenderGlobal::SamplerManager.Acquire(desc, m_PhysicalRenderSampler);

	KRenderGlobal::RenderDevice->CreatePipeline(m_UploadContentPipeline);
	m_UploadContentPipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	m_UploadContentPipeline->SetShader(ST_VERTEX, *m_QuadVS);
	m_UploadContentPipeline->SetShader(ST_FRAGMENT, *m_UploadFS);

	m_UploadContentPipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	m_UploadContentPipeline->SetBlendEnable(false);
	m_UploadContentPipeline->SetCullMode(CM_NONE);
	m_UploadContentPipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	m_UploadContentPipeline->SetPolygonMode(PM_FILL);
	m_UploadContentPipeline->SetColorWrite(true, true, true, true);
	m_UploadContentPipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

	m_UploadContentPipeline->SetSampler(SHADER_BINDING_TEXTURE0, nullptr, *m_PhysicalUpdateSampler, true);

	m_UploadContentPipeline->Init();

	m_PendingContentUpdate.resize(m_NumMips);
	m_UploadContentPasses.resize(m_NumMips);

	for (uint32_t idx = 0; idx < m_NumMips; ++idx)
	{
		KRenderGlobal::RenderDevice->CreateRenderPass(m_UploadContentPasses[idx]);
		m_UploadContentPasses[idx]->SetColorAttachment(0, m_PhysicalContentTarget->GetFrameBuffer());
		m_UploadContentPasses[idx]->SetOpColor(0, LO_LOAD, SO_STORE);
		m_UploadContentPasses[idx]->Init(idx);
	}

	Resize(1024, 1024);

	{
		IKCommandBufferPtr commandBuffer = KRenderGlobal::CommandPool->Request(CBL_PRIMARY);
		commandBuffer->BeginPrimary();

		for (uint32_t mip = 0; mip < m_NumMips; ++mip)
		{
			commandBuffer->BeginRenderPass(m_UploadContentPasses[mip], SUBPASS_CONTENTS_INLINE);
			commandBuffer->SetViewport(m_UploadContentPasses[mip]->GetViewPort());
			commandBuffer->EndRenderPass();
		}

		commandBuffer->Transition(m_PhysicalContentTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		commandBuffer->End();
		commandBuffer->Flush();
	}

	m_FeedbackDebugDrawer.Init(m_FeedbackTargets[0]->GetFrameBuffer(), 0, 0, 1, 1, false);
	m_PhysicalDebugDrawer.Init(m_PhysicalContentTarget->GetFrameBuffer(), 0, 0, 0.5, 0.5, false);

	return true;
}

bool KVirtualTextureManager::UnInit()
{
	m_UsedTiles.clear();
	m_PhysicalTiles.clear();
	m_TextureMap.clear();
	m_PendingContentUpdate.clear();
	m_PendingSourceTextures.clear();

	m_PhysicalUpdateSampler.Release();
	m_PhysicalRenderSampler.Release();
	m_QuadVS.Release();
	m_UploadFS.Release();

	m_FreeTileHead = nullptr;
	m_UsedTileHead = nullptr;

	SAFE_UNINIT(m_UploadContentPipeline);

	SAFE_UNINIT(m_PhysicalContentTarget);

	SAFE_UNINIT_CONTAINER(m_UploadContentPasses);

	SAFE_UNINIT_CONTAINER(m_FeedbackTargets);
	SAFE_UNINIT_CONTAINER(m_FeedbackDepths);
	SAFE_UNINIT_CONTAINER(m_FeedbackPasses);
	SAFE_UNINIT_CONTAINER(m_ResultReadbackTargets);

	m_FeedbackDebugDrawer.UnInit();
	m_PhysicalDebugDrawer.UnInit();

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

void KVirtualTextureManager::HandleFeedbackResult()
{
	uint32_t frameIdx = KRenderGlobal::CurrentInFlightFrameIndex;

	std::vector<KVirtualTextureResourceRef> virtualIdMap;
	uint32_t maxID = 0;

	for (auto& pair : m_TextureMap)
	{
		KVirtualTextureResourceRef resource = pair.second;
		resource->BeginRequest();
		maxID = std::max(maxID, resource->GetVirtualID());
	}

	virtualIdMap.resize(maxID + 1);
	for (auto& pair : m_TextureMap)
	{
		virtualIdMap[pair.second->GetVirtualID()] = pair.second;
	}

	{
		IKFrameBufferPtr src = m_FeedbackTargets[frameIdx]->GetFrameBuffer();
		IKFrameBufferPtr dest = m_ResultReadbackTargets[frameIdx]->GetFrameBuffer();
		src->CopyToReadback(dest.get());

		std::vector<uint32_t> readback;
		readback.resize(src->GetWidth() * src->GetHeight());
		dest->Readback(readback.data(), readback.size() * sizeof(uint32_t));

		for (size_t idx = 0; idx < readback.size(); ++idx)
		{
			uint32_t data = readback[idx];
			if (data != 0XFFFFFFFF)
			{
				uint32_t pageX = (data >> 0) & 0xFF;
				uint32_t pageY = (data >> 8) & 0xFF;
				uint32_t mipLevel = (data >> 16) & 0xFF;
				uint32_t virtualID = (data >> 24) & 0xFF;

				if (virtualID < virtualIdMap.size())
				{
					KVirtualTextureTile tile;
					tile.x = pageX;
					tile.y = pageY;
					tile.mip = mipLevel;
					virtualIdMap[virtualID]->AddRequest(tile);
				}
			}
		}
	}

	for (auto& pair : m_TextureMap)
	{
		KVirtualTextureResourceRef resource = pair.second;
		resource->EndRequest();
	}
}

bool KVirtualTextureManager::Update(IKCommandBufferPtr primaryBuffer, const std::vector<IKEntity*>& cullRes)
{
	LRUSortTile();

	HandleFeedbackResult();

	uint32_t frameIdx = KRenderGlobal::CurrentInFlightFrameIndex;

	primaryBuffer->Transition(m_FeedbackTargets[frameIdx]->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	primaryBuffer->Transition(m_PhysicalContentTarget->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);

	if (m_FeedbackPasses.size())
	{
		primaryBuffer->BeginDebugMarker("VirtualTexture_FeedbackRender", glm::vec4(1));
		primaryBuffer->BeginRenderPass(m_FeedbackPasses[frameIdx], SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_FeedbackPasses[frameIdx]->GetViewPort());

		for (auto& pair : m_TextureMap)
		{
			KVirtualTextureResourceRef resource = pair.second;
			resource->FeedbackRender(primaryBuffer, m_FeedbackPasses[frameIdx], m_CurrentTargetBinding, cullRes);
		}

		primaryBuffer->EndRenderPass();
		primaryBuffer->EndDebugMarker();
	}

	m_PendingSourceTextures[frameIdx].clear();

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.indexDraw = true;

	for (uint32_t mip = 0; mip < m_NumMips; ++mip)
	{
		if (m_PendingContentUpdate[mip].size())
		{
			std::vector<KVirtualTexturePhysicalUpdate> updates = std::move(m_PendingContentUpdate[mip]);

			primaryBuffer->BeginDebugMarker("VirtualTexture_PhysicalContentUpdate_" + std::to_string(mip), glm::vec4(1));
			primaryBuffer->BeginRenderPass(m_UploadContentPasses[mip], SUBPASS_CONTENTS_INLINE);
			primaryBuffer->SetViewport(m_UploadContentPasses[mip]->GetViewPort());

			for (const KVirtualTexturePhysicalUpdate& update : updates)
			{
				KTextureRef texture;
				if (KRenderGlobal::TextureManager.Acquire(update.sourceTexture.c_str(), texture, false))
				{
					m_PendingSourceTextures[frameIdx].push_back(texture);

					KViewPortArea viewport;
					viewport.x = (1 + 2 * update.location.x) * m_PaddingSize + update.location.x * m_TileSize;
					viewport.y = (1 + 2 * update.location.y) * m_PaddingSize + update.location.y * m_TileSize;
					viewport.width = m_TileSize + 2 * m_PaddingSize;
					viewport.height = m_TileSize + 2 * m_PaddingSize;

					m_UploadContentPipeline->SetSampler(SHADER_BINDING_TEXTURE0, texture->GetFrameBuffer(), *m_PhysicalUpdateSampler, true);

					command.pipeline = m_UploadContentPipeline;
					command.pipeline->GetHandle(m_UploadContentPasses[mip], command.pipelineHandle);

					primaryBuffer->SetViewport(viewport);
					primaryBuffer->Render(command);
				}
			}

			primaryBuffer->EndRenderPass();
			primaryBuffer->EndDebugMarker();
		}
	}

	// m_CurrentTargetBinding = (m_CurrentTargetBinding + 1) / MAX_MATERIAL_TEXTURE_BINDING;

	primaryBuffer->Transition(m_FeedbackTargets[frameIdx]->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	primaryBuffer->Transition(m_PhysicalContentTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	{
		for (auto& pair : m_TextureMap)
		{
			KVirtualTextureResourceRef resource = pair.second;
			resource->UpdateTableTexture(primaryBuffer);
		}
	}

	return true;
}

void KVirtualTextureManager::Resize(uint32_t width, uint32_t height)
{
	if (!width || !height)
	{
		return;
	}

	m_Width = width;
	m_Height = height;

	for (size_t i = 0; i < m_FeedbackPasses.size(); ++i)
	{
		if (!m_FeedbackTargets[i])
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_FeedbackTargets[i]);
		}
		if (!m_ResultReadbackTargets[i])
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_ResultReadbackTargets[i]);
		}
		if (!m_FeedbackDepths[i])
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_FeedbackDepths[i]);
		}
		if (!m_FeedbackPasses[i])
		{
			KRenderGlobal::RenderDevice->CreateRenderPass(m_FeedbackPasses[i]);
		}

		m_FeedbackTargets[i]->UnInit();
		m_ResultReadbackTargets[i]->UnInit();
		m_FeedbackDepths[i]->UnInit();
		m_FeedbackPasses[i]->UnInit();

		uint32_t scaleRatio = 8;
		uint32_t width = m_Width / scaleRatio;
		uint32_t height = m_Height / scaleRatio;

		m_ResultReadbackTargets[i]->InitFromReadback(width, height, 1, 1, EF_R8G8B8A8_UNORM);

		m_FeedbackTargets[i]->InitFromColor(width, height, 1, 1, EF_R8G8B8A8_UNORM);
		m_FeedbackDepths[i]->InitFromDepthStencil(width, height, 1, false);

		m_FeedbackPasses[i]->SetColorAttachment(0, m_FeedbackTargets[i]->GetFrameBuffer());
		m_FeedbackPasses[i]->SetClearColor(0, { 1,1,1,1 });
		m_FeedbackPasses[i]->SetDepthStencilAttachment(m_FeedbackDepths[i]->GetFrameBuffer());
		m_FeedbackPasses[i]->Init();
	}

	IKCommandBufferPtr commandBuffer = KRenderGlobal::CommandPool->Request(CBL_PRIMARY);
	commandBuffer->BeginPrimary();

	for (size_t i = 0; i < m_FeedbackPasses.size(); ++i)
	{
		commandBuffer->BeginRenderPass(m_FeedbackPasses[i], SUBPASS_CONTENTS_INLINE);
		commandBuffer->SetViewport(m_FeedbackPasses[i]->GetViewPort());
		commandBuffer->EndRenderPass();
		commandBuffer->Transition(m_FeedbackTargets[i]->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	commandBuffer->End();
	commandBuffer->Flush();
}

bool KVirtualTextureManager::ReloadShader()
{
	if (m_QuadVS)
	{
		m_QuadVS->Reload();
	}
	if (m_UploadFS)
	{
		m_UploadFS->Reload();
	}
	if (m_UploadContentPipeline)
	{
		m_UploadContentPipeline->Reload();
	}
	return true;
}

uint32_t KVirtualTextureManager::AcquireVirtualID()
{
	uint32_t ID = 0;
	if (!m_RecyledVirtualIDs.empty())
	{
		ID = m_RecyledVirtualIDs.front();
		m_RecyledVirtualIDs.pop();
	}
	else
	{
		ID = m_VirtualIDCounter++;
	}
	return ID;
}

void KVirtualTextureManager::RecyleVirtualID(uint32_t ID)
{
	m_RecyledVirtualIDs.push(ID);
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
		if (virutalTexture->Init(path, tileNum, AcquireVirtualID()))
		{
			ref = KVirtualTextureResourceRef(virutalTexture, [](KVirtualTexture* texture) { texture->UnInit(); });
			m_TextureMap.insert({ textureInfo, ref });
		}
		else
		{
			RecyleVirtualID(virutalTexture->GetVirtualID());
			SAFE_DELETE(virutalTexture);
		}
	}

	return ref.Get() != nullptr;
}

KVirtualTexturePhysicalTile* KVirtualTextureManager::RequestPhysical()
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

	return tile;
}

bool KVirtualTextureManager::ReturnPhysical(KVirtualTexturePhysicalTile* tile)
{
	if (m_UsedTiles.find(tile) != m_UsedTiles.end())
	{
		RemoveTileFromList(tile, m_UsedTileHead);
		m_UsedTiles.erase(tile);
		AddTileToList(tile, m_FreeTileHead);
		tile->refCount = 0;
		return true;
	}
	return false;
}

void KVirtualTextureManager::UploadToPhysical(const std::string& sourceTexture, KVirtualTexturePhysicalLocation location)
{
	assert(location.mip < m_NumMips);
	KVirtualTexturePhysicalUpdate newUpdate;
	newUpdate.sourceTexture = sourceTexture;
	newUpdate.location = location;
	m_PendingContentUpdate[location.mip].push_back(newUpdate);
}

bool KVirtualTextureManager::FeedbackDebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	return m_FeedbackDebugDrawer.Render(renderPass, primaryBuffer);
}

bool KVirtualTextureManager::PhysicalDebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	return m_PhysicalDebugDrawer.Render(renderPass, primaryBuffer);
}

bool KVirtualTextureManager::TableDebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	for (auto& pair : m_TextureMap)
	{
		KVirtualTextureResourceRef resource = pair.second;
		resource->TableDebugRender(renderPass, primaryBuffer);
	}
	return true;
}

IKFrameBufferPtr KVirtualTextureManager::GetPhysicalTextureFramebuffer(uint32_t index)
{
	return m_PhysicalContentTarget->GetFrameBuffer();
}

KSamplerRef KVirtualTextureManager::GetPhysicalTextureSampler(uint32_t index)
{
	return m_PhysicalRenderSampler;
}