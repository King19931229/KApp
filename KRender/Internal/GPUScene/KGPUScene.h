#pragma once
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKRenderScene.h"
#include "Internal/Asset/KMaterialSubMesh.h"

struct KGPUSceneInstance
{
	glm::mat4 transform;
	glm::mat4 prevTransform;
	glm::vec4 boundCenter;
	glm::vec4 boundHalfExtend;
	glm::uvec4 miscs;
};
static_assert((sizeof(KGPUSceneInstance) % 16) == 0, "Size must be a multiple of 16");

struct KGPUSceneState
{
	uint32_t megaShaderNum = 0;
	uint32_t instanceCount = 0;
	uint32_t groupAllocateOffset = 0;
	uint32_t padding = 0;
};
static_assert((sizeof(KGPUSceneState) % 16) == 0, "Size must be a multiple of 16");

struct KGPUSceneMegaShaderState
{
	uint32_t instanceCount = 0;
	uint32_t groupWriteOffset = 0;
	uint32_t groupWriteNum = 0;
	uint32_t padding = 0;
};
static_assert((sizeof(KGPUSceneMegaShaderState) % 16) == 0, "Size must be a multiple of 16");

enum GPUSceneBinding
{
	#define GPUSCENE_BINDING(SEMANTIC) GPUSCENE_BINDING_##SEMANTIC,
	#include "KGPUSceneBinding.inl"
	#undef GPUSCENE_BINDING
};

class KGPUScene
{
protected:
	enum
	{
		GPUSCENE_GROUP_SIZE = 64
	};

	IKRenderScene* m_Scene;
	const KCamera* m_Camera;

	struct SubMeshItem
	{
		KSubMeshPtr subMesh;
		KAABBBox bound;
		uint32_t vertexBufferOffset[VF_COUNT] = { 0 };
		uint32_t vertexBufferSize[VF_COUNT] = { 0 };
		uint32_t vertexCount = 0;
		uint32_t indexBufferOffset = 0;
		uint32_t indexBufferSize = 0;
		uint32_t indexCount = 0;
		uint32_t refCount = 0;
	};
	std::vector<SubMeshItem> m_SubMeshes;
	std::unordered_map<KSubMeshPtr, uint32_t> m_SubMeshToIndex;

	struct MaterialItem
	{
		IKMaterialPtr material;
		uint32_t refCount = 0;
	};
	std::vector<MaterialItem> m_Materials;
	std::unordered_map<IKMaterialPtr, uint32_t> m_MaterialToIndex;

	struct MegaShaderItem
	{
		IKShaderPtr vsShader;
		IKShaderPtr fsShader;
		std::vector<IKStorageBufferPtr> parametersBuffers;
		std::vector<uint32_t> materialIndices;
		uint32_t parameterDataSize = 0;
		uint32_t parameterDataCount = 0;
		uint32_t refCount = 0;
	};
	std::vector<MegaShaderItem> m_MegaShaders;
	std::unordered_map<uint64_t, uint32_t> m_MegaShaderToIndex;

	std::vector<KGPUSceneInstance> m_Instances;

	struct SceneEntity
	{
		uint32_t index = 0;
		KAABBBox localBound;
		glm::mat4 prevTransform;
		glm::mat4 transform;
		std::vector<KMaterialSubMeshPtr> materialSubMeshes;
		std::vector<uint32_t> subMeshIndices;
		std::vector<uint32_t> materialIndices;
		std::vector<uint32_t> megaShaderIndices;
		std::vector<uint32_t> shaderLocalIndices;
	};
	typedef std::shared_ptr<SceneEntity> SceneEntityPtr;
	std::unordered_map<IKEntity*, SceneEntityPtr> m_EntityMap;
	std::vector<SceneEntityPtr> m_Entities;

	struct MegaBufferCollection
	{
		IKStorageBufferPtr vertexBuffers[VF_COUNT] = { nullptr };
		IKStorageBufferPtr indexBuffer = nullptr;
	} m_MegaBuffer;

	IKTexturePtr m_TextureArrays[MAX_MATERIAL_TEXTURE_BINDING];

	std::vector<IKStorageBufferPtr> m_SceneStateBuffers;
	std::vector<IKStorageBufferPtr> m_InstanceDataBuffers;
	std::vector<IKStorageBufferPtr> m_GroupDataBuffers;
	std::vector<IKStorageBufferPtr> m_IndirectArgsBuffers;
	std::vector<IKStorageBufferPtr> m_InstanceCullResultBuffers;
	std::vector<IKStorageBufferPtr> m_MegaShaderStateBuffers;

	std::vector<IKComputePipelinePtr> m_InitStatePipelines;
	std::vector<IKComputePipelinePtr> m_InstanceCullPipelines;
	std::vector<IKComputePipelinePtr> m_GroupAllocatePipelines;
	std::vector<IKComputePipelinePtr> m_GroupScatterPipelines;

	enum DataDirtyBitMask
	{
		MEGA_BUFFER_DIRTY = 1,
		TEXTURE_ARRAY_DIRTY = 2,
		MEGA_SHADER_DIRTY = 4,
		INSTANCE_DIRTY = 8
	};
	uint32_t m_DataDirtyBits = 0;

	KShaderCompileEnvironment m_ComputeBindingEnv;

	EntityObserverFunc m_OnSceneChangedFunc;
	void OnSceneChanged(EntitySceneOp op, IKEntity* entity);

	RenderComponentObserverFunc m_OnRenderComponentChangedFunc;
	void OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init);

	uint64_t ComputeHashByMaterialSubMesh(KSubMeshPtr subMesh, IKMaterialPtr material);

	uint32_t CreateOrGetSubMeshIndex(KSubMeshPtr subMesh, bool create);
	uint32_t CreateOrGetMaterialIndex(IKMaterialPtr material, bool create);
	uint32_t CreateOrGetMegaShaderIndex(KSubMeshPtr subMesh, IKMaterialPtr material, bool create);

	void RemoveSubMesh(KSubMeshPtr subMesh);
	void RemoveMaterial(IKMaterialPtr material);
	void RemoveMegaShader(KSubMeshPtr subMesh, IKMaterialPtr material);

	SceneEntityPtr CreateEntity(IKEntity* entity);
	SceneEntityPtr GetEntity(IKEntity* entity);

	bool AddEntity(IKEntity* entity, const KAABBBox& localBound, const glm::mat4& transform, const std::vector<KMaterialSubMeshPtr>& subMeshes);
	bool TransformEntity(IKEntity* entity, const glm::mat4& transform);
	bool RemoveEntity(IKEntity* entity);

	void RebuildDirtyBuffer();
	void UpdateInstanceDataBuffer();

	void RebuildEntityDataIndex();
	void RebuildMegaBuffer();
	void RebuildMegaShaderState();
	void RebuildTextureArray();
	void RebuildInstance();
	void RebuildParameter();

	void InitializeBuffers();
	void InitializePipelines();
public:
	KGPUScene();
	~KGPUScene();

	bool Init(IKRenderScene* scene, const KCamera* camera);
	bool UnInit();

	bool Execute(IKCommandBufferPtr primaryBuffer);

	void ReloadShader();
};