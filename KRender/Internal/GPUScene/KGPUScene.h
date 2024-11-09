#pragma once
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKRenderScene.h"
#include "Internal/Asset/KMaterialSubMesh.h"
#include "Internal/Render/KRHICommandList.h"

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
	uint32_t slice[MAX_MATERIAL_TEXTURE_BINDING] = { 0 };
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
		GPUSCENE_GROUP_SIZE = 64,
		MAX_TEXTURE_ARRAY_NUM = 6
	};

	IKRenderScene* m_Scene;
	const KCamera* m_Camera;

	struct SubMeshItem
	{
		KSubMeshPtr subMesh;
		uint32_t vertexBufferOffset[VF_SCENE_COUNT] = { 0 };
		uint32_t vertexBufferSize[VF_SCENE_COUNT] = { 0 };
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
		std::vector<IKPipelinePtr> darwPipelines[GPUSCENE_RENDER_STAGE_COUNT];
		std::vector<IKStorageBufferPtr> parametersBuffers;
		std::vector<IKStorageBufferPtr> materialIndicesBuffers;
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
		std::vector<uint32_t> shaderDrawIndices;
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

	std::vector<KGPUSceneMaterialTextureBinding> m_MaterialTextureBindings;

	struct TextureArrayCollection
	{
		KSamplerRef samplers[MAX_TEXTURE_ARRAY_NUM];
		uint32_t dimension[MAX_TEXTURE_ARRAY_NUM] = { 0 };
		IKTexturePtr textureArrays[MAX_TEXTURE_ARRAY_NUM] = { nullptr };
		IKStorageBufferPtr materialTextureBindingBuffer = nullptr;
	} m_TextureArray;

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
		TEXTURE_ARRAY_DIRTY = 2,
		MEGA_SHADER_DIRTY = 4,
		INSTANCE_DIRTY = 8
	};
	uint32_t m_DataDirtyBits;

	bool m_Enable;

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
	void RebuildMeshStateBuffer();
	void RebuildMegaShaderState();
	void RebuildTextureArray();
	void RebuildInstance();
	void RebuildMegaShaderBuffer();

	void InitializeBuffers();
	void InitializePipelines();

	bool& IsEnable() { return m_Enable; }
public:
	KGPUScene();
	~KGPUScene();

	bool Init(IKRenderScene* scene, const KCamera* camera);
	bool UnInit();

	bool Execute(KRHICommandList& commandList);

	bool BasePassMain(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool BasePassPost(IKRenderPassPtr renderPass, KRHICommandList& commandList);

	void ReloadShader();

	bool& GetEnable() { return m_Enable; }
};