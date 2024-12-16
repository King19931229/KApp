#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderStage.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Internal/Asset/KSubMesh.h"
#include "Internal/ShaderMap/KShaderMap.h"

#include <functional>

class KMaterialSubMesh
{
protected:
	KSubMeshPtr				m_SubMesh;
	KMaterialRef			m_Material;

	IKPipelinePtr			m_Pipelines[RENDER_STAGE_COUNT];
	bool					m_PipelinesPreqReady[RENDER_STAGE_COUNT];

	KShaderRef				m_DebugVSShader;
	KShaderRef				m_DebugFSShader;

	KShaderRef				m_ShadowVSShader;
	KShaderRef				m_ShadowFSShader;

	KShaderRef				m_VoxelVSShader;
	KShaderRef				m_VoxelGSShader;
	KShaderRef				m_VoxelFSShader;
	KShaderRef				m_VoxelSparseFSShader;

	KShaderRef				m_VoxelClipmapVSShader;
	KShaderRef				m_VoxelClipmapGSShader;
	KShaderRef				m_VoxelClipmapFSShader;

	bool CreateFixedPipeline(RenderStage stage, IKPipelinePtr& pipeline);

	bool CreateMaterialPipeline();
	bool CreateGBufferPipeline();
	bool CreateVirtualFeedbackPipeline();
	bool CreateShadowPipeline();
	bool CreateVoxelPipeline();

	bool SetupMaterialGeneratedCode(std::string& code);
public:
	KMaterialSubMesh();
	~KMaterialSubMesh();
	bool Init(KSubMeshPtr subMesh, KMaterialRef material);
	bool InitDebug(KSubMeshPtr subMesh, DebugPrimitive primtive);
	bool UnInit();
	bool GetRenderCommand(RenderStage stage, KRenderCommand& command);

	inline KSubMeshPtr GetSubMesh() { return m_SubMesh; }
	inline KMaterialRef GetMaterial() { return m_Material; }
};

typedef std::shared_ptr<KMaterialSubMesh> KMaterialSubMeshPtr;