#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderCommand.h"
#include "KMaterial.h"

class KSubMesh
{
	friend class KMesh;
protected:
	typedef std::vector<IKPipelinePtr> FramePipelineList;

	KMesh*					m_pParent;
	KMaterialPtr			m_Material;

	const KVertexData*		m_pVertexData;
	KIndexData				m_IndexData;
	bool					m_IndexDraw;

	size_t					m_FrameInFlight;
	FramePipelineList		m_Pipelines[PIPELINE_STAGE_COUNT];

	PushConstant			m_ObjectConstant;
	PushConstantLocation	m_ObjectConstantLoc;

	IKShaderPtr				m_SceneVSShader;
	IKShaderPtr				m_SceneFSShader;

	bool CreatePipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline);
	bool GetRenderCommand(PipelineStage stage, size_t frameIndex, KRenderCommand& command);
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, size_t frameInFlight);
	bool UnInit();

	bool ResignMaterial(KMaterialPtr material);
	
	bool AppendRenderList(PipelineStage stage, size_t frameIndex, KRenderCommandList& list);
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;