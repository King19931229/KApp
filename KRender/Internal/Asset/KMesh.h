#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderCommand.h"
#include "KSubMesh.h"

class KMesh
{
	friend class KSubMesh;
protected:
	KVertexData m_VertexData;
	std::vector<KSubMeshPtr> m_SubMeshes;
public:
	KMesh();
	~KMesh();

	bool Load(const char* szPath);
	bool Save(const char* szPath);

	bool AppendRenderList(PipelineStage stage, IKRenderTarget* target, KRenderCommandList& list);
};