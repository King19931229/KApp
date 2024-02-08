#include "KGPUScene.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/KRenderGlobal.h"

KGPUScene::KGPUScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_SceneDirty(false)
{
	m_OnSceneChangedFunc = std::bind(&KGPUScene::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
	m_OnRenderComponentChangedFunc = std::bind(&KGPUScene::OnRenderComponentChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KGPUScene::~KGPUScene()
{
}

KGPUScene::SceneEntityPtr KGPUScene::CreateEntity(IKEntity* entity)
{
	auto it = m_EntityMap.find(entity);
	if (it != m_EntityMap.end())
	{
		assert(false && "shuold not reach");
		return it->second;
	}
	else
	{
		SceneEntityPtr sceneEntity(KNEW SceneEntity());
		sceneEntity->index = (uint32_t)m_EntityMap.size();
		m_Entities.push_back(sceneEntity);
		m_EntityMap.insert({ entity, sceneEntity });
		return sceneEntity;
	}
}

KGPUScene::SceneEntityPtr KGPUScene::GetEntity(IKEntity* entity)
{
	auto it = m_EntityMap.find(entity);
	if (it != m_EntityMap.end())
	{
		return it->second;
	}
	return nullptr;
}

uint32_t KGPUScene::CreateOrGetSubMeshIndex(KSubMeshPtr subMesh, bool create)
{
	uint32_t index = -1;
	auto it = m_SubMeshToIndex.find(subMesh);
	if (it != m_SubMeshToIndex.end())
	{
		index = it->second;
		m_SubMeshes[index].refCount += create;
	}
	else
	{
		m_SceneDirty = create;
		SubMeshItem newItem;
		newItem.refCount = 1;
		newItem.subMesh = subMesh;
		index = (uint32_t)m_SubMeshes.size();
		m_SubMeshes.push_back(newItem);
		m_SubMeshToIndex[subMesh] = index;
	}
	assert(index != -1);
	return index;
}

uint32_t KGPUScene::CreateOrGetMaterialIndex(IKMaterialPtr material, bool create)
{
	uint32_t index = -1;
	auto it = m_MaterialToIndex.find(material);
	if (it != m_MaterialToIndex.end())
	{
		index = it->second;
		m_Materials[index].refCount += create;
	}
	else
	{
		m_SceneDirty = create;
		MaterialItem newItem;
		newItem.refCount = 1;
		newItem.material = material;
		index = (uint32_t)m_Materials.size();
		m_Materials.push_back(newItem);
		m_MaterialToIndex[material] = index;
	}
	assert(index != -1);
	return index;
}

void KGPUScene::RemoveSubMesh(KSubMeshPtr subMesh)
{
	uint32_t removeIndex = -1;
	auto it = m_SubMeshToIndex.find(subMesh);
	if (it != m_SubMeshToIndex.end())
	{
		uint32_t index = it->second;
		m_SubMeshes[index].refCount -= 1;
		if (m_SubMeshes[index].refCount == 0)
		{
			m_SubMeshToIndex.erase(it);
			removeIndex = index;
		}
	}
	if (removeIndex != -1)
	{
		m_SceneDirty = true;
		for (auto& pair : m_SubMeshToIndex)
		{
			if (pair.second > removeIndex)
			{
				pair.second -= 1;
			}
		}
	}
}

void KGPUScene::RemoveMaterial(IKMaterialPtr material)
{
	uint32_t removeIndex = -1;
	auto it = m_MaterialToIndex.find(material);
	if (it != m_MaterialToIndex.end())
	{
		uint32_t index = it->second;
		m_Materials[index].refCount -= 1;
		if (m_Materials[index].refCount == 0)
		{
			m_MaterialToIndex.erase(it);
			removeIndex = index;
		}
	}
	if (removeIndex != -1)
	{
		m_SceneDirty = true;
		for (auto& pair : m_MaterialToIndex)
		{
			if (pair.second > removeIndex)
			{
				pair.second -= 1;
			}
		}
	}
}

bool KGPUScene::AddEntity(IKEntity* entity, const glm::mat4& transform, const std::vector<KMaterialSubMeshPtr>& subMeshes)
{
	if (entity)
	{
		SceneEntityPtr sceneEntity = CreateEntity(entity);
		sceneEntity->prevTransform = transform;
		sceneEntity->transform = transform;
		sceneEntity->materialSubMeshes = subMeshes;
		for (KMaterialSubMeshPtr materialSubMesh : sceneEntity->materialSubMeshes)
		{
			CreateOrGetSubMeshIndex(materialSubMesh->GetSubMesh(), true);
			CreateOrGetMaterialIndex(materialSubMesh->GetMaterial().Get(), true);
		}
		return true;
	}
	return false;
}

bool KGPUScene::TransformEntity(IKEntity* entity, const glm::mat4& transform)
{
	if (entity)
	{
		SceneEntityPtr sceneEntity = GetEntity(entity);
		if (sceneEntity)
		{
			sceneEntity->transform = transform;
		}
		return true;
	}
	return false;
}

bool KGPUScene::RemoveEntity(IKEntity* entity)
{
	auto it = m_EntityMap.find(entity);
	if (it != m_EntityMap.end())
	{
		uint32_t index = it->second->index;
		assert(index < m_Entities.size());

		SceneEntityPtr sceneEntity = m_Entities[index];
		for (KMaterialSubMeshPtr materialSubMesh : sceneEntity->materialSubMeshes)
		{
			RemoveSubMesh(materialSubMesh->GetSubMesh());
			RemoveMaterial(materialSubMesh->GetMaterial().Get());
		}

		for (uint32_t i = index + 1; i < m_Entities.size(); ++i)
		{
			--m_Entities[i]->index;
		}
		m_EntityMap.erase(it);
		m_Entities.erase(m_Entities.begin() + index);
	}
	return true;
}

void KGPUScene::RebuildEntitySubMeshAndMaterialIndex()
{
	for (SceneEntityPtr sceneEntity : m_Entities)
	{
		sceneEntity->subMeshIndices.resize(sceneEntity->materialSubMeshes.size());
		sceneEntity->materialIndices.resize(sceneEntity->materialSubMeshes.size());
		for (size_t i = 0; i < sceneEntity->materialSubMeshes.size(); ++i)
		{
			KMaterialSubMeshPtr materialSubMesh = sceneEntity->materialSubMeshes[i];
			sceneEntity->subMeshIndices[i] = CreateOrGetSubMeshIndex(materialSubMesh->GetSubMesh(), false);
			sceneEntity->materialIndices[i] = CreateOrGetMaterialIndex(materialSubMesh->GetMaterial().Get(), false);
		}
	}
}

void KGPUScene::RebuildMegaBuffer()
{
	uint32_t vertexBufferTotalSize[VF_COUNT] = { 0 };
	uint32_t vertexBufferWriteSize[VF_COUNT] = { 0 };
	uint32_t vertexBufferVertexCount[VF_COUNT] = { 0 };

	uint32_t indexBufferTotalSize = 0;
	uint32_t indexBufferWriteSize = 0;
	uint32_t indexBufferCount = 0;

	for (SubMeshItem& item : m_SubMeshes)
	{
		const KVertexData* vertexData = item.subMesh->GetVertexData();
		uint32_t vertexCount = vertexData->vertexCount;

		for (size_t index = 0; index < vertexData->vertexFormats.size(); ++index)
		{
			VertexFormat vertexFormat = vertexData->vertexFormats[index];
			const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail(vertexFormat);
			vertexBufferTotalSize[vertexFormat] += (uint32_t)(detail.vertexSize * vertexCount);
			vertexBufferVertexCount[vertexFormat] += vertexCount;
		}
		item.vertexCount = vertexCount;

		uint32_t indexCount = 0;
		KIndexData indexData;
		bool indexDraw = false;
		item.subMesh->GetIndexData(indexData, indexDraw);
		if (indexDraw)
		{
			indexCount = indexData.indexCount;
		}
		else
		{
			indexCount = vertexData->vertexCount;
		}
		item.indexCount = indexCount;
		indexBufferCount += indexCount;
	}
	indexBufferTotalSize = sizeof(uint32_t) * indexBufferCount;

	for (uint32_t vertexFormat = 0; vertexFormat < VF_COUNT; ++vertexFormat)
	{
		const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail((VertexFormat)vertexFormat);
		if (!m_MegaBuffer.vertexBuffers[vertexFormat] || m_MegaBuffer.vertexBuffers[vertexFormat]->GetBufferSize() != vertexBufferTotalSize[vertexFormat])
		{
			SAFE_UNINIT(m_MegaBuffer.vertexBuffers[vertexFormat]);
			KRenderGlobal::RenderDevice->CreateVertexBuffer(m_MegaBuffer.vertexBuffers[vertexFormat]);
			if (vertexBufferTotalSize[vertexFormat] > 0)
			{
				m_MegaBuffer.vertexBuffers[vertexFormat]->InitMemory(vertexBufferVertexCount[vertexFormat], detail.vertexSize, nullptr);
				m_MegaBuffer.vertexBuffers[vertexFormat]->InitDevice(false);
			}
			assert(m_MegaBuffer.vertexBuffers[vertexFormat]->GetBufferSize() == vertexBufferTotalSize[vertexFormat]);
		}
	}

	if (!m_MegaBuffer.indexBuffer || m_MegaBuffer.indexBuffer->GetBufferSize() != indexBufferTotalSize)
	{
		SAFE_UNINIT(m_MegaBuffer.indexBuffer);
		KRenderGlobal::RenderDevice->CreateIndexBuffer(m_MegaBuffer.indexBuffer);
		m_MegaBuffer.indexBuffer->InitMemory(IT_32, indexBufferCount, nullptr);
		m_MegaBuffer.indexBuffer->InitDevice(false);
		assert(m_MegaBuffer.indexBuffer->GetBufferSize() == indexBufferTotalSize);
	}

	for (uint32_t vertexFormat = 0; vertexFormat < VF_COUNT; ++vertexFormat)
	{
		IKVertexBufferPtr destVertexBuffer = m_MegaBuffer.vertexBuffers[vertexFormat];
		if (!destVertexBuffer)
		{
			continue;
		}

		void* pMegaVertexData = nullptr;
		destVertexBuffer->Map(&pMegaVertexData);

		const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail((VertexFormat)vertexFormat);

		for (SubMeshItem& item : m_SubMeshes)
		{
			const KVertexData* vertexData = item.subMesh->GetVertexData();
			uint32_t vertexStart = vertexData->vertexStart;
			uint32_t vertexCount = vertexData->vertexCount;

			for (size_t index = 0; index < vertexData->vertexFormats.size(); ++index)
			{
				if (vertexData->vertexFormats[index] == vertexFormat)
				{
					IKVertexBufferPtr srcVertexBuffer = vertexData->vertexBuffers[index];

					uint32_t destWriteOffset = vertexBufferWriteSize[vertexFormat];

					uint32_t srcVertexOffset = (uint32_t)(vertexStart * detail.vertexSize);
					uint32_t srcVertexSize = (uint32_t)(vertexCount * detail.vertexSize);

					item.vertexBufferOffset[vertexFormat] = destWriteOffset;
					item.vertexBufferSize[vertexFormat] = srcVertexSize;

					void* pSrcVertexData = nullptr;
					srcVertexBuffer->Map(&pSrcVertexData);
					pSrcVertexData = POINTER_OFFSET(pSrcVertexData, srcVertexOffset);

					void* pDestVertexData = nullptr;
					pDestVertexData = POINTER_OFFSET(pMegaVertexData, destWriteOffset);

					memcpy(pDestVertexData, pSrcVertexData, srcVertexSize);

					srcVertexBuffer->UnMap();
					vertexBufferWriteSize[vertexFormat] += item.vertexBufferSize[vertexFormat];

					break;
				}
			}
		}

		destVertexBuffer->UnMap();
		assert(vertexBufferTotalSize[vertexFormat] == vertexBufferWriteSize[vertexFormat]);
	}

	{
		void* pMegaIndexData = nullptr;
		m_MegaBuffer.indexBuffer->Map(&pMegaIndexData);

		void* pDestIndexData = pMegaIndexData;

		for (SubMeshItem& item : m_SubMeshes)
		{
			item.indexBufferOffset = indexBufferWriteSize;

			KIndexData indexData;
			bool indexDraw;
			item.subMesh->GetIndexData(indexData, indexDraw);
			if (indexDraw)
			{
				uint32_t indexStart = indexData.indexStart;
				uint32_t indexCount = indexData.indexCount;

				void* pSrcIndexData = nullptr;
				indexData.indexBuffer->Map(&pSrcIndexData);

				if (indexData.indexBuffer->GetIndexType() == IT_16)
				{
					pSrcIndexData = POINTER_OFFSET(pSrcIndexData, indexStart * sizeof(uint16_t));
				}
				else
				{
					pSrcIndexData = POINTER_OFFSET(pSrcIndexData, indexStart * sizeof(uint32_t));
				}

				for (uint32_t i = 0; i < indexCount; ++i)
				{
					if (indexData.indexBuffer->GetIndexType() == IT_16)
					{
						*(uint32_t*)pDestIndexData = *(uint16_t*)pSrcIndexData;
						pSrcIndexData = POINTER_OFFSET(pSrcIndexData, sizeof(uint16_t));
					}
					else
					{
						*(uint32_t*)pDestIndexData = *(uint32_t*)pSrcIndexData;
						pSrcIndexData = POINTER_OFFSET(pSrcIndexData, sizeof(uint32_t));
					}
					pDestIndexData = POINTER_OFFSET(pDestIndexData, sizeof(uint32_t));
					indexBufferWriteSize += sizeof(uint32_t);
				}
				indexData.indexBuffer->UnMap();
			}
			else
			{
				const KVertexData* vertexData = item.subMesh->GetVertexData();
				uint32_t vertexStart = vertexData->vertexStart;
				uint32_t vertexCount = vertexData->vertexCount;

				for (uint32_t i = 0; i < vertexCount; ++i)
				{
					*(uint32_t*)pDestIndexData = vertexStart + i;
					pDestIndexData = POINTER_OFFSET(pDestIndexData, sizeof(uint32_t));
					indexBufferWriteSize += sizeof(uint32_t);
				}
			}

			item.indexBufferSize = indexBufferWriteSize - item.indexBufferOffset;
		}

		m_MegaBuffer.indexBuffer->UnMap();
		assert(indexBufferTotalSize == indexBufferWriteSize);
	}
}

void KGPUScene::RebuildTextureArray()
{
	struct TextureArrayCreation
	{
		std::vector<IKTexturePtr> textures;
		std::unordered_map<IKTexturePtr, uint32_t> textureLocation;
	} creationInfos[MAX_MATERIAL_TEXTURE_BINDING];

	for (MaterialItem& item : m_Materials)
	{
		IKMaterialPtr material = item.material;
		IKMaterialTextureBindingPtr textureBinding = material->GetTextureBinding();
		uint32_t slots = textureBinding->GetNumSlot();
		assert(slots <= MAX_MATERIAL_TEXTURE_BINDING);
		for (uint32_t i = 0; i < slots; ++i)
		{
			TextureArrayCreation& creationInfo = creationInfos[i];
			IKTexturePtr texture = textureBinding->GetTexture(i);
			if (texture)
			{
				auto it = creationInfo.textureLocation.find(texture);
				if (it == creationInfo.textureLocation.end())
				{
					uint32_t location = (uint32_t)creationInfo.textures.size();
					creationInfo.textures.push_back(texture);
					creationInfo.textureLocation.insert({ texture, location });
				}
				else
				{
					creationInfo.textureLocation.insert({ texture, it->second });
				}
			}
		}
	}

	for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
	{
		const TextureArrayCreation& creationInfo = creationInfos[i];
		if (!m_TextureArrays[i] || m_TextureArrays[i]->GetSlice() < (uint32_t)creationInfo.textures.size())
		{
			SAFE_UNINIT(m_TextureArrays[i]);
			KRenderGlobal::RenderDevice->CreateTexture(m_TextureArrays[i]);
			if (creationInfo.textures.size() > 0)
			{
				m_TextureArrays[i]->InitMemoryFrom2DArray("GPUSceneTextureArray_" + std::to_string(i), 2048, 2048, (uint32_t)creationInfo.textures.size(), IF_R8G8B8A8, true);
				m_TextureArrays[i]->InitDevice(false);
			}
		}
		for (uint32_t sliceIndex = 0; sliceIndex < (uint32_t)creationInfo.textures.size(); ++sliceIndex)
		{
			m_TextureArrays[i]->CopyFromFrameBufferToSlice(creationInfo.textures[sliceIndex]->GetFrameBuffer(), sliceIndex);
		}
	}
}

void KGPUScene::OnSceneChanged(EntitySceneOp op, IKEntity* entity)
{
	KRenderComponent* renderComponent = nullptr;
	KTransformComponent* transformComponent = nullptr;

	ASSERT_RESULT(entity->GetComponent(CT_RENDER, &renderComponent));
	ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));

	if (op == ESO_ADD)
	{
		const glm::mat4& transform = transformComponent->GetFinal();
		renderComponent->RegisterCallback(&m_OnRenderComponentChangedFunc);
		const std::vector<KMaterialSubMeshPtr>& subMeshes = renderComponent->GetMaterialSubMeshs();
		if (subMeshes.size() > 0)
		{
			AddEntity(entity, transform, subMeshes);
		}
	}
	if (op == ESO_REMOVE)
	{
		RemoveEntity(entity);
		renderComponent->UnRegisterCallback(&m_OnRenderComponentChangedFunc);
	}
	else if (op == ESO_TRANSFORM)
	{
		const glm::mat4& transform = transformComponent->GetFinal();
		TransformEntity(entity, transform);
	}
}

void KGPUScene::OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init)
{
}

bool KGPUScene::Init(IKRenderScene* scene, const KCamera* camera)
{
	UnInit();
	if (scene && camera)
	{
		m_Scene = scene;
		m_Camera = camera;

		m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);
	}
	return true;
}

bool KGPUScene::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
		m_Scene = nullptr;
	}

	m_SubMeshes.clear();
	m_SubMeshToIndex.clear();

	m_Materials.clear();
	m_MaterialToIndex.clear();

	m_EntityMap.clear();
	m_Entities.clear();

	m_SceneDirty = false;

	for (uint32_t i = 0; i < VF_COUNT; ++i)
	{
		SAFE_UNINIT(m_MegaBuffer.vertexBuffers[i]);
	}
	SAFE_UNINIT(m_MegaBuffer.indexBuffer);

	for (uint32_t slot = 0; slot < MAX_MATERIAL_TEXTURE_BINDING; ++slot)
	{
		SAFE_UNINIT(m_TextureArrays[slot]);
	}

	return true;
}

bool KGPUScene::Execute(IKCommandBufferPtr primaryBuffer)
{
	if (m_SceneDirty)
	{
		RebuildEntitySubMeshAndMaterialIndex();
		RebuildMegaBuffer();
		RebuildTextureArray();
		m_SceneDirty = false;
	}

	return true;
}

void KGPUScene::ReloadShader()
{
}