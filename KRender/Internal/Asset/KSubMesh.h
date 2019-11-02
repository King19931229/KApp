#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderCommand.h"

class KSubMesh
{
	friend class KMesh;
protected:
	KMesh*		m_pParent;
	KVertexData m_VertexData;
	KIndexData	m_IndexData;
	bool		m_IndexDraw;

	bool GetRenderCommand(PipelineStage stage, IKRenderTarget* target, KRenderCommand& command);
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData& vertexData, const KIndexData& indexData);
	bool UnInit();
	
	bool AppendRenderList(PipelineStage stage, IKRenderTarget* target, KRenderCommandList& list);
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;