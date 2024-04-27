#include "KVirtualTextureManager.h"
#include "KBase/Publish/KMath.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"

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
#define VIRTUAL_TEXTURE_BINDING_TO_STR(x) #x
#define VIRTUAL_TEXTURE_BINDING(SEMANTIC) m_CompileEnv.macros.push_back( {VIRTUAL_TEXTURE_BINDING_TO_STR(VIRTUAL_TEXTURE_BINDING_##SEMANTIC), std::to_string(VIRTUAL_TEXTURE_BINDING_##SEMANTIC) });
#include "KVirtualTextureBinding.inl"
#undef VIRTUAL_TEXTURE_BINDING
#undef VIRTUAL_TEXTURE_BINDING_TO_STR
	m_CompileEnv.macros.push_back({ "GROUP_SIZE", std::to_string(GROUP_SIZE) });
	m_CompileEnv.macros.push_back({ "SQRT_GROUP_SIZE", std::to_string(SQRT_GROUP_SIZE) });
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

	m_FeedbackResultBuffers.resize(frameNum);
	m_MergedFeedbackResultBuffers.resize(frameNum);
	m_InitFeedbackResultPipelines.resize(frameNum);
	m_CountFeedbackResultPipelines.resize(frameNum);
	m_MergeFeedbackResultPipelines.resize(frameNum);

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_VirtualTextrueDescriptionBuffer);
	m_VirtualTextrueDescriptionBuffer->SetDebugName("VirtualTextrueDescription");
	m_VirtualTextrueDescriptionBuffer->InitMemory(1, false);
	m_VirtualTextrueDescriptionBuffer->InitDevice(false, false);

	Resize(1024, 1024);

	for (uint32_t i = 0; i < frameNum; ++i)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_FeedbackResultBuffers[i]);
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MergedFeedbackResultBuffers[i]);
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitFeedbackResultPipelines[i]);
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_CountFeedbackResultPipelines[i]);
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_MergeFeedbackResultPipelines[i]);

		m_FeedbackResultBuffers[i]->SetDebugName(("FeedbackResultBuffer_" + std::to_string(i)).c_str());
		m_FeedbackResultBuffers[i]->InitMemory(1024, false);
		m_FeedbackResultBuffers[i]->InitDevice(false, false);

		m_MergedFeedbackResultBuffers[i]->SetDebugName(("MergedFeedbackResultBuffer_" + std::to_string(i)).c_str());
		m_MergedFeedbackResultBuffers[i]->InitMemory(1024, false);
		m_MergedFeedbackResultBuffers[i]->InitDevice(false, false);

		m_InitFeedbackResultPipelines[i]->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_RESULT, m_FeedbackResultBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InitFeedbackResultPipelines[i]->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_MERGED_FEEDBACK_RESULT, m_MergedFeedbackResultBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InitFeedbackResultPipelines[i]->BindDynamicUniformBuffer(VIRTUAL_TEXTURE_BINDING_OBJECT);
		m_InitFeedbackResultPipelines[i]->Init("virtualtexture/init_feedback.comp", m_CompileEnv);

		m_CountFeedbackResultPipelines[i]->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_RESULT, m_FeedbackResultBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_CountFeedbackResultPipelines[i]->BindStorageImage(VIRTUAL_TEXTURE_BINDING_FEEDBACK_INPUT, m_FeedbackTargets[i]->GetFrameBuffer(), EF_R8G8B8A8_UNORM, COMPUTE_RESOURCE_IN, 0, true);
		m_CountFeedbackResultPipelines[i]->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_TEXTURE_DESCRIPTION, KRenderGlobal::VirtualTextureManager.GetVirtualTextrueDescriptionBuffer(), COMPUTE_RESOURCE_IN, true);
		m_CountFeedbackResultPipelines[i]->BindUniformBuffer(VIRTUAL_TEXTURE_BINDING_CONSTANT, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VIRTUAL_TEXTURE_CONSTANT));
		m_CountFeedbackResultPipelines[i]->Init("virtualtexture/count_feedback.comp", m_CompileEnv);

		m_MergeFeedbackResultPipelines[i]->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_RESULT, m_FeedbackResultBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_MergeFeedbackResultPipelines[i]->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_MERGED_FEEDBACK_RESULT, m_MergedFeedbackResultBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_MergeFeedbackResultPipelines[i]->BindDynamicUniformBuffer(VIRTUAL_TEXTURE_BINDING_OBJECT);
		m_MergeFeedbackResultPipelines[i]->Init("virtualtexture/merge_feedback.comp", m_CompileEnv);
	}

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

	SAFE_UNINIT(m_VirtualTextrueDescriptionBuffer);

	SAFE_UNINIT_CONTAINER(m_FeedbackResultBuffers);
	SAFE_UNINIT_CONTAINER(m_MergedFeedbackResultBuffers);
	SAFE_UNINIT_CONTAINER(m_InitFeedbackResultPipelines);
	SAFE_UNINIT_CONTAINER(m_CountFeedbackResultPipelines);
	SAFE_UNINIT_CONTAINER(m_MergeFeedbackResultPipelines);

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
	std::vector<KVirtualTexturePhysicalTile*> tiles;
	std::vector<KVirtualTexturePhysicalTile*> loadingTiles;

	tiles.reserve(m_UsedTiles.size());
	loadingTiles.reserve(m_UsedTiles.size());

	for (KVirtualTexturePhysicalTile* tile : m_UsedTiles)
	{
		uint32_t loadStatus = tile->payload.ownerNode->loadStatus;
		assert(loadStatus != KVirtualTextureTileNode::TILE_UNLOADED);
		if (loadStatus == KVirtualTextureTileNode::TILE_LOADING)
		{
			loadingTiles.push_back(tile);
		}
		else
		{
			tiles.push_back(tile);
		}
	}

	std::sort(tiles.begin(), tiles.end(), [](KVirtualTexturePhysicalTile* lhs, KVirtualTexturePhysicalTile* rhs)
	{
		if (lhs->payload.useFrameIndex != rhs->payload.useFrameIndex)
			return lhs->payload.useFrameIndex < rhs->payload.useFrameIndex;
		return lhs->payload.ownerNode->mip > rhs->payload.ownerNode->mip;
	});

	tiles.insert(tiles.end(), loadingTiles.begin(), loadingTiles.end());

	for (KVirtualTexturePhysicalTile* tile : tiles)
	{
		RemoveTileFromList(tile, m_UsedTileHead);
		AddTileToList(tile, m_UsedTileHead);
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

void KVirtualTextureManager::UpdateBuffer()
{
	{
		IKUniformBufferPtr virtualTextureBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VIRTUAL_TEXTURE_CONSTANT);

		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_VIRTUAL_TEXTURE_CONSTANT);
		const KConstantDefinition::ConstantBufferDetail& details = KConstantDefinition::GetConstantBufferDetail(CBT_VIRTUAL_TEXTURE_CONSTANT);

		glm::uvec4 description;
		description.x = m_TileSize;
		description.y = m_PaddingSize;

		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			assert(detail.offset % detail.size == 0);
			void* pWritePos = nullptr;
			if (detail.semantic == CS_VIRTUAL_TEXTURE_DESCRIPTION)
			{
				assert(sizeof(description) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				memcpy(pWritePos, &description, sizeof(description));
			}
		}
		virtualTextureBuffer->Write(pData);
	}

	m_FeedbackTileCount = 0;

	{
		std::vector<glm::uvec4> descriptions;
		descriptions.resize(m_VirtualIDCounter);

		for (auto& pair : m_TextureMap)
		{
			KVirtualTextureResourceRef resource = pair.second;
			uint32_t id = resource->GetVirtualID();
			descriptions[id].x = resource->GetTileNum();
			descriptions[id].y = resource->GetMaxMipLevel();
			descriptions[id].z = m_FeedbackTileCount * sizeof(uint32_t);

			uint32_t tileNum = resource->GetTileNum();
			uint32_t tileCount = 0;
			uint32_t mipOffsetSum = 0;
			for (uint32_t i = 0; i <= resource->GetMaxMipLevel(); ++i)
			{
				uint32_t levelTileNum = resource->GetTileNum() >> i;
				tileCount += levelTileNum * levelTileNum;
			}

			uint32_t calcTileCount = (tileNum * tileNum * 4 - tileNum * tileNum / (1 << (2 * resource->GetMaxMipLevel()))) / 3;
			assert(calcTileCount == tileCount);
			m_FeedbackTileCount += tileCount;
		}

		IKStorageBufferPtr descriptionBuffer = m_VirtualTextrueDescriptionBuffer;

		if (descriptionBuffer->GetBufferSize() < m_VirtualIDCounter * sizeof(glm::uvec4))
		{
			KRenderGlobal::RenderDevice->Wait();
			uint32_t bufferSize = KMath::SmallestPowerOf2GreaterEqualThan(m_VirtualIDCounter) * sizeof(glm::uvec4);
			descriptionBuffer->UnInit();
			descriptionBuffer->InitMemory(bufferSize, nullptr);
			descriptionBuffer->InitDevice(false, false);
		}

		void* pDst = nullptr;
		descriptionBuffer->Map(&pDst);
		memcpy(pDst, descriptions.data(), m_VirtualIDCounter * sizeof(glm::uvec4));
		descriptionBuffer->UnMap();
	}

	{
		uint32_t frameIdx = KRenderGlobal::CurrentInFlightFrameIndex;

		IKStorageBufferPtr feedbackResultBuffer = m_FeedbackResultBuffers[frameIdx];
		if (feedbackResultBuffer->GetBufferSize() < m_FeedbackTileCount * sizeof(uint32_t))
		{
			uint32_t bufferSize = 4 * ((m_FeedbackTileCount + 3) / 4) * sizeof(uint32_t);
			feedbackResultBuffer->UnInit();
			feedbackResultBuffer->InitMemory(bufferSize, nullptr);
			feedbackResultBuffer->InitDevice(false, false);
		}

		IKStorageBufferPtr mergedFeedbackResultBuffer = m_MergedFeedbackResultBuffers[frameIdx];
		if (mergedFeedbackResultBuffer->GetBufferSize() < 2 * (1 + m_FeedbackTileCount) * sizeof(uint32_t))
		{
			uint32_t bufferSize = 4 * ((2 * (m_FeedbackTileCount + 1) + 3) / 4) * sizeof(uint32_t);
			mergedFeedbackResultBuffer->UnInit();
			mergedFeedbackResultBuffer->InitMemory(bufferSize, nullptr);
			mergedFeedbackResultBuffer->InitDevice(false, false);
		}
	}
}

bool KVirtualTextureManager::Update(IKCommandBufferPtr primaryBuffer, const std::vector<IKEntity*>& cullRes)
{
	UpdateBuffer();
	LRUSortTile();
	HandleFeedbackResult();

	uint32_t frameIdx = KRenderGlobal::CurrentInFlightFrameIndex;

	{
		primaryBuffer->BeginDebugMarker("VirtualTexture_FeedbackReslutInit", glm::vec4(1));

		struct
		{
			uint32_t totalTileCount = 0;
		} objectData;
		objectData.totalTileCount = m_FeedbackTileCount;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = VIRTUAL_TEXTURE_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		m_InitFeedbackResultPipelines[frameIdx]->Execute(primaryBuffer, (m_FeedbackTileCount + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, &objectUsage);

		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("VirtualTexture_FeedbackResultCount", glm::vec4(1));
		primaryBuffer->Transition(m_FeedbackTargets[frameIdx]->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);

		uint32_t width = m_FeedbackTargets[frameIdx]->GetFrameBuffer()->GetWidth();
		uint32_t height = m_FeedbackTargets[frameIdx]->GetFrameBuffer()->GetHeight();

		m_CountFeedbackResultPipelines[frameIdx]->Execute(primaryBuffer, (width + SQRT_GROUP_SIZE - 1) / SQRT_GROUP_SIZE, (height + SQRT_GROUP_SIZE - 1) / SQRT_GROUP_SIZE, 1, nullptr);

		primaryBuffer->Transition(m_FeedbackTargets[frameIdx]->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	{
		primaryBuffer->BeginDebugMarker("VirtualTexture_FeedbackReslutMerge", glm::vec4(1));

		struct
		{
			uint32_t totalTileCount = 0;
		} objectData;
		objectData.totalTileCount = m_FeedbackTileCount;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = VIRTUAL_TEXTURE_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		m_MergeFeedbackResultPipelines[frameIdx]->Execute(primaryBuffer, (m_FeedbackTileCount + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, &objectUsage);

		primaryBuffer->EndDebugMarker();
	}

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

	uint32_t tileSizeWithPadding = m_TileSize + 2 * m_PaddingSize;

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
					viewport.x = tileSizeWithPadding * update.location.x;
					viewport.y = tileSizeWithPadding * update.location.y;
					viewport.width = tileSizeWithPadding;
					viewport.height = tileSizeWithPadding;

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
	for (size_t i = 0; i < m_InitFeedbackResultPipelines.size(); ++i)
	{
		m_InitFeedbackResultPipelines[i]->Reload();
	}
	for (size_t i = 0; i < m_CountFeedbackResultPipelines.size(); ++i)
	{
		m_CountFeedbackResultPipelines[i]->Reload();
	}
	for (size_t i = 0; i < m_MergeFeedbackResultPipelines.size(); ++i)
	{
		m_MergeFeedbackResultPipelines[i]->Reload();
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
		while (tile != m_UsedTileHead)
		{
			if (tile->payload.useFrameIndex == KRenderGlobal::CurrentFrameNum)
			{
				tile = tile->prev;
			}
			else
			{
				break;
			}
		}
		if (tile->payload.useFrameIndex == KRenderGlobal::CurrentFrameNum || tile->payload.ownerNode->loadStatus == KVirtualTextureTileNode::TILE_LOADING)
		{
			tile = nullptr;
		}
		else
		{
			m_UsedTileHead = tile;
		}
	}

	if (tile)
	{
		tile->payload.useFrameIndex = KRenderGlobal::CurrentFrameNum;
	}

	return tile;
}

bool KVirtualTextureManager::ReturnPhysical(KVirtualTexturePhysicalTile* tile)
{
	if (m_UsedTiles.find(tile) != m_UsedTiles.end())
	{
		RemoveTileFromList(tile, m_UsedTileHead);
		m_UsedTiles.erase(tile);
		AddTileToList(tile, m_FreeTileHead);
		tile->payload.pixelCount = 0;
		tile->payload.useFrameIndex = -1;
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

IKStorageBufferPtr KVirtualTextureManager::GetVirtualTextrueDescriptionBuffer()
{
	return m_VirtualTextrueDescriptionBuffer;
}

IKFrameBufferPtr KVirtualTextureManager::GetPhysicalTextureFramebuffer(uint32_t index)
{
	return m_PhysicalContentTarget->GetFrameBuffer();
}

KSamplerRef KVirtualTextureManager::GetPhysicalTextureSampler(uint32_t index)
{
	return m_PhysicalRenderSampler;
}