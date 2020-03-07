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
	typedef std::vector<IKPipelinePtr> FramePipelineList;

	KMesh*					m_pParent;
	KMaterialPtr			m_Material;

	const KVertexData*		m_pVertexData;
	KIndexData				m_IndexData;
	bool					m_IndexDraw;

	size_t					m_FrameInFlight;
	FramePipelineList		m_Pipelines[PIPELINE_STAGE_COUNT];

	IKShaderPtr				m_DebugVSShader;
	IKShaderPtr				m_DebugFSShader;

	IKShaderPtr				m_PreZVSShader;
	IKShaderPtr				m_PreZFSShader;

	IKShaderPtr				m_SceneVSShader;
	IKShaderPtr				m_SceneFSShader;

	IKShaderPtr				m_ShadowVSShader;
	IKShaderPtr				m_ShadowFSShader;

	bool CreatePipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline);
	bool GetRenderCommand(PipelineStage stage, size_t frameIndex,KRenderCommand& command);
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialPtr material, size_t frameInFlight);
	bool InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData, size_t frameInFlight);
	bool UnInit();

	bool Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand&&)> func);
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;