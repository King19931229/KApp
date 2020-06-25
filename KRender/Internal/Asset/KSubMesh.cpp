#include "KSubMesh.h"
#include "KMesh.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKSampler.h"

#include "Internal/KRenderGlobal.h"

KSubMesh::KSubMesh(KMesh* parent)
	: m_pParent(parent),
	m_pMaterial(nullptr),
	m_DebugPrimitive(DEBUG_PRIMITIVE_TRIANGLE),
	m_pVertexData(nullptr),
	m_FrameInFlight(0),
	m_IndexDraw(true)
{
}

KSubMesh::~KSubMesh()
{
}

bool KSubMesh::Init(const KVertexData* vertexData, const KIndexData& indexData, KMeshTextureBinding&& binding, size_t frameInFlight)
{
	UnInit();

	m_pMaterial = nullptr;
	m_pVertexData = vertexData;
	m_IndexData = indexData;
	m_FrameInFlight = frameInFlight;
	m_Texture = std::move(binding);
	m_IndexDraw = true;

	return true;
}

bool KSubMesh::InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData, size_t frameInFlight)
{
	UnInit();

	m_pMaterial = nullptr;
	m_pVertexData = vertexData;
	m_FrameInFlight = frameInFlight;
	m_DebugPrimitive = primtive;

	if (indexData != nullptr)
	{
		m_IndexDraw = true;
		m_IndexData = *indexData;
	}
	else
	{
		m_IndexDraw = false;
	}

	return true;
}

bool KSubMesh::UnInit()
{
	m_pVertexData = nullptr;
	m_pMaterial = nullptr;
	m_IndexData.Destroy();
	m_FrameInFlight = 0;
	m_Texture.Release();

	return true;
}