#include "KGPUScene.h"
#include "KBase/Publish/KInteger.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Render/KMeshLODView.h"
#include "Internal/Render/KRenderUtil.h"

KGPUScene::KGPUScene()
	: m_Scene(nullptr)
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
		sceneEntity->id = entity->GetID();
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
		m_DataDirtyBits |= TEXTURE_BINDINGS_DIRTY;
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
	KHash::HashCombine(hash, material->IsDoubleSide());

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
			if (SRGB_TEXTURE_BINDING_MACRO_INDEX[i] != uint32_t(-1))
			{
				IKTexturePtr texture = textureBinding->GetTexture(i);
				texture->WaitForMemory();
				KHash::HashCombine(hash, !textureBinding->GetIsVirtualTexture(i) && KPixelFormatHelper::IsSRGBFormat(texture->GetTextureFormat()));
			}
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
		if (!KInteger::FitsUInt32(vertexData->vertexFormats.size())) return UINT32_MAX;
		const KShaderInformation::Constant* constantInfo = material->GetShadingInfo();

		MegaShaderItem newItem;
		newItem.refCount = 1;
		newItem.vsShader = material->GetVSGPUSceneShader(vertexData->vertexFormats.data(), static_cast<uint32_t>(vertexData->vertexFormats.size()));
		newItem.fsShader = material->GetFSGPUSceneShader(vertexData->vertexFormats.data(), static_cast<uint32_t>(vertexData->vertexFormats.size()));
		newItem.parameterDataSize = constantInfo->size;
		newItem.parameterDataCount = 0;

		newItem.parametersBuffers.resize(KRenderGlobal::NumFramesInFlight);
		KRenderGlobal::RenderDevice->CreateStorageBuffer(newItem.materialIndicesBuffer);
		newItem.indirectDrawArgsBuffers.resize(KRenderGlobal::NumFramesInFlight);
		for (uint32_t i = 0; i < KRenderGlobal::NumFramesInFlight; ++i)
		{
			KRenderGlobal::RenderDevice->CreateStorageBuffer(newItem.parametersBuffers[i]);
			KRenderGlobal::RenderDevice->CreateStorageBuffer(newItem.indirectDrawArgsBuffers[i]);
		}

		for (uint32_t i = 0; i < GPUSCENE_RENDER_STAGE_COUNT; ++i)
		{
			newItem.darwPipelines[i].resize(KRenderGlobal::NumFramesInFlight);
		}

		for (GPUSceneRenderStage stage : {GPUSCENE_RENDER_STAGE_BASEPASS_MAIN})
		{
			if (material->GetShadingMode() == MSM_TRANSRPANT)
			{
				continue;
			}
			for (uint32_t frameIndex = 0; frameIndex < KRenderGlobal::NumFramesInFlight; ++frameIndex)
			{
				IKPipelinePtr pipeline = nullptr;
				KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

				pipeline->SetVertexBinding(nullptr, 0);
				pipeline->SetShader(ST_VERTEX, newItem.vsShader);

				pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
				pipeline->SetBlendEnable(false);
				pipeline->SetCullMode(material->IsDoubleSide() ? CM_NONE : CM_BACK);
				pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
				pipeline->SetPolygonMode(PM_FILL);
				pipeline->SetColorWrite(true, true, true, true);
				pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

				pipeline->SetShader(ST_FRAGMENT, newItem.fsShader);
				// TODO: Bind on need
				for (uint32_t vertexFormat = 0; vertexFormat < VF_SCENE_COUNT; ++vertexFormat)
				{
					pipeline->SetStorageBuffer(SHADER_BINDING_POINT_NORMAL_UV + vertexFormat, ST_VERTEX, m_MegaBuffer.vertexBuffers[vertexFormat]);
				}
				pipeline->SetStorageBuffer(SHADER_BINDING_INDEX, ST_VERTEX, m_MegaBuffer.indexBuffer);
				pipeline->SetStorageBuffer(SHADER_BINDING_MESH_STATE, ST_VERTEX, m_MegaBuffer.meshStateBuffer);

				pipeline->SetStorageBuffer(SHADER_BINDING_INSTANCE_DATA, ST_VERTEX, m_InstanceDataBuffers[frameIndex]);
				pipeline->SetStorageBuffer(SHADER_BINDING_DRAWING_GROUP, ST_VERTEX, m_GroupDataBuffers[frameIndex]);

				pipeline->SetStorageBuffer(SHADER_BINDING_MATERIAL_PARAMETER, ST_FRAGMENT, newItem.parametersBuffers[frameIndex]);
				pipeline->SetStorageBuffer(SHADER_BINDING_MATERIAL_INDEX, ST_FRAGMENT, newItem.materialIndicesBuffer);
				pipeline->SetStorageBuffer(SHADER_BINDING_MATERIAL_TEXTURE_BINDING, ST_FRAGMENT, m_MaterialTextureBindingBuffer);
				pipeline->SetStorageBuffer(SHADER_BINDING_MEGA_SHADER_STATE, ST_FRAGMENT, m_MegaShaderStateBuffers[frameIndex]);

				IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
				pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_FRAGMENT, cameraBuffer);

				bool hasVirtualTexture = false;
				for (uint8_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
				{
					hasVirtualTexture |= material->GetTextureBinding()->GetIsVirtualTexture(i);
				}

				if (hasVirtualTexture)
				{
					for (uint8_t i = 0; i < MAX_VIRTUAL_PHYSICAL_TEXTURE_BINDING; ++i)
					{
						uint32_t binding = SHADER_BINDING_TEXTURE0 + MAX_MATERIAL_TEXTURE_BINDING + i;
						pipeline->SetSampler(binding, KRenderGlobal::VirtualTextureManager.GetPhysicalTextureFramebuffer(i), *KRenderGlobal::VirtualTextureManager.GetPhysicalTextureSampler(i), true);
					}
				}

				IKUniformBufferPtr virtualTextureBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VIRTUAL_TEXTURE_CONSTANT);
				pipeline->SetConstantBuffer(CBT_VIRTUAL_TEXTURE_CONSTANT, ST_FRAGMENT, virtualTextureBuffer);

				ASSERT_RESULT(pipeline->Init());

				newItem.darwPipelines[stage][frameIndex] = pipeline;
			}
		}

		{
			newItem.calcDrawArgsPipelines.resize(KRenderGlobal::NumFramesInFlight);
			for (uint32_t frameIndex = 0; frameIndex < KRenderGlobal::NumFramesInFlight; ++frameIndex)
			{
				IKComputePipelinePtr pipeline = nullptr;
				KRenderGlobal::RenderDevice->CreateComputePipeline(pipeline);
				pipeline->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[frameIndex], COMPUTE_RESOURCE_IN, true);
				pipeline->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_DATA, m_InstanceDataBuffers[frameIndex], COMPUTE_RESOURCE_IN, true);
				pipeline->BindStorageBuffer(GPUSCENE_BINDING_MESH_STATE, m_MegaBuffer.meshStateBuffer, COMPUTE_RESOURCE_IN, true);
				pipeline->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[frameIndex], COMPUTE_RESOURCE_IN, true);
				pipeline->BindStorageBuffer(GPUSCENE_BINDING_GROUP_DATA, m_GroupDataBuffers[frameIndex], COMPUTE_RESOURCE_IN, true);
				pipeline->BindStorageBuffer(GPUSCENE_BINDING_INDIRECT_DRAW_ARGS, newItem.indirectDrawArgsBuffers[frameIndex], COMPUTE_RESOURCE_OUT, true);
				pipeline->BindDynamicUniformBuffer(GPUSCENE_BINDING_OBJECT);
				pipeline->Init("gpuscene/calc_draw_args.comp", m_ComputeBindingEnv, KRenderGlobal::EnableAsyncShaderCompile);
				newItem.calcDrawArgsPipelines[frameIndex] = pipeline;
			}
		}

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
		if (removeIndex != m_SubMeshes.size() - 1)
		{
			m_SubMeshes[removeIndex] = std::move(m_SubMeshes.back());
			m_SubMeshToIndex.at(m_SubMeshes[removeIndex].subMesh) = removeIndex;
		}
		m_SubMeshes.pop_back();
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
		m_DataDirtyBits |= TEXTURE_BINDINGS_DIRTY;
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

		MegaShaderItem& megaShader = m_MegaShaders[removeIndex];
		for (uint32_t i = 0; i < GPUSCENE_RENDER_STAGE_COUNT; ++i)
		{
			SAFE_UNINIT_CONTAINER(megaShader.darwPipelines[i]);
		}
		SAFE_UNINIT_CONTAINER(megaShader.parametersBuffers);
		SAFE_UNINIT(megaShader.materialIndicesBuffer);
		SAFE_UNINIT_CONTAINER(megaShader.indirectDrawArgsBuffers);
		SAFE_UNINIT_CONTAINER(megaShader.calcDrawArgsPipelines);

		m_MegaShaders.erase(m_MegaShaders.begin() + removeIndex);
	}
}

bool KGPUScene::AddEntity(IKEntity* entity, const KAABBBox& localBound, const glm::mat4& transform, const std::vector<KMaterialSubMeshPtr>& subMeshes)
{
	if (entity && !GetEntity(entity))
	{
		SceneEntityPtr sceneEntity = CreateEntity(entity);
		sceneEntity->prevTransform = transform;
		sceneEntity->transform = transform;
		sceneEntity->subMeshes.reserve(subMeshes.size());
		for (size_t slot = 0; slot < subMeshes.size(); ++slot)
		{
			const auto& binding = subMeshes[slot];
			const auto base = binding->GetSubMesh();
			const auto material = binding->GetMaterial();
			if (!base || !material || material->GetShadingMode() != MSM_OPAQUE || !base->GetDrawElementCount())
			{
				continue;
			}
			sceneEntity->subMeshes.push_back({ binding, slot });
			CreateOrGetSubMeshIndex(base, true);
			for (const auto& lod : base->GetReducedLODs())
			{
				if (lod)
				{
					CreateOrGetSubMeshIndex(lod, true);
				}
			}
			CreateOrGetMaterialIndex(material.Get(), true);
			CreateOrGetMegaShaderIndex(base, material.Get(), true);
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
		for (const auto& subMesh : sceneEntity->subMeshes)
		{
			const auto base = subMesh.binding->GetSubMesh();
			for (const auto& lod : base->GetReducedLODs())
			{
				if (lod)
				{
					RemoveSubMesh(lod);
				}
			}
			const auto material = subMesh.binding->GetMaterial();
			RemoveSubMesh(base);
			RemoveMaterial(material.Get());
			RemoveMegaShader(base, material.Get());
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

	for (const auto& sceneEntity : m_Entities)
	{
		for (auto& subMesh : sceneEntity->subMeshes)
		{
			const auto base = subMesh.binding->GetSubMesh();
			const auto material = subMesh.binding->GetMaterial();
			subMesh.meshIndices[0] = CreateOrGetSubMeshIndex(base, false);
			for (uint32_t level = 1; level < KSubMesh::LOD_COUNT; ++level)
			{
				const auto& lod = base->GetReducedLODs()[level - 1];
				subMesh.meshIndices[level] = lod ? CreateOrGetSubMeshIndex(lod, false) : subMesh.meshIndices[level - 1];
			}
			subMesh.megaShaderIndex = CreateOrGetMegaShaderIndex(base, material.Get(), false);
			MegaShaderItem& megaShader = m_MegaShaders[subMesh.megaShaderIndex];
			subMesh.shaderDrawIndex = static_cast<uint32_t>(megaShader.materialIndices.size());
			megaShader.materialIndices.push_back(CreateOrGetMaterialIndex(material.Get(), false));
		}
	}
}

void KGPUScene::RebuildMegaBuffer()
{
	++m_MegaBufferRebuilds;
	std::unordered_map<IKVertexBufferPtr, uint32_t> vertexBufferOffsets[VF_SCENE_COUNT];
	std::unordered_map<IKIndexBufferPtr, uint32_t> indexBufferOffsets;
	std::unordered_map<uint32_t, uint32_t> generatedIndexOffsets;
	uint64_t vertexBufferSizes[VF_SCENE_COUNT] = { 0 };
	uint64_t indexBufferSize = 0;

	// 每个源缓冲只复制一次，子网格保留各自的范围。
	for (SubMeshItem& item : m_SubMeshes)
	{
		const KVertexData* vertexData = item.subMesh->GetVertexData();
		item.vertexCount = vertexData->vertexCount;
		KIndexData indexData;
		bool indexDraw = false;
		item.subMesh->GetIndexData(indexData, indexDraw);
		for (size_t index = 0; index < vertexData->vertexFormats.size(); ++index)
		{
			VertexFormat format = vertexData->vertexFormats[index];
			IKVertexBufferPtr buffer = vertexData->vertexBuffers[index];
			const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail(format);
			auto inserted = vertexBufferOffsets[format].emplace(buffer, (uint32_t)vertexBufferSizes[format]);
			if (inserted.second)
			{
				vertexBufferSizes[format] += buffer->GetBufferSize();
			}
			ASSERT_RESULT(KInteger::FitsUInt32(vertexBufferSizes[format]));
			// 索引绘制沿用普通绘制的绝对顶点索引。
			uint64_t start = indexDraw ? 0 : uint64_t(vertexData->vertexStart) * detail.vertexSize;
			uint64_t size = uint64_t(vertexData->vertexCount) * detail.vertexSize;
			ASSERT_RESULT(start + size <= buffer->GetBufferSize());
			item.vertexBufferOffset[format] = inserted.first->second + (uint32_t)start;
		}

		if (indexDraw)
		{
			IKIndexBufferPtr buffer = indexData.indexBuffer;
			auto inserted = indexBufferOffsets.emplace(buffer, (uint32_t)indexBufferSize);
			if (inserted.second)
			{
				indexBufferSize += uint64_t(buffer->GetIndexCount()) * sizeof(uint32_t);
			}
			ASSERT_RESULT(uint64_t(indexData.indexStart) + indexData.indexCount <= buffer->GetIndexCount());
			item.indexBufferOffset = inserted.first->second + indexData.indexStart * sizeof(uint32_t);
			item.indexCount = indexData.indexCount;
		}
		else
		{
			// 顶点偏移已包含 vertexStart，索引从零开始生成。
			auto inserted = generatedIndexOffsets.emplace(item.vertexCount, (uint32_t)indexBufferSize);
			if (inserted.second)
			{
				indexBufferSize += uint64_t(item.vertexCount) * sizeof(uint32_t);
			}
			item.indexBufferOffset = inserted.first->second;
			item.indexCount = item.vertexCount;
		}
		ASSERT_RESULT(KInteger::FitsUInt32(indexBufferSize));
	}

	for (uint32_t format = 0; format < VF_SCENE_COUNT; ++format)
	{
		const IKStorageBufferPtr& dstBuffer = m_MegaBuffer.vertexBuffers[format];
		if (!vertexBufferSizes[format])
		{
			if (dstBuffer->GetBufferSize() != sizeof(uint32_t))
			{
				dstBuffer->UnInit();
				dstBuffer->InitMemory(sizeof(uint32_t), nullptr);
				dstBuffer->InitDevice(false, false);
			}
			continue;
		}
		const auto& offsets = vertexBufferOffsets[format];
		if (offsets.size() == 1 && dstBuffer->InitFromVertexBuffer(offsets.begin()->first))
		{
			continue;
		}

		dstBuffer->UnInit();
		dstBuffer->InitMemory(vertexBufferSizes[format], nullptr);
		dstBuffer->InitDevice(false, false);
		const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail((VertexFormat)format);
		dstBuffer->SetDebugName((std::string("GPUSceneVertexData_") + detail.name).c_str());

		void* pDstData = nullptr;
		ASSERT_RESULT(dstBuffer->Map(&pDstData, MA_WRITE_DISCARD));
		for (const auto& source : offsets)
		{
			const IKVertexBufferPtr& srcBuffer = source.first;
			void* pSrcData = nullptr;
			ASSERT_RESULT(srcBuffer->Map(&pSrcData, MA_READ_ONLY));
			memcpy(POINTER_OFFSET(pDstData, source.second), pSrcData, (size_t)srcBuffer->GetBufferSize());
			srcBuffer->UnMap();
		}
		dstBuffer->UnMap();
	}

	const IKStorageBufferPtr& dstBuffer = m_MegaBuffer.indexBuffer;
	if (dstBuffer->GetBufferSize() != indexBufferSize)
	{
		dstBuffer->UnInit();
		if (indexBufferSize)
		{
			dstBuffer->InitMemory(indexBufferSize, nullptr);
			dstBuffer->InitDevice(false, false);
			dstBuffer->SetDebugName("GPUSceneIndexData");
		}
	}
	if (!indexBufferSize)
	{
		return;
	}

	void* pDstData = nullptr;
	ASSERT_RESULT(dstBuffer->Map(&pDstData, MA_WRITE_DISCARD));
	for (const auto& source : indexBufferOffsets)
	{
		const IKIndexBufferPtr& srcBuffer = source.first;
		void* pSrcData = nullptr;
		ASSERT_RESULT(srcBuffer->Map(&pSrcData, MA_READ_ONLY));
		uint32_t* pDstIndices = static_cast<uint32_t*>(POINTER_OFFSET(pDstData, source.second));
		const uint32_t count = srcBuffer->GetIndexCount();
		if (srcBuffer->GetIndexType() == IT_16)
		{
			const uint16_t* pSrcIndices = static_cast<const uint16_t*>(pSrcData);
			for (uint32_t i = 0; i < count; ++i)
			{
				pDstIndices[i] = pSrcIndices[i];
			}
		}
		else
		{
			memcpy(pDstIndices, pSrcData, size_t(count) * sizeof(uint32_t));
		}
		srcBuffer->UnMap();
	}
	for (const auto& generated : generatedIndexOffsets)
	{
		uint32_t* pDstIndices = static_cast<uint32_t*>(POINTER_OFFSET(pDstData, generated.second));
		for (uint32_t i = 0; i < generated.first; ++i)
		{
			pDstIndices[i] = i;
		}
	}
	dstBuffer->UnMap();
}

void KGPUScene::RebuildMeshStateBuffer()
{
	m_MeshStates.clear();
	m_MeshStates.reserve(m_SubMeshes.size());

	for (SubMeshItem& subMeshItem : m_SubMeshes)
	{
		KAABBBox subMeshBound = subMeshItem.subMesh->GetBound();

		KGPUSceneMeshState newMeshState{};

		newMeshState.boundCenter = glm::vec4(subMeshBound.GetCenter(), 0);
		newMeshState.boundHalfExtend = glm::vec4(subMeshBound.GetExtend() * 0.5f, 0);

		newMeshState.miscs[KGPUSceneMeshState::INDEX_COUNT_INDEX] = subMeshItem.indexCount;
		newMeshState.miscs[KGPUSceneMeshState::VERTEX_COUNT_INDEX] = subMeshItem.vertexCount;
		newMeshState.miscs[KGPUSceneMeshState::INDEX_BUFFER_OFFSET] = subMeshItem.indexBufferOffset / sizeof(uint32_t);

		for (uint32_t i = 0; i < VF_SCENE_COUNT; ++i)
		{
			const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail((VertexFormat)i);
			newMeshState.miscs[KGPUSceneMeshState::VERTEX_BUFFER_OFFSET + i] = subMeshItem.vertexBufferOffset[i] / (uint32_t)detail.vertexSize;
		}

		m_MeshStates.push_back(newMeshState);
	}

	m_MegaBuffer.meshStateBuffer->UnInit();
	m_MegaBuffer.meshStateBuffer->InitMemory(uint64_t(sizeof(KGPUSceneMeshState)) * m_MeshStates.size(), m_MeshStates.data());
	m_MegaBuffer.meshStateBuffer->InitDevice(false, false);
}

void KGPUScene::RebuildMegaShaderState()
{
	for (size_t i = 0; i < m_MegaShaderStateBuffers.size(); ++i)
	{
		m_MegaShaderStateBuffers[i]->UnInit();
		m_MegaShaderStateBuffers[i]->InitMemory(uint64_t(sizeof(KGPUSceneMegaShaderState)) * m_MegaShaders.size(), nullptr);
		m_MegaShaderStateBuffers[i]->InitDevice(false, false);
	}
}

void KGPUScene::RebuildTextureBindings()
{
	std::map<std::pair<IKTexturePtr, IKSamplerPtr>, uint32_t> textureLocations;
	std::vector<IKFrameBufferPtr> images;
	std::vector<IKSamplerPtr> samplers;
	std::vector<KGPUSceneMaterialTextureBinding> materialBindings(m_Materials.size());

	for (size_t materialIndex = 0; materialIndex < m_Materials.size(); ++materialIndex)
	{
		const MaterialItem& materialItem = m_Materials[materialIndex];
		KGPUSceneMaterialTextureBinding& materialTextureBinding = materialBindings[materialIndex];

		IKMaterialPtr material = materialItem.material;
		IKMaterialTextureBindingPtr textureBinding = material->GetTextureBinding();
		for (uint32_t slotIndex = 0; slotIndex < MAX_MATERIAL_TEXTURE_BINDING; ++slotIndex)
		{
			materialTextureBinding.binding[slotIndex] = UINT32_MAX;
			IKTexturePtr texture = textureBinding->GetTexture(slotIndex);
			if (texture)
			{
				IKSamplerPtr sampler = textureBinding->GetSampler(slotIndex);
				auto inserted = textureLocations.emplace(std::make_pair(texture, sampler), (uint32_t)images.size());
				if (inserted.second)
				{
					texture->WaitForDevice();
					images.push_back(texture->GetFrameBuffer());
					samplers.push_back(sampler);
				}
				materialTextureBinding.binding[slotIndex] = inserted.first->second;
			}
		}
	}

	if (images.empty())
	{
		KTextureRef texture;
		KSamplerRef sampler;
		KRenderGlobal::TextureManager.GetErrorTexture(texture);
		KRenderGlobal::SamplerManager.GetErrorSampler(sampler);
		images.push_back(texture->GetFrameBuffer());
		samplers.push_back(sampler.Get());
	}

	const uint64_t bindingBufferSize = sizeof(KGPUSceneMaterialTextureBinding) * std::max<size_t>(1, materialBindings.size());
	m_MaterialTextureBindingBuffer->UnInit();
	m_MaterialTextureBindingBuffer->InitMemory(bindingBufferSize, materialBindings.empty() ? nullptr : materialBindings.data());
	m_MaterialTextureBindingBuffer->InitDevice(false, false);

	for (MegaShaderItem& shader : m_MegaShaders)
	{
		shader.fsShader->WaitForDevice();
		const auto& textures = shader.fsShader->GetInformation().textures;
		const bool hasTextureTable = std::any_of(textures.begin(), textures.end(), [](const KShaderInformation::Texture& texture)
		{
			return texture.runtimeArray && texture.bindingIndex == SHADER_BINDING_TEXTURE0;
		});
		if (!hasTextureTable)
		{
			continue;
		}
		for (auto& pipelines : shader.darwPipelines)
		{
			for (const IKPipelinePtr& pipeline : pipelines)
			{
				if (pipeline)
				{
					pipeline->SetSamplers(SHADER_BINDING_TEXTURE0, images, samplers, true);
				}
			}
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
	
		for (const auto& subMesh : entity->subMeshes)
		{
			newInstance.miscs[KGPUSceneInstance::ENEITY_INDEX] = entity->index;
			newInstance.miscs[KGPUSceneInstance::SUBMESH_INDEX] = subMesh.meshIndices[0];
			newInstance.miscs[KGPUSceneInstance::MEGA_SHADER_INDEX] = subMesh.megaShaderIndex;
			newInstance.miscs[KGPUSceneInstance::SHADER_DRAW_INDEX] = subMesh.shaderDrawIndex;
			m_Instances.push_back(newInstance);
		}
	}

	size_t instanceBufferCount = std::max((size_t)GPUSCENE_GROUP_SIZE,
		(size_t)KMath::SmallestPowerOf2GreaterEqualThan(m_Instances.size()));
	for (size_t i = 0; i < m_InstanceDataBuffers.size(); ++i)
	{
		IKStorageBufferPtr buffer = m_InstanceDataBuffers[i];
		if (buffer->GetBufferSize() < uint64_t(instanceBufferCount) * sizeof(KGPUSceneInstance))
		{
			buffer->UnInit();
			buffer->InitMemory(uint64_t(instanceBufferCount) * sizeof(KGPUSceneInstance), nullptr);
			buffer->InitDevice(false, false);
		}
	}

	for (size_t i = 0; i < m_GroupDataBuffers.size(); ++i)
	{
		IKStorageBufferPtr buffer = m_GroupDataBuffers[i];
		if (buffer->GetBufferSize() < uint64_t(instanceBufferCount) * sizeof(uint32_t))
		{
			buffer->UnInit();
			buffer->InitMemory(uint64_t(instanceBufferCount) * sizeof(uint32_t), nullptr);
			buffer->InitDevice(false, false);
		}
	}

	for (size_t i = 0; i < m_InstanceCullResultBuffers.size(); ++i)
	{
		IKStorageBufferPtr buffer = m_InstanceCullResultBuffers[i];
		if (buffer->GetBufferSize() < uint64_t(instanceBufferCount) * sizeof(uint32_t))
		{
			buffer->UnInit();
			buffer->InitMemory(uint64_t(instanceBufferCount) * sizeof(uint32_t), nullptr);
			buffer->InitDevice(false, false);
		}
	}
}

void KGPUScene::RebuildMegaShaderBuffer()
{
	for (MegaShaderItem& shaderItem : m_MegaShaders)
	{
		if (shaderItem.parameterDataCount < shaderItem.refCount)
		{
			shaderItem.parameterDataCount = KMath::SmallestPowerOf2GreaterEqualThan(shaderItem.refCount);
			for (size_t i = 0; i < shaderItem.parametersBuffers.size(); ++i)
			{
				shaderItem.parametersBuffers[i]->UnInit();
				shaderItem.parametersBuffers[i]->InitMemory(uint64_t(shaderItem.parameterDataSize) * shaderItem.parameterDataCount, nullptr);
				shaderItem.parametersBuffers[i]->InitDevice(false, false);
			}
			shaderItem.materialIndicesBuffer->UnInit();
			shaderItem.materialIndicesBuffer->InitMemory(uint64_t(sizeof(uint32_t)) * shaderItem.parameterDataCount, nullptr);
			shaderItem.materialIndicesBuffer->InitDevice(false, false);
		}
		if (shaderItem.drawArgsDataCount < m_Instances.size())
		{
			uint32_t drawArgsCount = (uint32_t)KMath::SmallestPowerOf2GreaterEqualThan(m_Instances.size());
			shaderItem.drawArgsDataCount = drawArgsCount < GPUSCENE_GROUP_SIZE ? GPUSCENE_GROUP_SIZE : drawArgsCount;
			for (size_t i = 0; i < shaderItem.indirectDrawArgsBuffers.size(); ++i)
			{
				shaderItem.indirectDrawArgsBuffers[i]->UnInit();
				shaderItem.indirectDrawArgsBuffers[i]->InitMemory(uint64_t(sizeof(uint32_t)) * 4 * shaderItem.drawArgsDataCount, nullptr);
				shaderItem.indirectDrawArgsBuffers[i]->InitDevice(true, false);
			}
		}

		shaderItem.materialIndicesBuffer->Write(shaderItem.materialIndices.data(), 0, shaderItem.materialIndices.size() * sizeof(uint32_t));

		for (size_t i = 0; i < shaderItem.parametersBuffers.size(); ++i)
		{
			shaderItem.parametersBuffers[i]->SetDebugName((std::string("KGPUSceneShaderParameter_") + std::to_string(i)).c_str());
		}
		shaderItem.materialIndicesBuffer->SetDebugName("KGPUSceneShaderMaterialIndices");
		for (size_t i = 0; i < shaderItem.indirectDrawArgsBuffers.size(); ++i)
		{
			shaderItem.indirectDrawArgsBuffers[i]->SetDebugName((std::string("KGPUSceneIndirectDrawArgs_") + std::to_string(i)).c_str());
		}
	}
}

void KGPUScene::OnSceneChanged(EntitySceneOp op, IKEntity* entity)
{
	if (op == ESO_ADD)
	{
		KRenderComponent* renderComponent = nullptr;
		KTransformComponent* transformComponent = nullptr;
		ASSERT_RESULT(entity->GetComponent(CT_RENDER, &renderComponent));
		ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));
		const glm::mat4& transform = transformComponent->GetFinal_RenderThread();
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
		IKRenderComponent* renderComponent = nullptr;
		if (entity->GetComponent(CT_RENDER, &renderComponent))
		{
			renderComponent->UnRegisterCallback(&m_OnRenderComponentChangedFunc);
		}
		RemoveEntity(entity);
	}
	else if (op == ESO_TRANSFORM)
	{
		KTransformComponent* transformComponent = nullptr;
		ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));
		const glm::mat4& transform = transformComponent->GetFinal_RenderThread();
		TransformEntity(entity, transform);
	}
}

void KGPUScene::OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init)
{
	IKEntity* entity = renderComponent->GetEntityHandle();
	if (!entity)
	{
		return;
	}
	if (init)
	{
		OnSceneChanged(ESO_ADD, entity);
	}
	else
	{
		RemoveEntity(entity);
	}
}

void KGPUScene::InitializeBuffers()
{
	uint32_t numFrames = KRenderGlobal::NumFramesInFlight;

	m_SceneStateBuffers.resize(numFrames);
	m_InstanceDataBuffers.resize(numFrames);
	m_GroupDataBuffers.resize(numFrames);
	m_DispatchArgsBuffers.resize(numFrames);
	m_InstanceCullResultBuffers.resize(numFrames);
	m_MegaShaderStateBuffers.resize(numFrames);

	for (uint32_t i = 0; i < numFrames; ++i)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_SceneStateBuffers[i]);
		m_SceneStateBuffers[i]->InitMemory(sizeof(KGPUSceneState), nullptr);
		m_SceneStateBuffers[i]->InitDevice(false, false);
		m_SceneStateBuffers[i]->SetDebugName((std::string("GPUSceneStateData_") + std::to_string(i)).c_str());
		m_SceneStateBuffers[i]->SetDebugName((std::string("GPUSceneStateData_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_InstanceDataBuffers[i]);
		m_InstanceDataBuffers[i]->InitMemory(sizeof(KGPUSceneInstance), nullptr);
		m_InstanceDataBuffers[i]->InitDevice(false, false);
		m_InstanceDataBuffers[i]->SetDebugName((std::string("GPUSceneInstanceData_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_GroupDataBuffers[i]);
		m_GroupDataBuffers[i]->InitMemory(sizeof(uint32_t), nullptr);
		m_GroupDataBuffers[i]->InitDevice(false, false);
		m_GroupDataBuffers[i]->SetDebugName((std::string("GPUSceneGroupData_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_DispatchArgsBuffers[i]);
		m_DispatchArgsBuffers[i]->InitMemory(sizeof(uint32_t) * 4, nullptr);
		m_DispatchArgsBuffers[i]->InitDevice(true, false);
		m_DispatchArgsBuffers[i]->SetDebugName((std::string("GPUSceneIndirectDispatchArgs_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_InstanceCullResultBuffers[i]);
		m_InstanceCullResultBuffers[i]->InitMemory(sizeof(uint32_t), nullptr);
		m_InstanceCullResultBuffers[i]->InitDevice(false, false);
		m_InstanceCullResultBuffers[i]->SetDebugName((std::string("GPUSceneInstanceCullResult_") + std::to_string(i)).c_str());

		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MegaShaderStateBuffers[i]);
		m_MegaShaderStateBuffers[i]->InitMemory(sizeof(KGPUSceneMegaShaderState), nullptr);
		m_MegaShaderStateBuffers[i]->InitDevice(false, false);
		m_MegaShaderStateBuffers[i]->SetDebugName((std::string("GPUSceneMegaShaderState_") + std::to_string(i)).c_str());
	}

	for (uint32_t vertexFormat = 0; vertexFormat < VF_SCENE_COUNT; ++vertexFormat)
	{
		const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail((VertexFormat)vertexFormat);
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MegaBuffer.vertexBuffers[vertexFormat]);
		m_MegaBuffer.vertexBuffers[vertexFormat]->InitMemory(detail.vertexSize, nullptr);
		m_MegaBuffer.vertexBuffers[vertexFormat]->InitDevice(false, false);
		m_MegaBuffer.vertexBuffers[vertexFormat]->SetDebugName((std::string("GPUSceneVertexData_") + detail.name).c_str());
	}

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MegaBuffer.indexBuffer);
	m_MegaBuffer.indexBuffer->InitMemory(sizeof(uint32_t), nullptr);
	m_MegaBuffer.indexBuffer->InitDevice(false, false);
	m_MegaBuffer.indexBuffer->SetDebugName("GPUSceneIndexData");

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MegaBuffer.meshStateBuffer);
	m_MegaBuffer.meshStateBuffer->InitMemory(sizeof(KGPUSceneMeshState), nullptr);
	m_MegaBuffer.meshStateBuffer->InitDevice(false, false);
	m_MegaBuffer.meshStateBuffer->SetDebugName("GPUSceneMeshState");

	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_MaterialTextureBindingBuffer);
	m_MaterialTextureBindingBuffer->InitMemory(sizeof(KGPUSceneMaterialTextureBinding), nullptr);
	m_MaterialTextureBindingBuffer->InitDevice(false, false);
	m_MaterialTextureBindingBuffer->SetDebugName("GPUSceneMaterialTextureBinding");
}

void KGPUScene::InitializePipelines()
{
	uint32_t numFrames = KRenderGlobal::NumFramesInFlight;

	m_InitStatePipelines.resize(numFrames);
	m_InstanceCullPipelines.resize(numFrames);
	m_CalcDispatchArgsPipelines.resize(numFrames);
	m_GroupAllocatePipelines.resize(numFrames);
	m_GroupScatterPipelines.resize(numFrames);

	for (uint32_t i = 0; i < numFrames; ++i)
	{
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitStatePipelines[i]);
		m_InitStatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InitStatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InitStatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INDIRECT_DISPATCH_ARGS, m_DispatchArgsBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InitStatePipelines[i]->Init("gpuscene/init_state.comp", m_ComputeBindingEnv, KRenderGlobal::EnableAsyncShaderCompile);

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_InstanceCullPipelines[i]);
		m_InstanceCullPipelines[i]->BindDynamicUniformBuffer(GPUSCENE_BINDING_OBJECT);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MESH_STATE, m_MegaBuffer.meshStateBuffer, COMPUTE_RESOURCE_IN, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_DATA, m_InstanceDataBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_CULL_RESULT, m_InstanceCullResultBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INDIRECT_DISPATCH_ARGS, m_DispatchArgsBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_InstanceCullPipelines[i]->Init("gpuscene/instance_cull.comp", m_ComputeBindingEnv, KRenderGlobal::EnableAsyncShaderCompile);

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_CalcDispatchArgsPipelines[i]);
		m_CalcDispatchArgsPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_CalcDispatchArgsPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INDIRECT_DISPATCH_ARGS, m_DispatchArgsBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_CalcDispatchArgsPipelines[i]->Init("gpuscene/calc_dispatch_args.comp", m_ComputeBindingEnv, KRenderGlobal::EnableAsyncShaderCompile);

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_GroupAllocatePipelines[i]);
		m_GroupAllocatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_GroupAllocatePipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_GroupAllocatePipelines[i]->Init("gpuscene/group_allocate.comp", m_ComputeBindingEnv, KRenderGlobal::EnableAsyncShaderCompile);

		KRenderGlobal::RenderDevice->CreateComputePipeline(m_GroupScatterPipelines[i]);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_SCENE_STATE, m_SceneStateBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_DATA, m_InstanceDataBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_INSTANCE_CULL_RESULT, m_InstanceCullResultBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_MEGA_SHADER_STATE, m_MegaShaderStateBuffers[i], COMPUTE_RESOURCE_IN, true);
		m_GroupScatterPipelines[i]->BindStorageBuffer(GPUSCENE_BINDING_GROUP_DATA, m_GroupDataBuffers[i], COMPUTE_RESOURCE_OUT, true);
		m_GroupScatterPipelines[i]->Init("gpuscene/group_scatter.comp", m_ComputeBindingEnv, KRenderGlobal::EnableAsyncShaderCompile);
	}
}

bool KGPUScene::Init(IKRenderScene* scene)
{
	UnInit();
	if (scene)
	{
		InitializeBuffers();
		InitializePipelines();

		m_Scene = scene;

		m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);
	}
	return true;
}

bool KGPUScene::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
		KRenderUtil::UnRegisterRenderComponentCallback(&m_OnRenderComponentChangedFunc);
		m_Scene = nullptr;
	}

	m_SubMeshes.clear();
	m_SubMeshToIndex.clear();

	m_Materials.clear();
	m_MaterialToIndex.clear();

	for (MegaShaderItem& megaShader : m_MegaShaders)
	{
		for (uint32_t i = 0; i < GPUSCENE_RENDER_STAGE_COUNT; ++i)
		{
			SAFE_UNINIT_CONTAINER(megaShader.darwPipelines[i]);
		}
		SAFE_UNINIT_CONTAINER(megaShader.parametersBuffers);
		SAFE_UNINIT(megaShader.materialIndicesBuffer);
		SAFE_UNINIT_CONTAINER(megaShader.indirectDrawArgsBuffers);
		SAFE_UNINIT_CONTAINER(megaShader.calcDrawArgsPipelines);
	}

	m_MegaShaders.clear();
	m_MegaShaderToIndex.clear();

	m_EntityMap.clear();
	m_Entities.clear();
	m_Instances.clear();
	m_MeshStates.clear();

	m_DataDirtyBits = 0;
	m_MegaBufferRebuilds = 0;
	m_LastDrawFrame = UINT32_MAX;

	for (uint32_t i = 0; i < VF_SCENE_COUNT; ++i)
	{
		SAFE_UNINIT(m_MegaBuffer.vertexBuffers[i]);
	}
	SAFE_UNINIT(m_MegaBuffer.indexBuffer);
	SAFE_UNINIT(m_MegaBuffer.meshStateBuffer);

	SAFE_UNINIT(m_MaterialTextureBindingBuffer);

	SAFE_UNINIT_CONTAINER(m_SceneStateBuffers);
	SAFE_UNINIT_CONTAINER(m_InstanceDataBuffers);
	SAFE_UNINIT_CONTAINER(m_GroupDataBuffers);
	SAFE_UNINIT_CONTAINER(m_DispatchArgsBuffers);
	SAFE_UNINIT_CONTAINER(m_InstanceCullResultBuffers);
	SAFE_UNINIT_CONTAINER(m_MegaShaderStateBuffers);

	SAFE_UNINIT_CONTAINER(m_InitStatePipelines);
	SAFE_UNINIT_CONTAINER(m_InstanceCullPipelines);
	SAFE_UNINIT_CONTAINER(m_CalcDispatchArgsPipelines);
	SAFE_UNINIT_CONTAINER(m_GroupAllocatePipelines);
	SAFE_UNINIT_CONTAINER(m_GroupScatterPipelines);

	return true;
}

void KGPUScene::RebuildDirtyBuffer()
{
	if (m_DataDirtyBits)
	{
		FLUSH_INFLIGHT_RENDER_JOB();
		RebuildEntityDataIndex();
	}
	if (m_DataDirtyBits & MEGA_BUFFER_DIRTY)
	{
		RebuildMegaBuffer();
		RebuildMeshStateBuffer();
	}
	if (m_DataDirtyBits & MEGA_SHADER_DIRTY)
	{
		RebuildMegaShaderState();
	}
	if (m_DataDirtyBits & (TEXTURE_BINDINGS_DIRTY | MEGA_SHADER_DIRTY))
	{
		RebuildTextureBindings();
	}
	if (m_DataDirtyBits & INSTANCE_DIRTY)
	{
		RebuildInstance();
		RebuildMegaShaderBuffer();
	}
	m_DataDirtyBits = 0;
}

void KGPUScene::UpdateInstanceDataBuffer(KRHICommandList& commandList, const KMeshLODView& lodView)
{
	if (m_Instances.size() > 0)
	{
		uint32_t frameIndex = KRenderGlobal::CurrentInFlightFrameIndex;
		IKStorageBufferPtr instanceDataBuffer = m_InstanceDataBuffers[frameIndex];
		assert(instanceDataBuffer->GetBufferSize() >= m_Instances.size() * sizeof(KGPUSceneInstance));

		size_t instanceIndex = 0;
		for (const auto& entity : m_Entities)
		{
			const auto* levels = lodView.GetLevels(entity->id);
			for (const auto& subMesh : entity->subMeshes)
			{
				auto& instance = m_Instances[instanceIndex++];
				instance.prevTransform = entity->prevTransform;
				instance.transform = entity->transform;
				const uint32_t level = levels ? (*levels)[subMesh.slot] : 0;
				instance.miscs[KGPUSceneInstance::SUBMESH_INDEX] = subMesh.meshIndices[level];
			}
		}

		commandList.UpdateStorageBuffer(instanceDataBuffer, m_Instances.data(), 0, (uint32_t)(m_Instances.size() * sizeof(KGPUSceneInstance)));
	}
}

bool KGPUScene::Execute(KRHICommandList& commandList, IKRenderScene* scene, const KCamera& camera, const KMeshLODView& lodView)
{
	if (!KRenderGlobal::EnableGPUScene || scene != m_Scene || m_SubMeshes.empty())
	{
		return false;
	}

	RebuildDirtyBuffer();
	UpdateInstanceDataBuffer(commandList, lodView);

	uint32_t currentFrameIndex = KRenderGlobal::CurrentInFlightFrameIndex;
	uint32_t megaShaderNum = (uint32_t)m_MegaShaders.size();
	const uint32_t instanceGroups = (static_cast<uint32_t>(m_Instances.size()) + GPUSCENE_GROUP_SIZE - 1) / GPUSCENE_GROUP_SIZE;

	commandList.BeginDebugMarker("GPUScene", glm::vec4(1));

	{
		m_SceneStateBuffers[currentFrameIndex]->Write(&megaShaderNum, MEMBER_OFFSET(KGPUSceneState, megaShaderNum), sizeof(megaShaderNum));

		uint32_t numDispatch = ((uint32_t)megaShaderNum + GPUSCENE_GROUP_SIZE - 1) / GPUSCENE_GROUP_SIZE;

		commandList.BeginDebugMarker("GPUScene_InitState", glm::vec4(1));
		commandList.Execute(m_InitStatePipelines[currentFrameIndex], numDispatch, 1, 1, nullptr);
		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("GPUScene_InstanceCull", glm::vec4(1));
		struct
		{
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewProj;
			uint32_t instanceCount = 0;
			uint32_t padding[3] = { 0 };
		} cameraObjectUsage;
		static_assert((sizeof(cameraObjectUsage) % 16) == 0, "Size must be a multiple of 16");

		cameraObjectUsage.view = camera.GetViewMatrix();
		cameraObjectUsage.proj = camera.GetProjectiveMatrix();
		cameraObjectUsage.viewProj = cameraObjectUsage.proj * cameraObjectUsage.view;
		cameraObjectUsage.instanceCount = (uint32_t)m_Instances.size();

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = GPUSCENE_BINDING_OBJECT;
		objectUsage.range = sizeof(cameraObjectUsage);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&cameraObjectUsage, objectUsage);

		commandList.Execute(m_InstanceCullPipelines[currentFrameIndex], instanceGroups, 1, 1, &objectUsage);
		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("GPUScene_CalcDispatchArgs", glm::vec4(1));
		commandList.Execute(m_CalcDispatchArgsPipelines[currentFrameIndex], 1, 1, 1, nullptr);
		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("GPUScene_GroupAllocate", glm::vec4(1));
		uint32_t numDispatch = (megaShaderNum + GPUSCENE_GROUP_SIZE - 1) / GPUSCENE_GROUP_SIZE;
		commandList.Execute(m_GroupAllocatePipelines[currentFrameIndex], numDispatch, 1, 1, nullptr);
		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("GPUScene_GroupScatter", glm::vec4(1));
		commandList.ExecuteIndirect(m_GroupScatterPipelines[currentFrameIndex], m_DispatchArgsBuffers[currentFrameIndex], nullptr);
		commandList.EndDebugMarker();
	}

	{
		struct
		{
			uint32_t index = 0;
			uint32_t padding[3] = { 0 };
		} megaShaderUsage;
		static_assert((sizeof(megaShaderUsage) % 16) == 0, "Size must be a multiple of 16");

		for (uint32_t i = 0; i < m_MegaShaders.size(); ++i)
		{
			commandList.BeginDebugMarker((std::string("GPUScene_MegaShaderCalcDrawArgs_") + std::to_string(i)).c_str(), glm::vec4(1));

			MegaShaderItem& megaShader = m_MegaShaders[i];
			megaShaderUsage.index = i;

			KDynamicConstantBufferUsage objectUsage;
			objectUsage.binding = GPUSCENE_BINDING_OBJECT;
			objectUsage.range = sizeof(megaShaderUsage);

			KRenderGlobal::DynamicConstantBufferManager.Alloc(&megaShaderUsage, objectUsage);

			commandList.Execute(megaShader.calcDrawArgsPipelines[currentFrameIndex], instanceGroups, 1, 1, &objectUsage);

			commandList.EndDebugMarker();
		}
	}

	// Make compute outputs visible to indirect draws and vertex shader reads.
	commandList.PipelineMemoryBarrier(PIPELINE_STAGE_COMPUTE_SHADER,
		PIPELINE_STAGE_DRAW_INDIRECT | PIPELINE_STAGE_VERTEX_SHADER);
	commandList.EndDebugMarker();

	return true;
}

void KGPUScene::AppendBasePassCommands(IKRenderPassPtr renderPass, KRHICommandList& commandList, KRenderCommandList& commands)
{
	uint32_t frameIndex = KRenderGlobal::CurrentInFlightFrameIndex;
	m_LastDrawFrame = frameIndex;

	KGPUSceneDrawParameter drawParameter;

	for (size_t i = 0; i < m_MegaShaders.size(); ++i)
	{
		const MegaShaderItem& megaShader = m_MegaShaders[i];
		if (!megaShader.darwPipelines[GPUSCENE_RENDER_STAGE_BASEPASS_MAIN][frameIndex])
		{
			continue;
		}

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(drawParameter);

		drawParameter.megaShaderIndex = (uint32_t)i;
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&drawParameter, objectUsage);

		{
			std::vector<char> datas;
			if (megaShader.parametersBuffers[frameIndex]->GetBufferSize() > datas.max_size()) continue;
			datas.resize(static_cast<size_t>(megaShader.parametersBuffers[frameIndex]->GetBufferSize()));
			void* pData = datas.data();

			for (uint32_t materialIndex : megaShader.materialIndices)
			{
				const MaterialItem& materialItem = m_Materials[materialIndex];
				const IKMaterialPtr material = materialItem.material;
				const IKMaterialParameterPtr parameter = material->GetParameter();
				if (parameter)
				{
					const KShaderInformation::Constant* constant = material->GetShadingInfo();
					if (constant && constant->size)
					{
						for (const KShaderInformation::Constant::ConstantMember& member : constant->members)
						{
							IKMaterialValuePtr value = parameter->GetValue(member.name);
							ASSERT_RESULT(value);
							memcpy(POINTER_OFFSET(pData, member.offset), value->GetData(), member.size);
						}
					}
				}
				pData = POINTER_OFFSET(pData, megaShader.parameterDataSize);
			}
			
			commandList.UpdateStorageBuffer(megaShader.parametersBuffers[frameIndex], datas.data(), 0, (uint32_t)datas.size());
		}

		{
			KRenderCommand command;
			command.dynamicConstantUsages.push_back(objectUsage);
			command.pipeline = megaShader.darwPipelines[GPUSCENE_RENDER_STAGE_BASEPASS_MAIN][frameIndex];
			command.indirectArgsBuffer = megaShader.indirectDrawArgsBuffers[frameIndex];
			if (!command.pipeline->GetHandle(renderPass, command.pipelineHandle))
			{
				continue;
			}
			command.indexDraw = false;
			command.indirectDraw = true;
			command.indirectOffset = 0;
			command.indirectCount = (uint32_t)m_Instances.size();
			commands.push_back(std::move(command));
		}
	}
}

bool KGPUScene::ReadBasePassTriangles(uint64_t& triangles) const
{
	triangles = 0;
	if (m_LastDrawFrame == UINT32_MAX)
	{
		return false;
	}
	std::vector<std::array<uint32_t, 4>> args(m_Instances.size());
	for (const auto& shader : m_MegaShaders)
	{
		if (!shader.darwPipelines[GPUSCENE_RENDER_STAGE_BASEPASS_MAIN][m_LastDrawFrame])
		{
			continue;
		}
		const auto& buffer = shader.indirectDrawArgsBuffers[m_LastDrawFrame];
		if (!buffer->Read(args.data(), 0, uint64_t(args.size()) * sizeof(args[0])))
		{
			return false;
		}
		for (const auto& draw : args)
		{
			triangles += uint64_t(draw[0] / 3) * draw[1];
		}
	}
	return true;
}

void KGPUScene::Reload()
{
	for (IKComputePipelinePtr& pipeline : m_InitStatePipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr& pipeline : m_InstanceCullPipelines)
	{
		pipeline->Reload();
	}
	for (IKComputePipelinePtr& pipeline : m_CalcDispatchArgsPipelines)
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
	for (MegaShaderItem& megaShader : m_MegaShaders)
	{
		for (IKComputePipelinePtr& pipeline : megaShader.calcDrawArgsPipelines)
		{
			pipeline->Reload();
		}
		megaShader.vsShader->Reload();
		megaShader.fsShader->Reload();
		for (IKPipelinePtr& pipeline : megaShader.darwPipelines[GPUSCENE_RENDER_STAGE_BASEPASS_MAIN])
		{
			if (pipeline)
			{
				pipeline->Reload(false);
			}
		}
	}
}
