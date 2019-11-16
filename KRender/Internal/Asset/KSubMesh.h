#pragma once
#include "Internal/KVertexDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "KMaterial.h"

class KSubMesh
{
	friend class KMesh;
protected:
	typedef std::vector<IKPipelinePtr> PipelineList;
	typedef std::vector<PipelineList> FramePipelineList;
	
	KMesh*					m_pParent;
	KMaterialPtr			m_Material;

	const KVertexData*		m_pVertexData;
	KIndexData				m_IndexData;
	bool					m_IndexDraw;

	size_t					m_FrameInFlight;
	size_t					m_RenderThreadNum;
	FramePipelineList		m_Pipelines[PIPELINE_STAGE_COUNT];

	PushConstant			m_ObjectConstant;
	PushConstantLocation	m_ObjectConstantLoc;

	IKShaderPtr				m_SceneVSShader;
	IKShaderPtr				m_SceneFSShader;

	bool CreatePipeline(PipelineStage stage, size_t frameIndex, size_t renderThreadIndex, IKPipelinePtr& pipeline);
	bool GetRenderCommand(PipelineStage stage, size_t frameIndex, size_t renderThreadIndex, KRenderCommand& command);
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialPtr material, size_t frameInFlight, size_t renderThreadNum);
	bool UnInit();

	bool AppendRenderList(PipelineStage stage, size_t frameIndex, size_t threadIndex, KRenderCommandList& list);
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;