#include "KVirtualTexture.h"
#include "Internal/KRenderGlobal.h"

KVirtualTextureTileNode::KVirtualTextureTileNode(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint32_t mipLevel)
{
	sx = startX;
	sy = startY;
	ex = endX;
	ey = endY;
	mip = mipLevel;
	loadStatus = TILE_UNLOADED;
	physicalTile = nullptr;
}

KVirtualTextureTileNode::~KVirtualTextureTileNode()
{
	for (uint32_t i = 0; i < 4; ++i)
	{
		SAFE_DELETE(children[i]);
	}
}

KVirtualTextureTileNode* KVirtualTextureTileNode::GetNodeWithDataLoaded(uint32_t x, uint32_t y, uint32_t mipLevel)
{
	if (x < sx || x >= ex || y < sy || y >= ey)
	{
		return nullptr;
	}
	if (mipLevel < mip)
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			if (children[i])
			{
				KVirtualTextureTileNode* node = children[i]->GetNodeWithDataLoaded(x, y, mipLevel);
				if (node)
				{
					return node;
				}
			}
		}
	}
	assert(x >= sx && x < ex&& y >= sy && y < ey);
	return (loadStatus == TILE_LOADED) ? this : nullptr;
}

KVirtualTextureTileNode* KVirtualTextureTileNode::GetNode(uint32_t x, uint32_t y, uint32_t mipLevel)
{
	if (x < sx || x >= ex || y < sy || y >= ey)
	{
		return nullptr;
	}

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
			KVirtualTextureTileNode* node = children[i]->GetNode(x, y, mipLevel);
			if (node)
			{
				return node;
			}
		}
		assert(false && "shuold not reach");
		return nullptr;
	}
	else
	{
		if (x >= sx && x < ex && y >= sy && y < ey)
		{
			return this;
		}
		return nullptr;
	}
}

void KVirtualTextureTileNode::ReturnPhysicalTileRecursively(void* owner)
{
	if (physicalTile && physicalTile->userPtr == owner)
	{
		KRenderGlobal::VirtualTextureManager.ReturnPhysical(physicalTile);
		loadStatus = KVirtualTextureTileNode::TILE_UNLOADED;
		physicalTile = nullptr;
	}
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (children[i])
		{
			children[i]->ReturnPhysicalTileRecursively(owner);
		}
	}
}

KVirtualTexture::KVirtualTexture()
	: m_RootNode(nullptr)
	, m_TileNum(0)
	, m_MaxMipLevel(0)
	, m_MaxUpdatePerFrame(0)
	, m_VirtualID(0)
{}

KVirtualTexture::~KVirtualTexture()
{}

bool KVirtualTexture::Init(const std::string& path, uint32_t tileNum, uint32_t virtualID)
{
	UnInit();

	m_Path = path;
	m_TileNum = tileNum;
	m_VirtualID = virtualID;
	m_MaxMipLevel = (uint32_t)std::log2(tileNum);
	m_MaxUpdatePerFrame = 3;
	m_Ext = ".png";

	KRenderGlobal::RenderDevice->CreateTexture(m_TableTexture);
	m_TableTexture->InitMemoryFromData(nullptr, m_Path + "_PageTable", tileNum, tileNum, 1, IF_R8G8B8A8, false, false, false);
	m_TableTexture->InitDevice(false);

	m_TableInfo.resize(tileNum * tileNum);
	memset(m_TableInfo.data(), -1, m_TableInfo.size() * sizeof(uint32_t));

	m_RootNode = new KVirtualTextureTileNode(0, 0, tileNum, tileNum, m_MaxMipLevel);

	m_TableUpdateStorages.resize(KRenderGlobal::NumFramesInFlight);
	m_TableUpdateComputePipelines.resize(KRenderGlobal::NumFramesInFlight);

	for (size_t i = 0; i < m_TableUpdateComputePipelines.size(); ++i)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_TableUpdateStorages[i]);
		m_TableUpdateStorages[i]->SetDebugName(std::string("VirtualTextureTableUpdateStorage_" + m_Path + "_" + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_TableUpdateComputePipelines[i]);

		m_TableUpdateComputePipelines[i]->BindStorageBuffer(BINDING_UPLOAD_INFO, m_TableUpdateStorages[i], COMPUTE_RESOURCE_IN, true);
		m_TableUpdateComputePipelines[i]->BindDynamicUniformBuffer(BINDING_OBJECT);
		m_TableUpdateComputePipelines[i]->BindStorageImage(BINDING_TABLE_IMAGE, m_TableTexture->GetFrameBuffer(), m_TableTexture->GetTextureFormat(), COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);

		KShaderCompileEnvironment env;

#define ENUM_TO_STR(x) #x
#define ADD_MACRO(SEMANTIC) env.macros.push_back(std::make_tuple(ENUM_TO_STR(##SEMANTIC), std::to_string(##SEMANTIC)));
		ADD_MACRO(BINDING_UPLOAD_INFO);
		ADD_MACRO(BINDING_OBJECT);
		ADD_MACRO(BINDING_TABLE_IMAGE);
		ADD_MACRO(UPLOAD_GROUP_SIZE);
#undef ADD_MACRO
#undef ENUM_TO_STR
		 
		m_TableUpdateComputePipelines[i]->Init("virtualtexture/table_upload.comp", env);
	}

	m_TableDebugDrawer.Init(m_TableTexture->GetFrameBuffer(), 0.5f, 0.5f, 0.5f, 0.5f, false);
	m_TableDebugDrawer.EnableDraw();

	return true;
}

bool KVirtualTexture::UnInit()
{
	if (m_RootNode)
	{
		m_RootNode->ReturnPhysicalTileRecursively(this);
	}

	SAFE_DELETE(m_RootNode);
	SAFE_UNINIT(m_TableTexture);

	SAFE_UNINIT_CONTAINER(m_TableUpdateStorages);
	SAFE_UNINIT_CONTAINER(m_TableUpdateComputePipelines);

	m_HashedTileRequests.clear();
	m_TableInfo.clear();
	m_PendingTableUpdates.clear();

	m_TableDebugDrawer.UnInit();

	return true;
}

bool KVirtualTexture::FeedbackRender(IKCommandBufferPtr primaryBuffer, IKRenderPassPtr renderPass, uint32_t targetBinding, const std::vector<IKEntity*>& cullRes)
{
	std::vector<KMaterialSubMeshInstance> subMeshInstances;
	KRenderUtil::CalculateInstancesByVirtualTexture(cullRes, targetBinding, m_TableTexture, subMeshInstances);

	for (KMaterialSubMeshInstance& subMeshInstance : subMeshInstances)
	{
		const std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = subMeshInstance.instanceData;
		ASSERT_RESULT(!instances.empty());

		if (instances.size() > 1)
		{
			KRenderCommand command;
			if (subMeshInstance.materialSubMesh->GetRenderCommand(RENDER_STAGE_VIRTUAL_TEXTURE_FEEDBACK_INSTANCE, command))
			{
				if (!KRenderUtil::AssignShadingParameter(command, subMeshInstance.materialSubMesh->GetMaterial()))
				{
					continue;
				}
				if (!KRenderUtil::AssignVirtualFeedbackParameter(command, targetBinding, this))
				{
					continue;
				}

				std::vector<KInstanceBufferManager::AllocResultBlock> allocRes;
				ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.GetVertexSize() == sizeof(instances[0]));
				ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.Alloc(instances.size(), instances.data(), allocRes));

				command.instanceDraw = true;
				command.instanceUsages.resize(allocRes.size());
				for (size_t i = 0; i < allocRes.size(); ++i)
				{
					KInstanceBufferUsage& usage = command.instanceUsages[i];
					KInstanceBufferManager::AllocResultBlock& allocResult = allocRes[i];
					usage.buffer = allocResult.buffer;
					usage.start = allocResult.start;
					usage.count = allocResult.count;
					usage.offset = allocResult.offset;
				}

				command.pipeline->GetHandle(renderPass, command.pipelineHandle);

				if (command.Complete())
				{
					primaryBuffer->Render(command);
				}
			}
		}
		else
		{
			for (size_t idx = 0; idx < instances.size(); ++idx)
			{
				KRenderCommand command;
				if (subMeshInstance.materialSubMesh->GetRenderCommand(RENDER_STAGE_VIRTUAL_TEXTURE_FEEDBACK, command))
				{
					if (!KRenderUtil::AssignShadingParameter(command, subMeshInstance.materialSubMesh->GetMaterial()))
					{
						continue;
					}
					if (!KRenderUtil::AssignVirtualFeedbackParameter(command, targetBinding, this))
					{
						continue;
					}

					const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[idx];

					KConstantDefinition::OBJECT objectData;
					objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
					objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));

					KDynamicConstantBufferUsage objectUsage;
					objectUsage.binding = SHADER_BINDING_OBJECT;
					objectUsage.range = sizeof(objectData);
					KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

					command.dynamicConstantUsages.push_back(objectUsage);

					command.pipeline->GetHandle(renderPass, command.pipelineHandle);

					if (command.Complete())
					{
						primaryBuffer->Render(command);
					}
				}
			}
		}
	}

	return true;
}

bool KVirtualTexture::UpdateTableTexture(IKCommandBufferPtr primaryBuffer)
{
	uint32_t frameIndex = KRenderGlobal::CurrentInFlightFrameIndex;
	m_TableUpdateStorages[frameIndex]->UnInit();

	if (m_PendingTableUpdates.size() > 0)
	{
		primaryBuffer->BeginDebugMarker(("VirtualTexture_UpdateTable_" + m_Path).c_str(), glm::vec4(1));

		m_TableUpdateStorages[frameIndex]->InitMemory(m_PendingTableUpdates.size() * sizeof(KVirtualTextureTableUpdate), m_PendingTableUpdates.data());
		m_TableUpdateStorages[frameIndex]->InitDevice(false, false);

		struct
		{
			glm::uvec4 dimension;
		} uploadUsage;
		static_assert((sizeof(uploadUsage) % 16) == 0, "Size must be a multiple of 16");

		uploadUsage.dimension[0] = m_TileNum;
		uploadUsage.dimension[1] = m_TileNum;
		uploadUsage.dimension[2] = (uint32_t)m_PendingTableUpdates.size();

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = BINDING_OBJECT;
		objectUsage.range = sizeof(uploadUsage);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&uploadUsage, objectUsage);

		primaryBuffer->Transition(m_TableTexture->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		m_TableUpdateComputePipelines[frameIndex]->Execute(primaryBuffer, (uint32_t)(m_PendingTableUpdates.size() + UPLOAD_GROUP_SIZE - 1) / UPLOAD_GROUP_SIZE, 1, 1, &objectUsage);
		primaryBuffer->Transition(m_TableTexture->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();

		m_PendingTableUpdates.clear();
	}

	return true;
}

void KVirtualTexture::BeginRequest()
{
	m_HashedTileRequests.clear();
}

void KVirtualTexture::AddRequest(const KVirtualTextureTile& tile)
{
	if (tile.IsValid() && tile.x < m_TileNum && tile.y < m_TileNum)
	{
		auto it = m_HashedTileRequests.find(tile);
		if (it == m_HashedTileRequests.end())
		{
			m_HashedTileRequests[tile] = 1;
		}
		else
		{
			++it->second;
		}
	}
}

void KVirtualTexture::EndRequest()
{
	std::vector<KVirtualTextureTileRequest> requests;
	requests.reserve(m_HashedTileRequests.size());

	for (auto& pair : m_HashedTileRequests)
	{
		KVirtualTextureTileRequest request;
		request.tile = pair.first;
		request.count = pair.second;
		requests.push_back(request);
	}

	std::sort(requests.begin(), requests.end(), [](const KVirtualTextureTileRequest& lhs, const KVirtualTextureTileRequest& rhs)
	{
		return lhs.count > rhs.count;
	});

	std::vector<uint32_t> tileWrited;
	tileWrited.resize(m_TableInfo.size());
	memset(tileWrited.data(), 0, sizeof(uint32_t) * tileWrited.size());

	std::vector<KVirtualTextureTileNode*> currentTileUpdates;

	for (const KVirtualTextureTileRequest& request : requests)
	{
		uint32_t id = request.tile.y * m_TileNum + request.tile.x;
		if (!tileWrited[id])
		{
			uint32_t x = request.tile.x;
			uint32_t y = request.tile.y;
			uint32_t mip = request.tile.mip;

			KVirtualTexturePhysicalTile* physicalTile = nullptr;
			KVirtualTextureTileNode* uploadNode = nullptr;

			KVirtualTextureTileNode* node = m_RootNode->GetNodeWithDataLoaded(x, y, mip);
			if (node)
			{
				physicalTile = node->physicalTile;
				if (node->mip > mip)
				{
					uploadNode = m_RootNode->GetNode(x, y, node->mip - 1);
					assert(uploadNode->mip == node->mip - 1);
				}
			}
			else
			{
				uploadNode = m_RootNode->GetNode(x, y, m_MaxMipLevel);
			}

			if (uploadNode)
			{
				if (uploadNode->loadStatus == KVirtualTextureTileNode::TILE_UNLOADED)
				{
					uploadNode->physicalTile = KRenderGlobal::VirtualTextureManager.RequestPhysical();
					uploadNode->loadStatus = KVirtualTextureTileNode::TILE_LOADING;
					uploadNode->physicalTile->userPtr = this;
					currentTileUpdates.push_back(uploadNode);
				}
			}

			if (physicalTile)
			{
				KVirtualTexturePhysicalLocation location = physicalTile->physicalLocation;
				if (location.IsValid())
				{
					uint32_t data = location.x | (location.y << 8) | (location.mip << 16) | (node->mip << 24);
					if (m_TableInfo[id] != data)
					{
						m_TableInfo[id] = data;
						m_PendingTableUpdates.push_back({id, data});
					}
				}
			}

			tileWrited[id] = true;
		}
	}

	std::sort(currentTileUpdates.begin(), currentTileUpdates.end(), [](const KVirtualTextureTileNode* lhs, const KVirtualTextureTileNode* rhs)
	{
		return lhs->mip > rhs->mip;
	});

	m_PendingTileUpdates.insert(m_PendingTileUpdates.end(), currentTileUpdates.begin(), currentTileUpdates.end());

	ProcessPendingUpdate();
}

void KVirtualTexture::ProcessPendingUpdate()
{
	uint32_t processCount = 0;
	uint32_t currentIdx = 0;

	while (currentIdx < m_PendingTileUpdates.size() && processCount < m_MaxUpdatePerFrame)
	{
		KVirtualTextureTileNode* updateTile = m_PendingTileUpdates[currentIdx];
		if (updateTile->physicalTile->userPtr == this)
		{
			uint32_t x = updateTile->sx / (1 << updateTile->mip);
			uint32_t y = updateTile->sy / (1 << updateTile->mip);
			uint32_t mip = updateTile->mip;

			std::string textureName = m_Path + "_MIP" + std::to_string(mip) + "_Y" + std::to_string(y) + "_X" + std::to_string(x) + m_Ext;
			KRenderGlobal::VirtualTextureManager.UploadToPhysical(textureName, updateTile->physicalTile->physicalLocation);

			++processCount;
			updateTile->loadStatus = KVirtualTextureTileNode::TILE_LOADED;
		}
		else
		{
			updateTile->physicalTile = nullptr;
			updateTile->loadStatus = KVirtualTextureTileNode::TILE_UNLOADED;
		}
		++currentIdx;
	}

	m_PendingTileUpdates.erase(m_PendingTileUpdates.begin(), m_PendingTileUpdates.begin() + currentIdx);
}

bool KVirtualTexture::TableDebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	return m_TableDebugDrawer.Render(renderPass, primaryBuffer);
}