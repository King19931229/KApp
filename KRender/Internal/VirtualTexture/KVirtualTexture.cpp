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

KVirtualTextureTileLocation KVirtualTextureTileNode::GetTile(uint32_t x, uint32_t y, uint32_t mipLevel)
{
	KVirtualTextureTileLocation location;
	if (mipLevel < mip)
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			if (children[i])
			{
				location = children[i]->GetTile(x, y, mip);
				if (location.physicalLocation.IsValid())
				{
					return location;
				}
			}
		}
	}
	if (x >= sx && x < ex && y >= sy && y < ey)
	{
		location.mip = mip;
		location.physicalLocation = physicalLocation;
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

	m_TableDebugDrawer.Init(m_TableTexture->GetFrameBuffer(), 0, 0, 1, 1);

	return true;
}

bool KVirtualTexture::UnInit()
{
	SAFE_DELETE(m_RootNode);
	SAFE_UNINIT(m_TableTexture);

	m_TableInfo.clear();

	m_TableDebugDrawer.UnInit();

	return true;
}

bool KVirtualTexture::FeedbackRender(IKCommandBufferPtr primaryBuffer, IKRenderPassPtr renderPass, const std::vector<IKEntity*>& cullRes)
{
	std::vector<KMaterialSubMeshInstance> subMeshInstances;
	KRenderUtil::CalculateInstancesByVirtualTexture(cullRes, m_TableTexture, subMeshInstances);

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