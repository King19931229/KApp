#pragma once
#include "Internal/KVertexDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "KSubMesh.h"

#include <functional>

class KMaterialSubMesh
{
protected:
	typedef std::vector<IKPipelinePtr> FramePipelineList;

	KSubMesh*				m_pSubMesh;
	IKMaterial*				m_pMaterial;

	size_t					m_FrameInFlight;
	FramePipelineList		m_Pipelines[PIPELINE_STAGE_COUNT];

	IKShaderPtr				m_DebugVSShader;
	IKShaderPtr				m_DebugFSShader;

	IKShaderPtr				m_PreZVSShader;
	IKShaderPtr				m_PreZFSShader;

	IKShaderPtr				m_GBufferVSShader;
	IKShaderPtr				m_GBufferVSInstanceShader;
	IKShaderPtr				m_GBufferFSShader;

	IKShaderPtr				m_ShadowVSShader;
	IKShaderPtr				m_ShadowFSShader;

	IKShaderPtr				m_CascadedShadowVSShader;
	IKShaderPtr				m_CascadedShadowVSInstanceShader;

	IKShaderPtr				m_VoxelVSShader;
	IKShaderPtr				m_VoxelGSShader;
	IKShaderPtr				m_VoxelFSShader;

	bool					m_MateriaShaderTriggerLoaded;
	bool					m_MaterialPipelineCreated;

	bool CreateFixedPipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline);
	bool GetRenderCommand(PipelineStage stage, size_t frameIndex, KRenderCommand& command);

	bool CreateFixedPipeline();
	bool CreateMaterialPipeline();
	bool CreateVoxelPipeline();
public:
	KMaterialSubMesh(KSubMesh* subMesh);
	~KMaterialSubMesh();
	bool Init(IKMaterial* material, size_t frameInFlight);
	bool InitDebug(DebugPrimitive primtive, size_t frameInFlight);
	bool UnInit();
	bool Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand&)> func);
};

typedef std::shared_ptr<KMaterialSubMesh> KMaterialSubMeshPtr;