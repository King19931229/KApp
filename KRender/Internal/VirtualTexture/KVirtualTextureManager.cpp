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
	, m_VirtualTextureFeedbackRatio(8)
	, m_VirtualTextureFeedbackWidth(0)
	, m_VirtualTextureFeedbackHeight(0)
	, m_VirtualIDCounter(0)
	, m_GPUProcessFeedback(true)
	, m_EnableStandaloneFeedbackPass(false)
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
	m_PendingSourceTextures.resize(frameNum);

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_PhysicalContentTarget);
	m_PhysicalContentTarget->InitFromColor(m_SizeWithPadding, m_SizeWithPadding, 1, m_NumMips, EF_R8G8B8A8_UNORM);
	m_PhysicalContentTarget->GetFrameBuffer()->SetDebugName("VirtualTexturePhysicalContent");

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

	m_PendingMipContentUpdates.resize(m_NumMips);
	m_UploadContentMipPasses.resize(m_NumMips);

	for (uint32_t idx = 0; idx < m_NumMips; ++idx)
	{
		KRenderGlobal::RenderDevice->CreateRenderPass(m_UploadContentMipPasses[idx]);
		m_UploadContentMipPasses[idx]->SetColorAttachment(0, m_PhysicalContentTarget->GetFrameBuffer());
		m_UploadContentMipPasses[idx]->SetOpColor(0, LO_LOAD, SO_STORE);
		m_UploadContentMipPasses[idx]->Init(idx);
	}

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_VirtualTextrueDescriptionBuffer);
	m_VirtualTextrueDescriptionBuffer->SetDebugName("VirtualTextrueDescription");
	m_VirtualTextrueDescriptionBuffer->InitMemory(1, false);
	m_VirtualTextrueDescriptionBuffer->InitDevice(false, false);

	Resize(1024, 1024);

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_FeedbackResultBuffer);
	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MergedFeedbackResultBuffer);
	KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitFeedbackResultPipeline);
	KRenderGlobal::RenderDevice->CreateComputePipeline(m_StandaloneCountFeedbackResultPipeline);
	KRenderGlobal::RenderDevice->CreateComputePipeline(m_GBufferCountFeedbackResultPipeline);
	KRenderGlobal::RenderDevice->CreateComputePipeline(m_MergeFeedbackResultPipeline);
	KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitFeedbackTargetPipeline);

	m_FeedbackResultBuffer->SetDebugName("FeedbackResultBuffer");
	m_FeedbackResultBuffer->InitMemory(1024, false);
	m_FeedbackResultBuffer->InitDevice(false, false);

	m_MergedFeedbackResultBuffer->SetDebugName("MergedFeedbackResultBuffer");
	m_MergedFeedbackResultBuffer->InitMemory(1024, false);
	m_MergedFeedbackResultBuffer->InitDevice(false, false);

	m_InitFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_RESULT, m_FeedbackResultBuffer, COMPUTE_RESOURCE_OUT, true);
	m_InitFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_MERGED_FEEDBACK_RESULT, m_MergedFeedbackResultBuffer, COMPUTE_RESOURCE_OUT, true);
	m_InitFeedbackResultPipeline->BindDynamicUniformBuffer(VIRTUAL_TEXTURE_BINDING_OBJECT);
	m_InitFeedbackResultPipeline->Init("virtualtexture/init_feedback_result.comp", m_CompileEnv);

	KShaderCompileEnvironment standaloneCountFeedbackEnv;
	standaloneCountFeedbackEnv.macros.push_back({ "READ_FEEDBACK_FROM_FRAMEBUFFER","1" });
	standaloneCountFeedbackEnv.parentEnv = &m_CompileEnv;

	m_StandaloneCountFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_RESULT, m_FeedbackResultBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_StandaloneCountFeedbackResultPipeline->BindStorageImage(VIRTUAL_TEXTURE_BINDING_FEEDBACK_INPUT, m_FeedbackTarget->GetFrameBuffer(), EF_R8G8B8A8_UNORM, COMPUTE_RESOURCE_IN, 0, true);
	m_StandaloneCountFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_TEXTURE_DESCRIPTION, m_VirtualTextrueDescriptionBuffer, COMPUTE_RESOURCE_IN, true);
	m_StandaloneCountFeedbackResultPipeline->BindUniformBuffer(VIRTUAL_TEXTURE_BINDING_CONSTANT, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VIRTUAL_TEXTURE_CONSTANT));
	m_StandaloneCountFeedbackResultPipeline->Init("virtualtexture/count_feedback.comp", standaloneCountFeedbackEnv);

	KShaderCompileEnvironment gbufferCountFeedbackEnv;
	gbufferCountFeedbackEnv.parentEnv = &m_CompileEnv;

	m_GBufferCountFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_RESULT, m_FeedbackResultBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_GBufferCountFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_INPUT, m_VirtualTextureFeedbackBuffer, COMPUTE_RESOURCE_IN, true);
	m_GBufferCountFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_TEXTURE_DESCRIPTION, m_VirtualTextrueDescriptionBuffer, COMPUTE_RESOURCE_IN, true);
	m_GBufferCountFeedbackResultPipeline->BindUniformBuffer(VIRTUAL_TEXTURE_BINDING_CONSTANT, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VIRTUAL_TEXTURE_CONSTANT));
	m_GBufferCountFeedbackResultPipeline->Init("virtualtexture/count_feedback.comp", gbufferCountFeedbackEnv);

	m_MergeFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_RESULT, m_FeedbackResultBuffer, COMPUTE_RESOURCE_IN, true);
	m_MergeFeedbackResultPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_MERGED_FEEDBACK_RESULT, m_MergedFeedbackResultBuffer, COMPUTE_RESOURCE_OUT, true);
	m_MergeFeedbackResultPipeline->BindDynamicUniformBuffer(VIRTUAL_TEXTURE_BINDING_OBJECT);
	m_MergeFeedbackResultPipeline->Init("virtualtexture/merge_feedback.comp", m_CompileEnv);

	m_InitFeedbackTargetPipeline->BindStorageBuffer(VIRTUAL_TEXTURE_BINDING_FEEDBACK_INPUT, m_VirtualTextureFeedbackBuffer, COMPUTE_RESOURCE_OUT, true);
	m_InitFeedbackTargetPipeline->BindUniformBuffer(VIRTUAL_TEXTURE_BINDING_CONSTANT, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VIRTUAL_TEXTURE_CONSTANT));
	m_InitFeedbackTargetPipeline->Init("virtualtexture/init_feedback_target.comp", m_CompileEnv);

	{
		KRenderGlobal::ImmediateCommandList.BeginRecord();

		for (uint32_t mip = 0; mip < m_NumMips; ++mip)
		{
			KRenderGlobal::ImmediateCommandList.BeginRenderPass(m_UploadContentMipPasses[mip], SUBPASS_CONTENTS_INLINE);
			KRenderGlobal::ImmediateCommandList.SetViewport(m_UploadContentMipPasses[mip]->GetViewPort());
			KRenderGlobal::ImmediateCommandList.EndRenderPass();
		}

		KRenderGlobal::ImmediateCommandList.Transition(m_PhysicalContentTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		KRenderGlobal::ImmediateCommandList.EndRecord();
	}

	m_FeedbackDebugDrawer.Init(m_FeedbackTarget->GetFrameBuffer(), 0, 0, 1, 1, false);
	m_PhysicalDebugDrawer.Init(m_PhysicalContentTarget->GetFrameBuffer(), 0, 0, 0.5, 0.5, false);

	return true;
}

bool KVirtualTextureManager::UnInit()
{
	m_UsedTiles.clear();
	m_PhysicalTiles.clear();
	m_TextureMap.clear();
	m_PendingMipContentUpdates.clear();
	m_PendingSourceTextures.clear();

	m_PhysicalUpdateSampler.Release();
	m_PhysicalRenderSampler.Release();
	m_QuadVS.Release();
	m_UploadFS.Release();

	m_FreeTileHead = nullptr;
	m_UsedTileHead = nullptr;

	SAFE_UNINIT(m_UploadContentPipeline);

	SAFE_UNINIT(m_PhysicalContentTarget);

	SAFE_UNINIT_CONTAINER(m_UploadContentMipPasses);

	SAFE_UNINIT(m_FeedbackTarget);
	SAFE_UNINIT(m_FeedbackDepth);
	SAFE_UNINIT(m_FeedbackPass);
	SAFE_UNINIT(m_ResultReadbackTarget);

	SAFE_UNINIT(m_VirtualTextrueDescriptionBuffer);
	SAFE_UNINIT(m_VirtualTextureFeedbackBuffer);

	SAFE_UNINIT(m_FeedbackResultBuffer);
	SAFE_UNINIT(m_MergedFeedbackResultBuffer);
	SAFE_UNINIT(m_InitFeedbackResultPipeline);
	SAFE_UNINIT(m_StandaloneCountFeedbackResultPipeline);
	SAFE_UNINIT(m_GBufferCountFeedbackResultPipeline);
	SAFE_UNINIT(m_MergeFeedbackResultPipeline);
	SAFE_UNINIT(m_InitFeedbackTargetPipeline);

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

void KVirtualTextureManager::AddTileToList(KVirtualTexturePhysicalTile* tile, KVirtualTexturePhysicalTile*& head)
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
		return lhs->payload.ownerNode->mip < rhs->payload.ownerNode->mip;
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
	if (!m_TextureMap.size())
	{
		return;
	}

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

	if (m_GPUProcessFeedback)
	{
		uint32_t* feedbackResultData = nullptr;
		m_MergedFeedbackResultBuffer->Map((void**)&feedbackResultData);

		uint32_t dataCount = feedbackResultData[0];
		if (dataCount > 0)
		{
			for (uint32_t i = 0; i < dataCount; ++i)
			{
				uint32_t tileIndex = feedbackResultData[2 + 2 * i];
				uint32_t count = feedbackResultData[2 + 2 * i + 1];

				uint32_t virtualID = -1;
				uint32_t tileOffset = -1;

				for (const TileOffsetRecord& record : m_TileOffsetRecords)
				{
					if (tileIndex >= record.tileOffset)
					{
						virtualID = record.virtualID;
						tileOffset = record.tileOffset;
						break;
					}
				}

				assert(virtualID != -1);
				assert(count > 0);

				if (virtualID < virtualIdMap.size())
				{
					KVirtualTextureResourceRef resource = virtualIdMap[virtualID];

					uint32_t inTileOffset = tileIndex - tileOffset;
					uint32_t maxMipLevel = resource->GetMaxMipLevel();
					uint32_t textureTileNum = resource->GetTileNum();
					for (int32_t mipLevel = maxMipLevel; mipLevel >= 0; --mipLevel)
					{
						uint32_t mipWriteOffset = mipLevel * textureTileNum * textureTileNum;
						if (inTileOffset >= mipWriteOffset)
						{
							uint32_t inMipOffset = inTileOffset - mipWriteOffset;
							uint32_t pageX = inMipOffset % textureTileNum;
							uint32_t pageY = inMipOffset / textureTileNum;
							KVirtualTextureTile tile;
							tile.x = pageX;
							tile.y = pageY;
							tile.mip = mipLevel;
							virtualIdMap[virtualID]->AddRequest(tile, count);
							break;
						}
					}
				}
			}
		}

		m_MergedFeedbackResultBuffer->UnMap();
	}
	else
	{
		IKFrameBufferPtr src = m_FeedbackTarget->GetFrameBuffer();
		IKFrameBufferPtr dest = m_ResultReadbackTarget->GetFrameBuffer();
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
					virtualIdMap[virtualID]->AddRequest(tile, 1);
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
		description.z = m_VirtualTextureFeedbackWidth;
		description.w = m_VirtualTextureFeedbackHeight;

		glm::uvec4 description2;
		description2.x = m_VirtualTextureFeedbackRatio;
		description2.y = m_VirtualTextureFeedbackWidth * m_VirtualTextureFeedbackHeight;
		description2.z = KRenderGlobal::CurrentFrameNum % 4;
		description2.w = (KRenderGlobal::CurrentFrameNum / 4) % 4;

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
			else if (detail.semantic == CS_VIRTUAL_TEXTURE_DESCRIPTION2)
			{
				assert(sizeof(description2) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				memcpy(pWritePos, &description2, sizeof(description2));
			}
		}
		virtualTextureBuffer->Write(pData);
	}

	m_TileOffsetRecords.clear();
	m_TileOffsetRecords.reserve(m_TextureMap.size());
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
			descriptions[id].z = m_FeedbackTileCount;
			descriptions[id].w = resource->GetTileNum() * m_TileSize;

			m_TileOffsetRecords.push_back({ id, m_FeedbackTileCount });
			m_FeedbackTileCount += descriptions[id].x * descriptions[id].x * (descriptions[id].y + 1);
		}

		IKStorageBufferPtr descriptionBuffer = m_VirtualTextrueDescriptionBuffer;

		if (descriptionBuffer->GetBufferSize() < m_VirtualIDCounter * sizeof(glm::uvec4))
		{
			KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThreadToDone);
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
		IKStorageBufferPtr feedbackResultBuffer = m_FeedbackResultBuffer;
		if (feedbackResultBuffer->GetBufferSize() < m_FeedbackTileCount * sizeof(uint32_t))
		{
			uint32_t bufferSize = 4 * ((m_FeedbackTileCount + 3) / 4) * sizeof(uint32_t);
			feedbackResultBuffer->UnInit();
			feedbackResultBuffer->InitMemory(bufferSize, nullptr);
			feedbackResultBuffer->InitDevice(false, false);
		}

		IKStorageBufferPtr mergedFeedbackResultBuffer = m_MergedFeedbackResultBuffer;
		if (mergedFeedbackResultBuffer->GetBufferSize() < 2 * (1 + m_FeedbackTileCount) * sizeof(uint32_t))
		{
			uint32_t bufferSize = 4 * ((2 * (m_FeedbackTileCount + 1) + 3) / 4) * sizeof(uint32_t);
			mergedFeedbackResultBuffer->UnInit();
			mergedFeedbackResultBuffer->InitMemory(bufferSize, nullptr);
			mergedFeedbackResultBuffer->InitDevice(false, false);
		}
	}
}

void KVirtualTextureManager::FeedbackRender(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	commandList.Transition(m_FeedbackTarget->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);

	if (m_EnableStandaloneFeedbackPass)
	{
		commandList.BeginDebugMarker("VirtualTexture_FeedbackRender", glm::vec4(1));
		commandList.BeginRenderPass(m_FeedbackPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(m_FeedbackPass->GetViewPort());

		for (auto& pair : m_TextureMap)
		{
			KVirtualTextureResourceRef resource = pair.second;
			resource->FeedbackRender(commandList, m_FeedbackPass, m_CurrentTargetBinding, cullRes);
		}

		commandList.EndRenderPass();
		commandList.EndDebugMarker();
	}
}

void KVirtualTextureManager::DispatchFeedbackAnalyze(KRHICommandList& commandList)
{
	{
		commandList.BeginDebugMarker("VirtualTexture_FeedbackResultInit", glm::vec4(1));

		struct
		{
			uint32_t totalTileCount = 0;
		} objectData;
		objectData.totalTileCount = m_FeedbackTileCount;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = VIRTUAL_TEXTURE_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		commandList.Execute(m_InitFeedbackResultPipeline, (m_FeedbackTileCount + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, &objectUsage);

		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("VirtualTexture_FeedbackResultCount", glm::vec4(1));
		commandList.Transition(m_FeedbackTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_GENERAL);

		if (m_EnableStandaloneFeedbackPass)
		{
			uint32_t width = m_FeedbackTarget->GetFrameBuffer()->GetWidth();
			uint32_t height = m_FeedbackTarget->GetFrameBuffer()->GetHeight();
			commandList.Execute(m_StandaloneCountFeedbackResultPipeline, (width + SQRT_GROUP_SIZE - 1) / SQRT_GROUP_SIZE, (height + SQRT_GROUP_SIZE - 1) / SQRT_GROUP_SIZE, 1, nullptr);
		}
		else
		{
			commandList.Execute(m_GBufferCountFeedbackResultPipeline, (m_VirtualTextureFeedbackWidth * m_VirtualTextureFeedbackHeight + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, nullptr);
		}

		commandList.Transition(m_FeedbackTarget->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_COLOR_ATTACHMENT);

		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("VirtualTexture_FeedbackReslutMerge", glm::vec4(1));

		struct
		{
			uint32_t totalTileCount = 0;
		} objectData;
		objectData.totalTileCount = m_FeedbackTileCount;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = VIRTUAL_TEXTURE_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		commandList.Execute(m_MergeFeedbackResultPipeline, (m_FeedbackTileCount + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, &objectUsage);

		commandList.EndDebugMarker();
	}

	commandList.Transition(m_FeedbackTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commandList.Transition(m_PhysicalContentTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
}

void KVirtualTextureManager::ProcessPhysicalUpdate(KRHICommandList& commandList)
{
	commandList.Transition(m_PhysicalContentTarget->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);

	uint32_t frameIdx = KRenderGlobal::CurrentInFlightFrameIndex;
	m_PendingSourceTextures[frameIdx].clear();

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.indexDraw = true;

	uint32_t tileSizeWithPadding = m_TileSize + 2 * m_PaddingSize;

	for (uint32_t mip = 0; mip < m_NumMips; ++mip)
	{
		if (m_PendingMipContentUpdates[mip].size())
		{
			std::vector<KVirtualTexturePhysicalUpdate> updates = std::move(m_PendingMipContentUpdates[mip]);

			commandList.BeginDebugMarker("VirtualTexture_PhysicalContentUpdate_" + std::to_string(mip), glm::vec4(1));
			commandList.BeginRenderPass(m_UploadContentMipPasses[mip], SUBPASS_CONTENTS_INLINE);
			commandList.SetViewport(m_UploadContentMipPasses[mip]->GetViewPort());

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
					command.pipeline->GetHandle(m_UploadContentMipPasses[mip], command.pipelineHandle);

					commandList.SetViewport(viewport);
					commandList.Render(command);
				}
			}

			commandList.EndRenderPass();
			commandList.EndDebugMarker();
		}
	}

	m_CurrentTargetBinding = (m_CurrentTargetBinding + 1) / MAX_MATERIAL_TEXTURE_BINDING;

	{
		for (auto& pair : m_TextureMap)
		{
			KVirtualTextureResourceRef resource = pair.second;
			resource->UpdateTexture(commandList);
		}
	}
}

bool KVirtualTextureManager::InitFeedbackTarget(KRHICommandList& commandList)
{
	commandList.BeginDebugMarker("VirtualTexture_FeedbackTargetInit", glm::vec4(1));
	commandList.Execute(m_InitFeedbackTargetPipeline, (m_VirtualTextureFeedbackWidth * m_VirtualTextureFeedbackHeight + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, nullptr);
	commandList.EndDebugMarker();
	return true;
}

bool KVirtualTextureManager::Update(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	UpdateBuffer();
	LRUSortTile();
	HandleFeedbackResult();
	ProcessPhysicalUpdate(commandList);
	FeedbackRender(commandList, cullRes);
	DispatchFeedbackAnalyze(commandList);
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
		if (!m_ResultReadbackTarget)
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_ResultReadbackTarget);
		}
		if (!m_VirtualTextureFeedbackBuffer)
		{
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_VirtualTextureFeedbackBuffer);
		}

		m_FeedbackTarget->UnInit();
		m_FeedbackDepth->UnInit();
		m_FeedbackPass->UnInit();
		m_ResultReadbackTarget->UnInit();
		m_VirtualTextureFeedbackBuffer->UnInit();

		uint32_t width = (m_Width + m_VirtualTextureFeedbackRatio - 1) / m_VirtualTextureFeedbackRatio;
		uint32_t height = (m_Height + m_VirtualTextureFeedbackRatio - 1) / m_VirtualTextureFeedbackRatio;

		m_FeedbackTarget->InitFromColor(width, height, 1, 1, EF_R8G8B8A8_UNORM);
		m_FeedbackDepth->InitFromDepthStencil(width, height, 1, false);
		m_ResultReadbackTarget->InitFromReadback(width, height, 1, 1, EF_R8G8B8A8_UNORM);

		m_FeedbackPass->SetColorAttachment(0, m_FeedbackTarget->GetFrameBuffer());
		m_FeedbackPass->SetClearColor(0, { 1,1,1,1 });
		m_FeedbackPass->SetDepthStencilAttachment(m_FeedbackDepth->GetFrameBuffer());
		m_FeedbackPass->Init();

		width = (m_Width + m_VirtualTextureFeedbackRatio - 1) / m_VirtualTextureFeedbackRatio;
		height = (m_Height + m_VirtualTextureFeedbackRatio - 1) / m_VirtualTextureFeedbackRatio;

		m_VirtualTextureFeedbackWidth = width;
		m_VirtualTextureFeedbackHeight = height;
		m_VirtualTextureFeedbackBuffer->InitMemory(sizeof(float) * 4 * m_VirtualTextureFeedbackWidth * m_VirtualTextureFeedbackHeight, nullptr);
		m_VirtualTextureFeedbackBuffer->InitDevice(false, false);
		m_VirtualTextureFeedbackBuffer->SetDebugName("VirtualTextureFeedbackBuffer");
	}

	KRenderGlobal::ImmediateCommandList.BeginRecord();

	KRenderGlobal::ImmediateCommandList.BeginRenderPass(m_FeedbackPass, SUBPASS_CONTENTS_INLINE);
	KRenderGlobal::ImmediateCommandList.SetViewport(m_FeedbackPass->GetViewPort());
	KRenderGlobal::ImmediateCommandList.EndRenderPass();
	KRenderGlobal::ImmediateCommandList.Transition(m_FeedbackTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	KRenderGlobal::ImmediateCommandList.EndRecord();
}

bool KVirtualTextureManager::Reload()
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
		m_UploadContentPipeline->Reload(false);
	}
	if (m_InitFeedbackResultPipeline)
	{
		m_InitFeedbackResultPipeline->Reload(false);
	}
	if (m_StandaloneCountFeedbackResultPipeline)
	{
		m_StandaloneCountFeedbackResultPipeline->Reload(false);
	}
	if (m_GBufferCountFeedbackResultPipeline)
	{
		m_GBufferCountFeedbackResultPipeline->Reload(false);
	}
	if (m_MergeFeedbackResultPipeline)
	{
		m_MergeFeedbackResultPipeline->Reload(false);
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
	m_PendingMipContentUpdates[location.mip].push_back(newUpdate);
}

bool KVirtualTextureManager::FeedbackDebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	return m_FeedbackDebugDrawer.Render(renderPass, commandList);
}

bool KVirtualTextureManager::PhysicalDebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	return m_PhysicalDebugDrawer.Render(renderPass, commandList);
}

bool KVirtualTextureManager::TableDebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	for (auto& pair : m_TextureMap)
	{
		KVirtualTextureResourceRef resource = pair.second;
		resource->TableDebugRender(renderPass, commandList);
	}
	return true;
}

IKStorageBufferPtr KVirtualTextureManager::GetVirtualTextureFeedbackBuffer()
{ 
	return m_VirtualTextureFeedbackBuffer;
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