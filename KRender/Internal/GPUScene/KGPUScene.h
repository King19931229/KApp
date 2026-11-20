#pragma once
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKRenderScene.h"
#include "Internal/Asset/KMaterialSubMesh.h"
#include "Internal/Render/KRHICommandList.h"

class KMeshLODView;

struct KGPUSceneInstance
{
	glm::mat4 transform;
	glm::mat4 prevTransform;
	glm::vec4 boundCenter;
	glm::vec4 boundHalfExtend;
	enum
	{
		ENEITY_INDEX = 0,
		SUBMESH_INDEX,
		MEGA_SHADER_INDEX,
		SHADER_DRAW_INDEX
	};
	glm::uvec4 miscs;
};
static_assert((sizeof(KGPUSceneInstance) % 16) == 0, "Size must be a multiple of 16");

struct KGPUSceneMeshState
{
	glm::vec4 boundCenter;
	glm::vec4 boundHalfExtend;
	enum
	{
		INDEX_COUNT_INDEX,
		VERTEX_COUNT_INDEX,
		INDEX_BUFFER_OFFSET,
		VERTEX_BUFFER_OFFSET
	};
	uint32_t miscs[16];
	static_assert(VERTEX_BUFFER_OFFSET + VF_SCENE_COUNT <= 16, "Must not overflow");
};
static_assert((sizeof(KGPUSceneMeshState) % 16) == 0, "Size must be a multiple of 16");

struct KGPUSceneState
{
	uint32_t megaShaderNum = 0;
	uint32_t instanceCount = 0;
	uint32_t groupAllocateOffset = 0;
	uint32_t padding = 0;
};
static_assert((sizeof(KGPUSceneState) % 16) == 0, "Size must be a multiple of 16");

struct KGPUSceneMaterialTextureBinding
{
	uint32_t binding[MAX_MATERIAL_TEXTURE_BINDING] = { 0 };
};
static_assert((sizeof(KGPUSceneMaterialTextureBinding) % 16) == 0, "Size must be a multiple of 16");

struct KGPUSceneMegaShaderState
{
	uint32_t instanceCount = 0;
	uint32_t groupWriteOffset = 0;
	uint32_t groupWriteNum = 0;
	uint32_t padding = 0;
};
static_assert((sizeof(KGPUSceneMegaShaderState) % 16) == 0, "Size must be a multiple of 16");

struct KGPUSceneDrawParameter
{
	uint32_t megaShaderIndex = 0;
};

enum GPUSceneBinding
{
	#define GPUSCENE_BINDING(SEMANTIC) GPUSCENE_BINDING_##SEMANTIC,
	#include "KGPUSceneBinding.inl"
	#undef GPUSCENE_BINDING
};

enum GPUSceneRenderStage
{
	GPUSCENE_RENDER_STAGE_BASEPASS_MAIN,
	GPUSCENE_RENDER_STAGE_BASEPASS_POST,
	GPUSCENE_RENDER_STAGE_COUNT
};

class KGPUScene
{
protected:
	enum
	{
		GPUSCENE_GROUP_SIZE = 64
	};

	IKRenderScene* m_Scene;

	struct SubMeshItem
	{
		KSubMeshPtr subMesh;
		uint32_t vertexBufferOffset[VF_SCENE_COUNT] = { 0 };
		uint32_t vertexCount = 0;
		uint32_t indexBufferOffset = 0;
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
		std::vector<IKPipelinePtr> darwPipelines[GPUSCENE_RENDER_STAGE_COUNT];
		std::vector<IKStorageBufferPtr> parametersBuffers;
		IKStorageBufferPtr materialIndicesBuffer;
		std::vector<IKStorageBufferPtr> indirectDrawArgsBuffers;
		std::vector<IKComputePipelinePtr> calcDrawArgsPipelines;
		std::vector<uint32_t> materialIndices;
		uint32_t parameterDataSize = 0;
		uint32_t parameterDataCount = 0;
		uint32_t drawArgsDataCount = 0;
		uint32_t refCount = 0;
	};
	std::vector<MegaShaderItem> m_MegaShaders;
	std::unordered_map<uint64_t, uint32_t> m_MegaShaderToIndex;

	std::vector<KGPUSceneInstance> m_Instances;
	std::vector<KGPUSceneMeshState> m_MeshStates;
	uint64_t m_MegaBufferRebuilds = 0;
	uint32_t m_LastDrawFrame = UINT32_MAX;

	struct EntitySubMesh
	{
		KMaterialSubMeshPtr binding;
		size_t slot = 0;
		std::array<uint32_t, KSubMesh::LOD_COUNT> meshIndices{};
		uint32_t megaShaderIndex = 0;
		uint32_t shaderDrawIndex = 0;
	};

	struct SceneEntity
	{
		IKEntity::IDType id = 0;
		uint32_t index = 0;
		KAABBBox localBound;
		glm::mat4 prevTransform;
		glm::mat4 transform;
		std::vector<EntitySubMesh> subMeshes;
	};
	typedef std::shared_ptr<SceneEntity> SceneEntityPtr;
	std::unordered_map<IKEntity*, SceneEntityPtr> m_EntityMap;
	std::vector<SceneEntityPtr> m_Entities;

	struct MegaBufferCollection
	{
		IKStorageBufferPtr vertexBuffers[VF_COUNT] = { nullptr };
		IKStorageBufferPtr indexBuffer = nullptr;
		IKStorageBufferPtr meshStateBuffer = nullptr;
	} m_MegaBuffer;

	IKStorageBufferPtr m_MaterialTextureBindingBuffer;

	std::vector<IKStorageBufferPtr> m_SceneStateBuffers;
	std::vector<IKStorageBufferPtr> m_InstanceDataBuffers;
	std::vector<IKStorageBufferPtr> m_GroupDataBuffers;
	std::vector<IKStorageBufferPtr> m_DispatchArgsBuffers;
	std::vector<IKStorageBufferPtr> m_InstanceCullResultBuffers;
	std::vector<IKStorageBufferPtr> m_MegaShaderStateBuffers;

	std::vector<IKComputePipelinePtr> m_InitStatePipelines;
	std::vector<IKComputePipelinePtr> m_InstanceCullPipelines;
	std::vector<IKComputePipelinePtr> m_CalcDispatchArgsPipelines;
	std::vector<IKComputePipelinePtr> m_GroupAllocatePipelines;
	std::vector<IKComputePipelinePtr> m_GroupScatterPipelines;

	KShaderCompileEnvironment m_ComputeBindingEnv;

	enum DataDirtyBitMask
	{
		MEGA_BUFFER_DIRTY = 1,
		TEXTURE_BINDINGS_DIRTY = 2,
		MEGA_SHADER_DIRTY = 4,
		INSTANCE_DIRTY = 8
	};
	uint32_t m_DataDirtyBits;

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
	void UpdateInstanceDataBuffer(KRHICommandList& commandList, const KMeshLODView& lodView);

	void RebuildEntityDataIndex();
	void RebuildMegaBuffer();
	void RebuildMeshStateBuffer();
	void RebuildMegaShaderState();
	void RebuildTextureBindings();
	void RebuildInstance();
	void RebuildMegaShaderBuffer();

	void InitializeBuffers();
	void InitializePipelines();
public:
	KGPUScene();
	~KGPUScene();

	bool Init(IKRenderScene* scene);
	bool UnInit();

	bool Execute(KRHICommandList& commandList, IKRenderScene* scene, const KCamera& camera, const KMeshLODView& lodView);

	void AppendBasePassCommands(IKRenderPassPtr renderPass, KRHICommandList& commandList, KRenderCommandList& commands);
	uint64_t GetMegaBufferRebuilds() const { return m_MegaBufferRebuilds; }
	bool ReadBasePassTriangles(uint64_t& triangles) const;

	void Reload();
};
