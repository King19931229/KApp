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
	m_Path = szPath;
	return true;
}

bool KMesh::UnInit()
{
	return true;
}

bool KMesh::AppendRenderList(PipelineStage stage, size_t frameIndex, KRenderCommandList& list)
{
	for(KSubMeshPtr subMesh : m_SubMeshes)
	{
		ASSERT_RESULT(subMesh->AppendRenderList(stage, frameIndex, list));
	}
	return true;
}