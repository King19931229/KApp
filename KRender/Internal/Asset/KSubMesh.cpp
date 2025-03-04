#include "KSubMesh.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"

KSubMesh::KSubMesh(KMesh* parent)
	: m_pParent(parent),
	m_DebugPrimitive(DEBUG_PRIMITIVE_TRIANGLE),
	m_pVertexData(nullptr),
	m_IndexDraw(true),
	m_AccelerationStructure(nullptr),
	m_NeedAccelerationStructure(true),
	m_NeedMeshlet(false)
{
}

KSubMesh::~KSubMesh()
{
}

bool KSubMesh::Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialRef material, const KAABBBox& bound, const std::string& debugLabel)
{
	UnInit();

	m_Material = material;
	m_pVertexData = vertexData;
	m_IndexData = indexData;
	m_IndexDraw = indexData.indexBuffer && indexData.indexCount > 0;
	m_Bound = bound;
	m_DebugLabel = debugLabel;

	if (m_NeedAccelerationStructure)
	{
		CreateAccelerationStructure();
	}

	if (m_NeedMeshlet)
	{
		CreateMeshlet();
	}

	return true;
}

bool KSubMesh::InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData, const KAABBBox& bound)
{
	UnInit();

	m_Material.Release();
	m_pVertexData = vertexData;
	m_DebugPrimitive = primtive;
	m_Bound = bound;

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
	m_Material.Release();
	m_IndexData.Destroy();
	DestroyAccelerationStructure();
	DestroyMeshlet();
	return true;
}

bool KSubMesh::CreateAccelerationStructure()
{
	if (m_AccelerationStructure)
	{
		return true;
	}

	if (m_IndexDraw)
	{
		KRenderGlobal::RenderDevice->CreateAccelerationStructure(m_AccelerationStructure);

		if (m_IndexData.indexBuffer->GetIndexType() == IT_16)
		{
			IKIndexBufferPtr indexBuffer = nullptr;

			KRenderGlobal::RenderDevice->CreateIndexBuffer(indexBuffer);
			size_t count = m_IndexData.indexBuffer->GetIndexCount();

			std::vector<uint16_t> srcData;
			srcData.resize(count);
			ASSERT_RESULT(m_IndexData.indexBuffer->Read(srcData.data()));

			std::vector<uint32_t> destData;
			destData.resize(count);
			for (size_t i = 0; i < count; ++i)
			{
				destData[i] = srcData[i];
			}

			ASSERT_RESULT(indexBuffer->InitMemory(IT_32, count, destData.data()));
			ASSERT_RESULT(indexBuffer->InitDevice(m_IndexData.indexBuffer->IsHostVisible()));
			SAFE_UNINIT(m_IndexData.indexBuffer);
			m_IndexData.indexBuffer = indexBuffer;
		}

		m_AccelerationStructure->SetDebugName((m_DebugLabel + "_BLAS").c_str());
		m_AccelerationStructure->InitBottomUp(m_pVertexData->vertexFormats[0], m_pVertexData->vertexBuffers[0], m_IndexData.indexBuffer);
	}
	else
	{
		KLog::Logger->Log(LL_ERROR, "No IndexBuffer No AccelerationStructure");
	}

	return true;
}

bool KSubMesh::DestroyAccelerationStructure()
{
	if (!m_AccelerationStructure)
	{
		return true;
	}
	SAFE_UNINIT(m_AccelerationStructure);
	return true;
}

bool KSubMesh::CreateMeshlet()
{
	if (m_IndexData.indexBuffer && m_pVertexData)
	{
		ASSERT_RESULT(m_pVertexData->vertexFormats[0] == VF_POINT_NORMAL_UV);

		KMeshletPackBasicBuilder buider;
		buider.Setup(32, 84, false);

		std::vector<char> indices;
		std::vector<char> vertices;

		indices.resize(m_IndexData.indexBuffer->GetBufferSize());
		vertices.resize(m_pVertexData->vertexBuffers[0]->GetBufferSize());

		ASSERT_RESULT(m_IndexData.indexBuffer->Read(indices.data()));
		ASSERT_RESULT(m_pVertexData->vertexBuffers[0]->Read(vertices.data()));

		uint32_t minVertex = UINT32_MAX;
		uint32_t maxVertex = 0;

		void* pIndices = indices.data();
		void* pVertices = vertices.data();

		if (m_IndexData.indexBuffer->GetIndexType() == IT_16)
		{
			pIndices = POINTER_OFFSET(pIndices, 2 * m_IndexData.indexStart);
			ASSERT_RESULT(buider.BuildMeshlets(m_Meshlet, m_IndexData.indexCount, (uint16_t*)pIndices) == m_IndexData.indexCount);
			for (size_t i = 0; i < m_IndexData.indexCount; ++i)
			{
				uint16_t vertex = *(uint16_t*)POINTER_OFFSET(pIndices, 2 * i);
				minVertex = std::min((uint32_t)vertex, minVertex);
				maxVertex = std::max((uint32_t)vertex, maxVertex);
			}
		}
		else
		{
			pIndices = POINTER_OFFSET(pIndices, 4 * m_IndexData.indexStart);
			ASSERT_RESULT(buider.BuildMeshlets(m_Meshlet, m_IndexData.indexCount, (uint32_t*)pIndices) == m_IndexData.indexCount);
			for (size_t i = 0; i < m_IndexData.indexCount; ++i)
			{
				uint32_t vertex = *(uint32_t*)POINTER_OFFSET(pIndices, 4 * i);
				minVertex = std::min((uint32_t)vertex, minVertex);
				maxVertex = std::max((uint32_t)vertex, maxVertex);
			}
		}

		size_t positionStride = m_pVertexData->vertexBuffers[0]->GetVertexSize();
		buider.BuildMeshletEarlyCulling(m_Meshlet, m_pVertexData->bound.GetMin(), m_pVertexData->bound.GetMax(), (const float*)pVertices, positionStride);

		if (m_IndexData.indexBuffer->GetIndexType() == IT_16)
		{
			ASSERT_RESULT(buider.ErrorCheck(m_Meshlet, minVertex, maxVertex, m_IndexData.indexCount, (uint16_t*)pIndices) == KMESHLET_STATUS_NO_ERROR);
		}
		else
		{
			ASSERT_RESULT(buider.ErrorCheck(m_Meshlet, minVertex, maxVertex, m_IndexData.indexCount, (uint32_t*)pIndices) == KMESHLET_STATUS_NO_ERROR);
		}

		buider.PadTaskMeshlets(m_Meshlet);

		if (m_Meshlet.meshletDescriptors.size() && m_Meshlet.meshletPacks.size())
		{
			KRenderGlobal::RenderDevice->CreateVertexBuffer(m_MeshData.meshletDescBuffer);
			m_MeshData.meshletDescBuffer->InitMemory(m_Meshlet.meshletDescriptors.size(), sizeof(KMeshletPackBasicDesc), m_Meshlet.meshletDescriptors.data());
			m_MeshData.meshletDescBuffer->InitDevice(false);

			KRenderGlobal::RenderDevice->CreateVertexBuffer(m_MeshData.meshletPrimBuffer);
			m_MeshData.meshletPrimBuffer->InitMemory(m_Meshlet.meshletPacks.size(), sizeof(KMeshletPackBasicType), m_Meshlet.meshletPacks.data());
			m_MeshData.meshletPrimBuffer->InitDevice(false);
		}

		m_MeshData.count = (uint32_t)m_Meshlet.meshletDescriptors.size();
		m_MeshData.offset = 0;

		return true;
	}
	return false;
}

bool KSubMesh::DestroyMeshlet()
{
	m_Meshlet.Reset();
	m_MeshData.Destroy();
	return true;
}