#include "KSubMesh.h"
#include "KMesh.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"

KSubMesh::KSubMesh(KMesh* parent)
	: m_pParent(parent),
	m_IndexDraw(true)
{

}

KSubMesh::~KSubMesh()
{

}

bool KSubMesh::Init(const KVertexData& vertexData, const KIndexData& indexData)
{
	m_VertexData = vertexData;
	m_IndexData = indexData;
	return true;
}

bool KSubMesh::UnInit()
{
	m_VertexData.Destroy();
	m_IndexData.Destroy();
	return true;
}

bool KSubMesh::GetRenderCommand(PipelineStage stage, IKRenderTarget* target, KRenderCommand& command)
{
	return false;
	// TODO Get it from Pipeline Manager
}

bool KSubMesh::AppendRenderList(PipelineStage stage, IKRenderTarget* target, KRenderCommandList& list)
{
	KRenderCommand command;
	if(GetRenderCommand(stage, target, command))
	{
		list.push_back(std::move(command));
		return true;
	}

	return false;
}