#include "KMesh.h"

KMesh::KMesh()
{
}

KMesh::~KMesh()
{
}

bool KMesh::SaveAsFile(const char* szPath)
{
	return false;
}

bool KMesh::InitFromFile(const char* szPath, size_t frameInFlight)
{
	// TODO

	for(SubMeshData& subMeshData : m_SubMeshes)
	{
		ASSERT_RESULT(subMeshData.subMesh != nullptr);
		ASSERT_RESULT(subMeshData.subMesh->Init(m_VertexData, subMeshData.indexData, frameInFlight));
	}
	return true;
}

bool KMesh::UnInit()
{
	for(SubMeshData& subMeshData : m_SubMeshes)
	{
		ASSERT_RESULT(subMeshData.subMesh != nullptr);
		ASSERT_RESULT(subMeshData.subMesh->UnInit());
	}
	m_SubMeshes.clear();

	if(m_Material)
	{
		m_Material = nullptr;
	}

	return true;
}

KMaterialPtr KMesh::GetMaterial()
{
	return m_Material;
}

bool KMesh::AppendRenderList(PipelineStage stage, size_t frameIndex, KRenderCommandList& list)
{
	for(SubMeshData& subMeshData : m_SubMeshes)
	{
		ASSERT_RESULT(subMeshData.subMesh != nullptr);
		ASSERT_RESULT(subMeshData.subMesh->AppendRenderList(stage, frameIndex, list));
	}
	return true;
}