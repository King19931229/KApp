#pragma once
#include "Internal/KVertexDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Internal/ShaderMap/KShaderMap.h"

#include "KSubMesh.h"

#include <functional>

class KMaterialSubMesh
{
protected:
	KSubMesh*				m_pSubMesh;
	IKMaterial*				m_pMaterial;

	IKPipelinePtr			m_Pipelines[PIPELINE_STAGE_COUNT];

	KShaderRef				m_DebugVSShader;
	KShaderRef				m_DebugFSShader;

	KShaderRef				m_ShadowVSShader;
	KShaderRef				m_ShadowFSShader;

	KShaderRef				m_CascadedShadowStaticVSShader;
	KShaderRef				m_CascadedShadowStaticVSInstanceShader;

	KShaderRef				m_CascadedShadowDynamicVSShader;
	KShaderRef				m_CascadedShadowDynamicVSInstanceShader;

	KShaderRef				m_VoxelVSShader;
	KShaderRef				m_VoxelGSShader;
	KShaderRef				m_VoxelFSShader;
	KShaderRef				m_VoxelSparseFSShader;

	KShaderRef				m_VoxelClipmapVSShader;
	KShaderRef				m_VoxelClipmapGSShader;
	KShaderRef				m_VoxelClipmapFSShader;

	KShaderMap				m_PrePassShaderMap;

	bool					m_MateriaShaderTriggerLoaded;
	bool					m_MaterialPipelineCreated;

	bool CreateFixedPipeline(PipelineStage stage, IKPipelinePtr& pipeline);
	bool GetRenderCommand(PipelineStage stage, KRenderCommand& command);

	bool CreateFixedPipeline();
	bool CreateMaterialPipeline();
	bool CreateGBufferPipeline();
	bool CreateVoxelPipeline();

	bool SetupMaterialGeneratedCode(std::string& code);
public:
	KMaterialSubMesh(KSubMesh* subMesh);
	~KMaterialSubMesh();
	bool Init(IKMaterial* material);
	bool InitDebug(DebugPrimitive primtive);
	bool UnInit();
	bool Visit(PipelineStage stage, std::function<void(KRenderCommand&)> func);
};

typedef std::shared_ptr<KMaterialSubMesh> KMaterialSubMeshPtr;