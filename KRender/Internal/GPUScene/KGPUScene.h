#pragma once
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKRenderScene.h"
#include "Internal/Asset/KMaterialSubMesh.h"

class KGPUScene
{
protected:
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
		std::vector<IKStorageBufferPtr> shaderParameterBuffer;
		uint32_t shaderParameterIndexCounter = 0;
		uint32_t refCount = 0;
	};
	std::vector<MegaShaderItem> m_MegaShaders;
	std::unordered_map<uint64_t, uint32_t> m_MegaShaderToIndex;

	struct SceneEntity
	{
		uint32_t index = 0;
		glm::mat4 prevTransform;
		glm::mat4 transform;
		std::vector<KMaterialSubMeshPtr> materialSubMeshes;
		std::vector<uint32_t> subMeshIndices;
		std::vector<uint32_t> materialIndices;
		std::vector<uint32_t> megaShaderIndices;
		std::vector<uint32_t> shaderParameterIndices;
	};
	typedef std::shared_ptr<SceneEntity> SceneEntityPtr;
	std::unordered_map<IKEntity*, SceneEntityPtr> m_EntityMap;
	std::vector<SceneEntityPtr> m_Entities;

	struct MegaBufferCollection
	{
		IKVertexBufferPtr vertexBuffers[VF_COUNT] = { nullptr };
		IKIndexBufferPtr indexBuffer = nullptr;
	} m_MegaBuffer;

	IKTexturePtr m_TextureArrays[MAX_MATERIAL_TEXTURE_BINDING];

	bool m_SceneDirty;

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

	bool AddEntity(IKEntity* entity, const glm::mat4& transform, const std::vector<KMaterialSubMeshPtr>& subMeshes);
	bool TransformEntity(IKEntity* entity, const glm::mat4& transform);
	bool RemoveEntity(IKEntity* entity);

	void RebuildEntitySubMeshAndMaterialIndex();
	void RebuildMegaBuffer();
	void RebuildTextureArray();
public:
	KGPUScene();
	~KGPUScene();

	bool Init(IKRenderScene* scene, const KCamera* camera);
	bool UnInit();

	bool Execute(IKCommandBufferPtr primaryBuffer);

	void ReloadShader();
};