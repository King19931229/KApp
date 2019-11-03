#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderCommand.h"
#include "KMaterial.h"
#include "KSubMesh.h"

class KMesh
{
	friend class KSubMesh;
protected:
	KVertexData m_VertexData;
	KMaterialPtr m_Material;

	struct SubMeshData
	{
		KSubMeshPtr subMesh;
		KIndexData indexData;
	};
	std::vector<SubMeshData> m_SubMeshes;
public:
	KMesh();
	~KMesh();

	bool SaveAsFile(const char* szPath);

	bool InitFromFile(const char* szPath, size_t frameInFlight);
	bool UnInit();

	KMaterialPtr GetMaterial();

	bool AppendRenderList(PipelineStage stage, size_t frameIndex, KRenderCommandList& list);
};