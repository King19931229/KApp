#pragma once
#include "Internal/KVertexDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "KMaterial.h"

#include <functional>

class KMesh;
class KSubMesh
{
	friend class KMesh;
	friend class KMeshSerializerV0;
protected:
	struct PipelineInfo
	{
		IKPipelinePtr pipeline;
		uint32_t objectPushOffset;
	};

	typedef std::vector<PipelineInfo> PipelineList;
	typedef std::vector<PipelineList> FramePipelineList;
	
	KMesh*					m_pParent;
	KMaterialPtr			m_Material;

	const KVertexData*		m_pVertexData;
	KIndexData				m_IndexData;
	bool					m_IndexDraw;

	size_t					m_FrameInFlight;
	size_t					m_RenderThreadNum;
	FramePipelineList		m_Pipelines[PIPELINE_STAGE_COUNT];

	IKShaderPtr				m_SceneVSShader;
	IKShaderPtr				m_SceneFSShader;

	IKShaderPtr				m_ShadowVSShader;
	IKShaderPtr				m_ShadowFSShader;

	bool CreatePipeline(PipelineStage stage, size_t frameIndex, size_t renderThreadIndex, IKPipelinePtr& pipeline, uint32_t& objectPushOffset);
	bool GetRenderCommand(PipelineStage stage, size_t frameIndex, size_t renderThreadIndex, KRenderCommand& command);
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialPtr material, size_t frameInFlight, size_t renderThreadNum);
	bool UnInit();

	bool Visit(PipelineStage stage, size_t frameIndex, size_t threadIndex, std::function<void(KRenderCommand)> func);
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;