#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderCommand.h"

class KSubMesh
{
	friend class KMesh;
protected:
	typedef std::vector<IKPipelinePtr> FramePipelineList;

	KMesh*					m_pParent;
	size_t					m_MaterialIndex;

	KVertexData				m_VertexData;
	KIndexData				m_IndexData;
	bool					m_IndexDraw;

	size_t					m_FrameInFlight;
	FramePipelineList		m_Pipelines[PIPELINE_STAGE_COUNT];

	PushConstant			m_ObjectConstant;
	PushConstantLocation	m_ObjectConstantLoc;

	bool CreatePipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline);
	bool GetRenderCommand(PipelineStage stage, size_t frameIndex, KRenderCommand& command);
public:
	KSubMesh(KMesh* parent, size_t mtlIndex);
	~KSubMesh();

	bool Init(const KVertexData& vertexData, const KIndexData& indexData, size_t frameInFlight);
	bool UnInit();
	
	bool AppendRenderList(PipelineStage stage, size_t frameIndex, KRenderCommandList& list);
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;