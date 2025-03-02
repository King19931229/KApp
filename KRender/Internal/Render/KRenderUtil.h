#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/KMaterial.h"
#include "Internal/Asset/KMaterialSubMesh.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/VirtualTexture/KVirtualTexture.h"

struct KMaterialSubMeshInstance
{
	KMaterialSubMeshPtr materialSubMesh;
	std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F> instanceData;
};

typedef std::function<bool(const KMaterialSubMeshInstance&, const KMaterialSubMeshInstance&)> KMaterialSubMeshInstanceCompareFunction;

namespace KRenderUtil
{
	void CalculateInstancesByMesh(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances);
	void CalculateInstancesByMaterial(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances);
	void CalculateInstancesByVirtualTexture(const std::vector<IKEntity*>& entities, uint32_t targetBinding, IKTexturePtr virtualTexture, std::vector<KMaterialSubMeshInstance>& instances);

	void GetInstances(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances, KMaterialSubMeshInstanceCompareFunction comp);

	bool AssignRenderStageBinding(KRenderCommand& command, RenderStage renderStage, uint32_t debugOption);
	bool AssignShadingParameter(KRenderCommand& command, KMaterialRef material);
	bool AssignVirtualFeedbackParameter(KRenderCommand& command, uint32_t targetBinding, KVirtualTexture* virtualTexture);
};