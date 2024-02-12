#include "KGPUScene.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/KRenderGlobal.h"

#define COMPUTE_HASH_CONSTRUCT_MEGA_BUFFER 0

KGPUScene::KGPUScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_DataDirtyBits(0)
{
#define GPUSCENE_BINDING_TO_STR(x) #x

#define GPUSCENE_BINDING(SEMANTIC) m_ComputeBindingEnv.macros.push_back( {GPUSCENE_BINDING_TO_STR(GPUSCENE_BINDING_##SEMANTIC), std::to_string(GPUSCENE_BINDING_##SEMANTIC) });
#include "KGPUSceneBinding.inl"
#undef GPUSCENE_BINDING

#undef GPUSCENE_BINDING_TO_STR

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
	else if (create)
	{
		m_DataDirtyBits |= MEGA_BUFFER_DIRTY;
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
	else if (create)
	{
		m_DataDirtyBits |= TEXTURE_ARRAY_DIRTY;
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

uint64_t KGPUScene::ComputeHashByMaterialSubMesh(KSubMeshPtr subMesh, IKMaterialPtr material)
{
	uint64_t hash = 0;

	KHash::HashCombine(hash, material->GetVSInformation()->Hash());
	KHash::HashCombine(hash, material->GetFSInformation()->Hash());

	const KVertexData* vertexData = subMesh->GetVertexData();
	for (VertexFormat format : vertexData->vertexFormats)
	{
		KHash::HashCombine(hash, format);
	}

	const IKMaterialTextureBindingPtr textureBinding = material->GetTextureBinding();
	for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
	{
		if (textureBinding->GetTexture(i))
		{
			KHash::HashCombine(hash, 0xFF);
		}
		else
		{
			KHash::HashCombine(hash, 0xAF);
		}
	}

	return hash;
}

uint32_t KGPUScene::CreateOrGetMegaShaderIndex(KSubMeshPtr subMesh, IKMaterialPtr material, bool create)
{
	uint64_t hash = ComputeHashByMaterialSubMesh(subMesh, material);

	uint32_t index = -1;
	auto it = m_MegaShaderToIndex.find(hash);
	if (it != m_MegaShaderToIndex.end())
	{
		index = it->second;
		m_MegaShaders[index].refCount += create;
	}
	else if (create)
	{
		m_DataDirtyBits |= MEGA_SHADER_DIRTY;
		index = (uint32_t)m_MegaShaders.size();

		const KVertexData* vertexData = subMesh->GetVertexData();
		const KShaderInformation::Constant* constantInfo = material->GetShadingInfo();

		MegaShaderItem newItem;
		newItem.refCount = 1;
		newItem.vsShader = material->GetVSGPUSceneShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		newItem.fsShader = material->GetFSGPUSceneShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		newItem.parameterDataSize = constantInfo->size;

		m_MegaShaders.push_back(newItem);
		m_MegaShaderToIndex[hash] = index;
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
		m_DataDirtyBits |= MEGA_BUFFER_DIRTY;
		for (auto& pair : m_SubMeshToIndex)
		{
			if (pair.second > removeIndex)
			{
				pair.second -= 1;
			}
		}
		m_SubMeshes.erase(m_SubMeshes.begin() + removeIndex);
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
		m_DataDirtyBits |= TEXTURE_ARRAY_DIRTY;
		for (auto& pair : m_MaterialToIndex)
		{
			if (pair.second > removeIndex)
			{
				pair.second -= 1;
			}
		}
		m_Materials.erase(m_Materials.begin() + removeIndex);
	}
}

void KGPUScene::RemoveMegaShader(KSubMeshPtr subMesh, IKMaterialPtr material)
{
	uint64_t hash = ComputeHashByMaterialSubMesh(subMesh, material);

	uint32_t removeIndex = -1;
	auto it = m_MegaShaderToIndex.find(hash);
	if (it != m_MegaShaderToIndex.end())
	{
		uint32_t index = it->second;
		m_MegaShaders[index].refCount -= 1;
		if (m_MegaShaders[index].refCount == 0)
		{
			m_MegaShaderToIndex.erase(it);
			removeIndex = index;
		}
	}
	if (removeIndex != -1)
	{
		m_DataDirtyBits |= MEGA_SHADER_DIRTY;
		for (auto& pair : m_MegaShaderToIndex)
		{
			if (pair.second > removeIndex)
			{
				pair.second -= 1;
			}
		}
		SAFE_UNINIT_CONTAINER(m_MegaShaders[removeIndex].parametersBuffers);
		m_MegaShaders.erase(m_MegaShaders.begin() + removeIndex);
	}
}

bool KGPUScene::AddEntity(IKEntity* entity, const KAABBBox& localBound, const glm::mat4& transform, const std::vector<KMaterialSubMeshPtr>& subMeshes)
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
			CreateOrGetMegaShaderIndex(materialSubMesh->GetSubMesh(), materialSubMesh->GetMaterial().Get(), true);
		}
		sceneEntity->localBound = localBound;
		m_DataDirtyBits |= INSTANCE_DIRTY;
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
			sceneEntity->prevTransform = transform;
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
			RemoveMegaShader(materialSubMesh->GetSubMesh(), materialSubMesh->GetMaterial().Get());
		}

		for (uint32_t i = index + 1; i < m_Entities.size(); ++i)
		{
			--m_Entities[i]->index;
		}
		m_EntityMap.erase(it);
		m_Entities.erase(m_Entities.begin() + index);
		m_DataDirtyBits |= INSTANCE_DIRTY;
	}
	return true;
}

void KGPUScene::RebuildEntityDataIndex()
{
	for (MegaShaderItem& megaShader : m_MegaShaders)
	{
		megaShader.materialIndices.clear();
	}

	for (SceneEntityPtr sceneEntity : m_Entities)
	{
		sceneEntity->subMeshIndices.resize(sceneEntity->materialSubMeshes.size());
		sceneEntity->materialIndices.resize(sceneEntity->materialSubMeshes.size());
		sceneEntity->megaShaderIndices.resize(sceneEntity->materialSubMeshes.size());
		sceneEntity->shaderLocalIndices.resize(sceneEntity->materialSubMeshes.size());

		for (size_t i = 0; i < sceneEntity->materialSubMeshes.size(); ++i)
		{
			KMaterialSubMeshPtr materialSubMesh = sceneEntity->materialSubMeshes[i];
			sceneEntity->subMeshIndices[i] = CreateOrGetSubMeshIndex(materialSubMesh->GetSubMesh(), false);
			sceneEntity->materialIndices[i] = CreateOrGetMaterialIndex(materialSubMesh->GetMaterial().Get(), false);
			sceneEntity->megaShaderIndices[i] = CreateOrGetMegaShaderIndex(materialSubMesh->GetSubMesh(), materialSubMesh->GetMaterial().Get(), false);
			// Modify mega shader material index
			MegaShaderItem& megaShader = m_MegaShaders[sceneEntity->megaShaderIndices[i]];
			sceneEntity->shaderLocalIndices[i] = (uint32_t)megaShader.materialIndices.size();
			megaShader.materialIndices.push_back(sceneEntity->materialIndices[i]);
		}
	}
}

void KGPUScene::RebuildMegaBuffer()
{
	std::unordered_map<uint64_t, uint32_t> vertexBufferOffset[VF_COUNT];
	std::unordered_set<uint64_t> vertexBufferWrite[VF_COUNT];
	uint32_t vertexBufferTotalSize[VF_COUNT] = { 0 };
	uint32_t vertexBufferWriteSize[VF_COUNT] = { 0 };
	uint32_t vertexBufferVertexCount[VF_COUNT] = { 0 };

	std::unordered_map<uint64_t, uint32_t> indexBufferOffset;
	std::unordered_set<uint64_t> indexBufferWrite;
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
			IKVertexBufferPtr vertexBuffer = vertexData->vertexBuffers[index];

			const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail(vertexFormat);

			uint64_t hash = 0;
#if COMPUTE_HASH_CONSTRUCT_MEGA_BUFFER
			void* pData = nullptr;
			vertexBuffer->Map(&pData);
			pData = POINTER_OFFSET(pData, vertexData->vertexStart * detail.vertexSize);
			hash = KHash::BKDR((const char*)pData, vertexData->vertexCount * detail.vertexSize);
			vertexBuffer->UnMap();
#else
			hash = (uint64_t)vertexBuffer.get();
#endif

			auto it = vertexBufferOffset[vertexFormat].find(hash);
			if (it == vertexBufferOffset[vertexFormat].end())
			{
				vertexBufferOffset[vertexFormat].insert({ hash, vertexBufferTotalSize[vertexFormat] });
				vertexBufferTotalSize[vertexFormat] += (uint32_t)(vertexData->vertexCount * detail.vertexSize);
				vertexBufferVertexCount[vertexFormat] += vertexCount;
			}
		}

		uint32_t indexCount = 0;
		KIndexData indexData;
		bool indexDraw = false;
		item.subMesh->GetIndexData(indexData, indexDraw);
		{
			uint64_t hash = 0;

			if (indexDraw)
			{
#if COMPUTE_HASH_CONSTRUCT_MEGA_BUFFER
				void* pData = nullptr;
				indexData.indexBuffer->Map(&pData);
				pData = POINTER_OFFSET(pData, indexData.indexStart * ((indexData.indexBuffer->GetIndexType() == IT_16) ? sizeof(uint16_t) : sizeof(uint32_t)));
				hash = KHash::BKDR((const char*)pData, indexData.indexCount * ((indexData.indexBuffer->GetIndexType() == IT_16) ? sizeof(uint16_t) : sizeof(uint32_t)));
				indexData.indexBuffer->UnMap();
#else
				hash = (uint64_t)indexData.indexBuffer.get();
#endif
				indexCount = indexData.indexCount;
			}
			else
			{
				std::vector<uint32_t> indices;
				indices.reserve(vertexData->vertexCount);
				for (uint32_t i = 0; i < vertexData->vertexCount; ++i)
				{
					indices.push_back(i + vertexData->vertexStart);
				}
				hash = KHash::BKDR((const char*)indices.data(), vertexData->vertexCount * sizeof(uint32_t));

				indexCount = vertexData->vertexCount;
			}

			auto it = indexBufferOffset.find(hash);
			if (it == indexBufferOffset.end())
			{
				indexBufferOffset.insert({ hash, indexBufferTotalSize });
				indexBufferTotalSize += indexCount * sizeof(uint32_t);
				indexBufferCount += indexCount;
			}
		}

		// Assign vertexCount and indexCount
		item.vertexCount = vertexCount;
		item.indexCount = indexCount;
	}

	for (uint32_t vertexFormat = 0; vertexFormat < VF_COUNT; ++vertexFormat)
	{
		const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail((VertexFormat)vertexFormat);
		if (m_MegaBuffer.vertexBuffers[vertexFormat]->GetBufferSize() != vertexBufferTotalSize[vertexFormat])
		{
			m_MegaBuffer.vertexBuffers[vertexFormat]->UnInit();
			if (vertexBufferTotalSize[vertexFormat] > 0)
			{
				m_MegaBuffer.vertexBuffers[vertexFormat]->InitMemory(vertexBufferTotalSize[vertexFormat], nullptr);
				m_MegaBuffer.vertexBuffers[vertexFormat]->InitDevice(false, false);
				m_MegaBuffer.vertexBuffers[vertexFormat]->SetDebugName((std::string("GPUSceneVertexData_") + detail.name).c_str());
			}
			assert(m_MegaBuffer.vertexBuffers[vertexFormat]->GetBufferSize() == vertexBufferTotalSize[vertexFormat]);
		}
	}

	if (m_MegaBuffer.indexBuffer->GetBufferSize() != indexBufferTotalSize)
	{
		m_MegaBuffer.indexBuffer->UnInit();
		if (indexBufferCount > 0)
		{
			m_MegaBuffer.indexBuffer->InitMemory(indexBufferTotalSize, nullptr);
			m_MegaBuffer.indexBuffer->InitDevice(false, false);
			m_MegaBuffer.indexBuffer->SetDebugName("GPUSceneIndexData");
		}
		assert(m_MegaBuffer.indexBuffer->GetBufferSize() == indexBufferTotalSize);
	}

	for (uint32_t vertexFormat = 0; vertexFormat < VF_COUNT; ++vertexFormat)
	{
		IKStorageBufferPtr destVertexBuffer = m_MegaBuffer.vertexBuffers[vertexFormat];
		if (destVertexBuffer->GetBufferSize() == 0)
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
				if (vertexData->vertexFormats[index] != vertexFormat)
				{
					continue;
				}

				IKVertexBufferPtr srcVertexBuffer = vertexData->vertexBuffers[index];

				uint64_t hash = 0;

				uint32_t srcVertexOffset = (uint32_t)(vertexStart * detail.vertexSize);
				uint32_t srcVertexSize = (uint32_t)(vertexCount * detail.vertexSize);

				uint32_t destWriteOffset = 0;

				void* pSrcVertexData = nullptr;
				srcVertexBuffer->Map(&pSrcVertexData);
#if COMPUTE_HASH_CONSTRUCT_MEGA_BUFFER
				pSrcVertexData = POINTER_OFFSET(pSrcVertexData, srcVertexOffset);
				hash = KHash::BKDR((const char*)pSrcVertexData, srcVertexSize);
#else
				hash = (uint64_t)srcVertexBuffer.get();
#endif
				// Find the location to write
				auto it = vertexBufferOffset[vertexFormat].find(hash);
				assert(it != vertexBufferOffset[vertexFormat].end());
				if (it != vertexBufferOffset[vertexFormat].end())
				{
					destWriteOffset = it->second;
				}

				// Copy only when necessary
				if (vertexBufferWrite[vertexFormat].find(hash) == vertexBufferWrite[vertexFormat].end())
				{
					void* pDestVertexData = nullptr;
					pDestVertexData = POINTER_OFFSET(pMegaVertexData, destWriteOffset);
					memcpy(pDestVertexData, pSrcVertexData, srcVertexSize);
					vertexBufferWrite[vertexFormat].insert(hash);
					vertexBufferWriteSize[vertexFormat] += srcVertexSize;
				}

				srcVertexBuffer->UnMap();
				
				// Assign vertexBufferOffset and vertexBufferSize
				item.vertexBufferOffset[vertexFormat] = destWriteOffset;
				item.vertexBufferSize[vertexFormat] = srcVertexSize;

				break;
			}
		}

		destVertexBuffer->UnMap();
		assert(vertexBufferTotalSize[vertexFormat] == vertexBufferWriteSize[vertexFormat]);
	}

	{
		void* pMegaIndexData = nullptr;
		m_MegaBuffer.indexBuffer->Map(&pMegaIndexData);

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

				uint64_t hash = 0;
#if COMPUTE_HASH_CONSTRUCT_MEGA_BUFFER
				if (indexData.indexBuffer->GetIndexType() == IT_16)
				{
					hash = KHash::BKDR((const char*)pSrcIndexData, indexCount * sizeof(uint16_t));
				}
				else
				{
					hash = KHash::BKDR((const char*)pSrcIndexData, indexCount * sizeof(uint32_t));
				}
#else
				hash = (uint64_t)indexData.indexBuffer.get();
#endif

				uint32_t destWriteOffset = 0;

				// Find the location to write
				auto it = indexBufferOffset.find(hash);
				assert(it != indexBufferOffset.end());
				if (it != indexBufferOffset.end())
				{
					destWriteOffset = it->second;
				}

				// Copy only when necessary
				if (indexBufferWrite.find(hash) == indexBufferWrite.end())
				{
					void* pDestIndexData = pMegaIndexData;
					pDestIndexData = POINTER_OFFSET(pDestIndexData, destWriteOffset);

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
					indexBufferWrite.insert(hash);
				}
				indexData.indexBuffer->UnMap();

				item.indexBufferSize = indexCount * sizeof(uint32_t);
			}
			else
			{
				const KVertexData* vertexData = item.subMesh->GetVertexData();
				uint32_t vertexStart = vertexData->vertexStart;
				uint32_t vertexCount = vertexData->vertexCount;
	
				std::vector<uint32_t> indices;
				indices.reserve(vertexData->vertexCount);
				for (uint32_t i = 0; i < vertexData->vertexCount; ++i)
				{
					indices.push_back(i + vertexData->vertexStart);
				}
				uint32_t hash = KHash::BKDR((const char*)indices.data(), vertexData->vertexCount * sizeof(uint32_t));

				uint32_t destWriteOffset = 0;
				// Find the location to write
				auto it = indexBufferOffset.find(hash);
				assert(it != indexBufferOffset.end());
				if (it != indexBufferOffset.end())
				{
					destWriteOffset = it->second;
				}

				void* pDestIndexData = pMegaIndexData;
				pDestIndexData = POINTER_OFFSET(pDestIndexData, destWriteOffset);

				// Copy only when necessary
				if (indexBufferWrite.find(hash) == indexBufferWrite.end())
				{
					for (uint32_t i = 0; i < vertexCount; ++i)
					{
						*(uint32_t*)pDestIndexData = vertexStart + i;
						pDestIndexData = POINTER_OFFSET(pDestIndexData, sizeof(uint32_t));
						indexBufferWriteSize += sizeof(uint32_t);
					}
					indexBufferWrite.insert(hash);
				}

				item.indexBufferSize = vertexCount * sizeof(uint32_t);
			}
		}

		m_MegaBuffer.indexBuffer->UnMap();
		assert(indexBufferTotalSize == indexBufferWriteSize);
	}
}

void KGPUScene::RebuildMegaShaderState()
{
	for (size_t i = 0; i < m_MegaShaderStateBuffers.size(); ++i)
	{
		m_MegaShaderStateBuffers[i]->UnInit();
		m_MegaShaderStateBuffers[i]->InitMemory(sizeof(KGPUSceneMegaShaderState) * m_MegaShaders.size(), nullptr);
		m_MegaShaderStateBuffers[i]->InitDevice(false, false);
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

void KGPUScene::RebuildInstance()
{
	m_Instances.clear();

	for (SceneEntityPtr entity : m_Entities)
	{
		KGPUSceneInstance newInstance;

		newInstance.boundCenter = glm::vec4(entity->localBound.GetCenter(), 0);
		newInstance.boundHalfExtend = glm::vec4(entity->localBound.GetExtend() * 0.5f, 0);
		newInstance.prevTransform = entity->prevTransform;
		newInstance.transform = entity->transform;
	
		for (size_t i = 0; i < entity->materialSubMeshes.size(); ++i)
		{
			newInstance.miscs[0] = entity->index;
			newInstance.miscs[1] = entity->subMeshIndices[i];
			newInstance.miscs[2] = entity->megaShaderIndices[i];
			newInstance.miscs[3] = entity->shaderLocalIndices[i];

			m_Instances.push_back(newInstance);
		}
	}

	for (size_t i = 0; i < m_InstanceDataBuffers.size(); ++i)
	{
		IKStorageBufferPtr buffer = m_InstanceDataBuffers[i];
		buffer->UnInit();
		buffer->InitMemory(m_Instances.size() * sizeof(KGPUSceneInstance), m_Instances.data());
		buffer->InitDevice(false, false);
	}

	for (size_t i = 0; i < m_GroupDataBuffers.size(); ++i)
	{
		IKStorageBufferPtr buffer = m_GroupDataBuffers[i];
		buffer->UnInit();
		buffer->InitMemory(m_Instances.size() * sizeof(uint32_t), nullptr);
		buffer->InitDevice(false, false);
	}

	for (size_t i = 0; i < m_InstanceCullResultBuffers.size(); ++i)
	{
		IKStorageBufferPtr buffer = m_InstanceCullResultBuffers[i];
		buffer->UnInit();
		buffer->InitMemory(sizeof(uint32_t) * m_Instances.size(), nullptr);
		buffer->InitDevice(false, false);
	}
}

void KGPUScene::RebuildParameter()
{
	for (MegaShaderItem& shaderItem : m_MegaShaders)
	{
		if (shaderItem.parameterDataCount < shaderItem.refCount)
		{
			shaderItem.parameterDataCount = KMath::SmallestPowerOf2GreaterEqualThan(shaderItem.refCount);
			if (shaderItem.parametersBuffers.size() == 0)
			{
				shaderItem.parametersBuffers.resize(KRenderGlobal::NumFramesInFlight);
				for (uint32_t i = 0; i < KRenderGlobal::NumFramesInFlight; ++i)
				{
					KRenderGlobal::RenderDevice->CreateStorageBuffer(shaderItem.parametersBuffers[i]);
				}
			}
			for (size_t i = 0; i < shaderItem.parametersBuffers.size(); ++i)
			{
				shaderItem.parametersBuffers[i]->UnInit();
				shaderItem.parametersBuffers[i]->InitMemory(shaderItem.parameterDataSize * shaderItem.parameterDataCount, nullptr);
				shaderItem.parametersBuffers[i]->InitDevice(false, false);
			}
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
		KAABBBox localBound;
		entity->GetLocalBound(localBound);
		renderComponent->RegisterCallback(&m_OnRenderComponentChangedFunc);
		const std::vector<KMaterialSubMeshPtr>& subMeshes = renderComponent->GetMaterialSubMeshs();
		if (subMeshes.size() > 0)
		{
			AddEntity(entity, localBound, transform, subMeshes);
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

void KGPUScene::InitializeBuffers()
{
	uint32_t numFrames = KRenderGlobal::NumFramesInFlight;

	m_SceneStateBuffers.resize(numFrames);
	m_InstanceDataBuffers.resize(numFrames);
	m_GroupDataBuffers.resize(numFrames);
	m_IndirectArgsBuffers.resize(numFrames);
	m_InstanceCullResultBuffers.resize(numFrames);
	m_MegaShaderStateBuffers.resize(numFrames);

	for (uint32_t i = 0; i < numFrames; ++i)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_SceneStateBuffers[i]);
		m_SceneStateBuffers[i]->SetDebugName((std::string("GPUSceneStateData_") + std::to_string(i)).c_str());
		m_SceneStateBuffers[i]->InitMemory(sizeof(KGPUSceneState), nullptr);
		m_SceneStateBuffers[i]->InitDevice(false, false);

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_InstanceDataBuffers[i]);
		m_InstanceDataBuffers[i]->InitMemory(sizeof(KGPUSceneInstance), nullptr);
		m_InstanceDataBuffers[i]->InitDevice(false, false);
		m_InstanceDataBuffers[i]->SetDebugName((std::string("GPUSceneInstanceData_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_GroupDataBuffers[i]);
		m_GroupDataBuffers[i]->InitMemory(sizeof(uint32_t), nullptr);
		m_GroupDataBuffers[i]->InitDevice(false, false);
		m_GroupDataBuffers[i]->SetDebugName((std::string("GPUSceneGroupData_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_IndirectArgsBuffers[i]);
		m_IndirectArgsBuffers[i]->InitMemory(sizeof(glm::uvec4), nullptr);
		m_IndirectArgsBuffers[i]->InitDevice(true, false);
		m_IndirectArgsBuffers[i]->SetDebugName((std::string("GPUSceneIndirectArgs_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_InstanceCullResultBuffers[i]);
		m_InstanceCullResultBuffers[i]->InitMemory(sizeof(uint32_t), nullptr);
		m_InstanceCullResultBuffers[i]->InitDevice(false, false);
		m_InstanceCullResultBuffers[i]->SetDebugName((std::string("GPUSceneInstanceCullResult_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MegaShaderStateBuffers[i]);
		m_MegaShaderStateBuffers[i]->InitMemory(sizeof(KGPUSceneMegaShaderState), nullptr);
		m_MegaShaderStateBuffers[i]->InitDevice(false, false);
		m_MegaShaderStateBuffers[i]->SetDebugName((std::string("GPUSceneMegaShaderState_") + std::to_string(i)).c_str());
	}

	for (uint32_t vertexFormat = 0; vertexFormat < VF_COUNT; ++vertexFormat)
	{
		const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail((VertexFormat)vertexFormat);
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MegaBuffer.vertexBuffers[vertexFormat]);
		m_MegaBuffer.vertexBuffers[vertexFormat]->SetDebugName((std::string("GPUSceneVertexData_") + detail.name).c_str());
	}

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MegaBuffer.indexBuffer);
	m_MegaBuffer.indexBuffer->SetDebugName("GPUSceneIndexData");
}

void KGPUScene::InitializePipelines()
{
	uint32_t numFrames = KRenderGlobal::NumFramesInFlight;

	m_InitStatePipelines.resize(numFrames);
	m_InstanceCullPipelines.resize(numFrames);
	m_GroupAllocatePipelines.resize(numFrames);
	m_GroupScatterPipelines.resize(numFrames);

	for (uint32_t i = 0; i < numFrames; ++i)
	{
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitStatePipelines[i]);
		m_InitStatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InitStatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InitStatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INDIRECT_ARGS, m_IndirectArgsBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InitStatePipelines[i]->Init("gpuscene/init_state.comp", m_ComputeBindingEnv);

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_InstanceCullPipelines[i]);
		m_InstanceCullPipelines[i]->BindDynamicUniformBuffer(GPUSCENE_BINDING_OBJECT);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_DATA, m_InstanceDataBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_CULL_RESULT, m_InstanceCullResultBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INDIRECT_ARGS, m_IndirectArgsBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->Init("gpuscene/instance_cull.comp", m_ComputeBindingEnv);

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_GroupAllocatePipelines[i]);
		m_GroupAllocatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_GroupAllocatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_GroupAllocatePipelines[i]->Init("gpuscene/group_allocate.comp", m_ComputeBindingEnv);

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_GroupScatterPipelines[i]);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_DATA, m_InstanceDataBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_CULL_RESULT, m_InstanceCullResultBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_GROUP_DATA, m_GroupDataBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_GroupScatterPipelines[i]->Init("gpuscene/group_scatter.comp", m_ComputeBindingEnv);
	}
}

bool KGPUScene::Init(IKRenderScene* scene, const KCamera* camera)
{
	UnInit();
	if (scene && camera)
	{
		InitializeBuffers();
		InitializePipelines();

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

	m_Materials.clear();
	m_MaterialToIndex.clear();

	for (MegaShaderItem& megaShader : m_MegaShaders)
	{
		SAFE_UNINIT_CONTAINER(megaShader.parametersBuffers);
	}

	m_MegaShaders.clear();
	m_MegaShaderToIndex.clear();

	m_EntityMap.clear();
	m_Entities.clear();

	m_DataDirtyBits = 0;

	for (uint32_t i = 0; i < VF_COUNT; ++i)
	{
		SAFE_UNINIT(m_MegaBuffer.vertexBuffers[i]);
	}
	SAFE_UNINIT(m_MegaBuffer.indexBuffer);

	for (uint32_t slot = 0; slot < MAX_MATERIAL_TEXTURE_BINDING; ++slot)
	{
		SAFE_UNINIT(m_TextureArrays[slot]);
	}

	SAFE_UNINIT_CONTAINER(m_SceneStateBuffers);
	SAFE_UNINIT_CONTAINER(m_InstanceDataBuffers);
	SAFE_UNINIT_CONTAINER(m_GroupDataBuffers);
	SAFE_UNINIT_CONTAINER(m_IndirectArgsBuffers);
	SAFE_UNINIT_CONTAINER(m_InstanceCullResultBuffers);
	SAFE_UNINIT_CONTAINER(m_MegaShaderStateBuffers);

	SAFE_UNINIT_CONTAINER(m_InitStatePipelines);
	SAFE_UNINIT_CONTAINER(m_InstanceCullPipelines);
	SAFE_UNINIT_CONTAINER(m_GroupAllocatePipelines);
	SAFE_UNINIT_CONTAINER(m_GroupScatterPipelines);

	return true;
}

void KGPUScene::RebuildDirtyBuffer()
{
	if (m_DataDirtyBits)
	{
		KRenderGlobal::RenderDevice->Wait();
		RebuildEntityDataIndex();
	}
	if (m_DataDirtyBits & MEGA_BUFFER_DIRTY)
	{
		RebuildMegaBuffer();
	}
	if (m_DataDirtyBits & MEGA_SHADER_DIRTY)
	{
		RebuildMegaShaderState();
	}
	if (m_DataDirtyBits & TEXTURE_ARRAY_DIRTY)
	{
		RebuildTextureArray();
	}
	if (m_DataDirtyBits & INSTANCE_DIRTY)
	{
		RebuildInstance();
		RebuildParameter();
	}
	m_DataDirtyBits = 0;
}

void KGPUScene::UpdateInstanceDataBuffer()
{
	uint32_t frameIndex = KRenderGlobal::CurrentInFlightFrameIndex;
	IKStorageBufferPtr instanceDataBuffer = m_InstanceDataBuffers[frameIndex];
	assert(instanceDataBuffer->GetBufferSize() == m_Instances.size() * sizeof(KGPUSceneInstance));

	for (KGPUSceneInstance& instance : m_Instances)
	{
		uint32_t entityIndex = instance.miscs[0];
		SceneEntityPtr sceneEntity = m_Entities[entityIndex];
		instance.prevTransform = sceneEntity->prevTransform;
		instance.transform = sceneEntity->transform;
	}
	
	instanceDataBuffer->Write(m_Instances.data());
}

bool KGPUScene::Execute(IKCommandBufferPtr primaryBuffer)
{
	RebuildDirtyBuffer();
	UpdateInstanceDataBuffer();

	uint32_t currentFrameIndex = KRenderGlobal::CurrentInFlightFrameIndex;
	uint32_t megaShaderNum = (uint32_t)m_MegaShaders.size();

	primaryBuffer->BeginDebugMarker("GPUScene", glm::vec4(1));

	{
		KGPUSceneState sceneState;
		m_SceneStateBuffers[currentFrameIndex]->Read(&sceneState);
		sceneState.megaShaderNum = megaShaderNum;
		m_SceneStateBuffers[currentFrameIndex]->Write(&sceneState);

		uint32_t numDispatch = ((uint32_t)megaShaderNum + GPUSCENE_GROUP_SIZE - 1) / GPUSCENE_GROUP_SIZE;

		primaryBuffer->BeginDebugMarker("GPUScene_InitState", glm::vec4(1));
		m_InitStatePipelines[currentFrameIndex]->Execute(primaryBuffer, numDispatch, 1, 1, nullptr);
		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("GPUScene_InstanceCull", glm::vec4(1));
		uint32_t numDispatch = ((uint32_t)m_Instances.size() + GPUSCENE_GROUP_SIZE - 1) / GPUSCENE_GROUP_SIZE;

		struct
		{
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewProj;
		} cameraObjectUsage;
		static_assert((sizeof(cameraObjectUsage) % 16) == 0, "Size must be a multiple of 16");

		cameraObjectUsage.view = m_Camera->GetViewMatrix();
		cameraObjectUsage.proj = m_Camera->GetProjectiveMatrix();
		cameraObjectUsage.viewProj = m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix();

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = GPUSCENE_BINDING_OBJECT;
		objectUsage.range = sizeof(cameraObjectUsage);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&cameraObjectUsage, objectUsage);

		m_InstanceCullPipelines[currentFrameIndex]->Execute(primaryBuffer, numDispatch, 1, 1, &objectUsage);
		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("GPUScene_GroupAllocate", glm::vec4(1));
		uint32_t numDispatch = (megaShaderNum + GPUSCENE_GROUP_SIZE - 1) / GPUSCENE_GROUP_SIZE;
		m_GroupAllocatePipelines[currentFrameIndex]->Execute(primaryBuffer, numDispatch, 1, 1, nullptr);
		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("GPUScene_GroupScatter", glm::vec4(1));
		m_GroupScatterPipelines[currentFrameIndex]->ExecuteIndirect(primaryBuffer, m_IndirectArgsBuffers[currentFrameIndex], nullptr);
		primaryBuffer->EndDebugMarker();
	}

	primaryBuffer->EndDebugMarker();

	return true;
}

void KGPUScene::ReloadShader()
{
	for (IKComputePipelinePtr& pipeline : m_InitStatePipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr& pipeline : m_InstanceCullPipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr& pipeline : m_GroupAllocatePipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr& pipeline : m_GroupScatterPipelines)
	{
		pipeline->Reload();
	}
}