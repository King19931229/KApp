#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/KMaterial.h"
#include "Internal/Asset/KMaterialSubMesh.h"
#include "Internal/ECS/Component/KRenderComponent.h"

struct KMaterialSubMeshInstance
{
	KMaterialSubMeshPtr materialSubMesh;
	std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F> instanceData;
};

namespace KRenderUtil
{
	void CalculateInstancesByMesh(const std::vector<KRenderComponent*>& renderComponents, std::vector<KMaterialSubMeshInstance>& instances);
	void CalculateInstanceByMaterial(const std::vector<KRenderComponent*>& renderComponents, std::vector<KMaterialSubMeshInstance>& instances);
	bool AssignShadingParameter(KRenderCommand& command, KMaterialRef material);
	bool AssignMeshStorageParameter(KRenderCommand& command);
};