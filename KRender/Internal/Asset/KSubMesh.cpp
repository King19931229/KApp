#include "KSubMesh.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"

KSubMesh::KSubMesh(KMesh* parent)
	: m_pParent(parent),
	m_pMaterial(nullptr),
	m_DebugPrimitive(DEBUG_PRIMITIVE_TRIANGLE),
	m_pVertexData(nullptr),
	m_AccelerationStructure(nullptr),
	m_FrameInFlight(0),
	m_IndexDraw(true),
	m_NeedAccelerationStructure(true)
{
}

KSubMesh::~KSubMesh()
{
}

bool KSubMesh::Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialTextureBinding&& binding, size_t frameInFlight)
{
	UnInit();

	m_pMaterial = nullptr;
	m_pVertexData = vertexData;
	m_IndexData = indexData;
	m_FrameInFlight = frameInFlight;
	m_Texture = std::move(binding);
	m_IndexDraw = true;

	if (m_NeedAccelerationStructure)
	{
		CreateAccelerationStructure();
	}

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
	m_Texture.Clear();
	SAFE_UNINIT(m_AccelerationStructure);

	return true;
}

bool KSubMesh::CreateAccelerationStructure()
{
	m_NeedAccelerationStructure = true;
	if (m_AccelerationStructure)
		return true;

	KRenderGlobal::RenderDevice->CreateAccelerationStructure(m_AccelerationStructure);

	if (m_IndexData.indexBuffer->GetIndexType() == IT_16)
	{
		IKIndexBufferPtr newIndexBuffer = nullptr;

		KRenderGlobal::RenderDevice->CreateIndexBuffer(newIndexBuffer);
		size_t count = m_IndexData.indexBuffer->GetIndexCount();

		std::vector<uint16_t> srcData; srcData.resize(count);
		ASSERT_RESULT(m_IndexData.indexBuffer->Read(srcData.data()));

		std::vector<uint32_t> destData; destData.resize(count);
		for (size_t i = 0; i < count; ++i) destData[i] = srcData[i];

		ASSERT_RESULT(newIndexBuffer->InitMemory(IT_32, count, destData.data()));
		ASSERT_RESULT(newIndexBuffer->InitDevice(m_IndexData.indexBuffer->IsHostVisible()));
		SAFE_UNINIT(m_IndexData.indexBuffer);
		m_IndexData.indexBuffer = newIndexBuffer;
	}

	 m_AccelerationStructure->InitBottomUp(m_pVertexData->vertexFormats[0], m_pVertexData->vertexBuffers[0], m_IndexData.indexBuffer);

	return true;
}

bool KSubMesh::DestroyAccelerationStructure()
{
	m_NeedAccelerationStructure = false;
	if (!m_AccelerationStructure)
		return true;
	SAFE_UNINIT(m_AccelerationStructure);

	return true;
}