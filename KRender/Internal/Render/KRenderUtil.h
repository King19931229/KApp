#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/KMaterial.h"
#include "Internal/Asset/KMaterialSubMesh.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/VirtualTexture/KVirtualTexture.h"

class KMeshLODView;

struct KMaterialSubMeshInstance
{
	KMaterialSubMeshPtr materialSubMesh;
	std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F> instanceData;
	uint32_t lodLevel = 0;
};

typedef std::function<bool(const KMaterialSubMeshInstance&, const KMaterialSubMeshInstance&)> KMaterialSubMeshInstanceCompareFunction;

namespace KRenderUtil
{
	void UnRegisterRenderComponentCallback(RenderComponentObserverFunc* callback);

	void CalculateInstancesByMesh(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances);
	void CalculateInstancesByMaterial(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances, const KMeshLODView* lodView = nullptr);
	void CalculateInstancesByVirtualTexture(const std::vector<IKEntity*>& entities, uint32_t targetBinding, IKTexturePtr virtualTexture, std::vector<KMaterialSubMeshInstance>& instances, const KMeshLODView& lodView);

	void GetInstances(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances, RenderStage stage, KMaterialSubMeshInstanceCompareFunction comp);

	bool AssignRenderStageBinding(KRenderCommand& command, RenderStage renderStage, uint32_t debugOption);
	bool AssignShadingParameter(KRenderCommand& command, KMaterialRef material);
	bool AssignVirtualFeedbackParameter(KRenderCommand& command, uint32_t targetBinding, KVirtualTexture* virtualTexture);

	bool PopulateRenderCommandList(IKRenderPassPtr renderPass, const std::vector<IKEntity*>& entities, KRenderCommandList& renderCommands, RenderStage renderStage, RenderStage instanceRenderStage = RENDER_STAGE_UNKNOWN);
};
