#pragma once
#include "Internal/KVertexDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Internal/Asset/Material/KMaterialShader.h"
#include "Internal/ShaderMap/KShaderMap.h"

#include "KSubMesh.h"

#include <functional>

class KMaterialSubMesh
{
protected:
	KSubMesh*				m_pSubMesh;
	IKMaterial*				m_pMaterial;

	IKPipelinePtr			m_Pipelines[PIPELINE_STAGE_COUNT];

	IKShaderPtr				m_DebugVSShader;
	IKShaderPtr				m_DebugFSShader;

	IKShaderPtr				m_ShadowVSShader;
	IKShaderPtr				m_ShadowFSShader;

	IKShaderPtr				m_CascadedShadowStaticVSShader;
	IKShaderPtr				m_CascadedShadowStaticVSInstanceShader;

	IKShaderPtr				m_CascadedShadowDynamicVSShader;
	IKShaderPtr				m_CascadedShadowDynamicVSInstanceShader;

	IKShaderPtr				m_VoxelVSShader;
	IKShaderPtr				m_VoxelGSShader;
	IKShaderPtr				m_VoxelFSShader;
	IKShaderPtr				m_VoxelSparseFSShader;

	IKShaderPtr				m_VoxelClipmapVSShader;
	IKShaderPtr				m_VoxelClipmapGSShader;
	IKShaderPtr				m_VoxelClipmapFSShader;

	// TODO 合并到同一个Shader里实现
	KShaderMap				m_MaterialShaderMap;
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