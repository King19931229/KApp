#include "KMeshUtility.h"
#include "Internal/Asset/KSubMesh.h"
#include "Internal/KVertexDefinition.h"
#include <assert.h>

namespace KMeshUtility
{
	bool CreateBox(IKRenderDevice* device, KMesh* pMesh, const glm::vec3& halfExtent, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateBox(pMesh, halfExtent, frameInFlight);
	}

	bool CreateQuad(IKRenderDevice* device, KMesh* pMesh, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateQuad(pMesh, lengthU, lengthV, axisU, axisV, frameInFlight);
	}

	bool CreateCone(IKRenderDevice* device, KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateCone(pMesh, org, height, radius, frameInFlight);
	}

	bool CreateCylinder(IKRenderDevice* device, KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateCylinder(pMesh, org, height, radius, frameInFlight);
	}

	bool CreateCircle(IKRenderDevice* device, KMesh* pMesh, float radius, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateCircle(pMesh, radius, frameInFlight);
	}
}

KMeshUtilityImpl::KMeshUtilityImpl(IKRenderDevice* device)
	: m_Device(device)
{
	assert(device);
}

KMeshUtilityImpl::~KMeshUtilityImpl()
{
}

bool KMeshUtilityImpl::CreateBox(KMesh* pMesh, const glm::vec3& halfExtend, size_t frameInFlight)
{
	if (pMesh)
	{
		pMesh->UnInit();

		KVertexData& vertexData = pMesh->m_VertexData;

		IKVertexBufferPtr vertexBuffer = nullptr;
		m_Device->CreateVertexBuffer(vertexBuffer);

		KAABBBox bound;
		bound.InitFromHalfExtent(glm::vec3(0.0f), halfExtend);

		KVertexDefinition::DEBUG_POS_3F positions[] =
		{
			{ halfExtend * glm::vec3(-1.0f, -1.0, -1.0f) },
			{ halfExtend * glm::vec3(-1.0, 1.0f, -1.0f) },
			{ halfExtend * glm::vec3(1.0f, 1.0f, -1.0f) },
			{ halfExtend * glm::vec3(1.0f, -1.0f, -1.0f) },

			{ halfExtend * glm::vec3(1.0f, 1.0f, 1.0f) },
			{ halfExtend * glm::vec3(-1.0, 1.0f, 1.0f) },
			{ halfExtend * glm::vec3(-1.0, -1.0f, 1.0f) },
			{ halfExtend * glm::vec3(1.0f, -1.0f, 1.0f) }
		};

		vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
		vertexData.vertexBuffers.push_back(vertexBuffer);
		vertexData.vertexStart = 0;
		vertexData.vertexCount = ARRAY_SIZE(positions);
		vertexData.bound = bound;

		auto halfExtend = bound.GetExtend() * 0.5f;

		vertexBuffer->InitMemory(ARRAY_SIZE(positions), sizeof(positions[0]), positions);
		vertexBuffer->InitDevice(false);

		uint16_t indices[] =
		{
			0, 1,
			2, 3,
			1, 2,
			3, 0,
			1, 5,
			4, 2,
			0, 6,
			7, 3,
			5, 6,
			7, 4
		};

		IKIndexBufferPtr indexBuffer = nullptr;
		m_Device->CreateIndexBuffer(indexBuffer);

		indexBuffer->InitMemory(IT_16, ARRAY_SIZE(indices), indices);
		indexBuffer->InitDevice(false);

		KIndexData indexData;
		indexData.indexBuffer = indexBuffer;
		indexData.indexStart = 0;
		indexData.indexCount = ARRAY_SIZE(indices);

		auto& subMeshes = pMesh->m_SubMeshes;

		KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
		newSubMesh->InitDebug(DEBUG_PRIMITIVE_LINE, &vertexData, &indexData, frameInFlight);

		subMeshes.push_back(newSubMesh);

		return true;
	}
	return false;
}

bool KMeshUtilityImpl::CreateQuad(KMesh* pMesh, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, size_t frameInFlight)
{
	if (pMesh)
	{
		pMesh->UnInit();

		KVertexData& vertexData = pMesh->m_VertexData;

		IKVertexBufferPtr vertexBuffer = nullptr;
		m_Device->CreateVertexBuffer(vertexBuffer);

		KVertexDefinition::DEBUG_POS_3F positions[] =
		{
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(axisU * lengthU),
			glm::vec3(axisV * lengthV),
			glm::vec3(axisU * lengthU + axisV * lengthV),
		};

		KAABBBox bound;
		for (const auto& pos : positions)
		{
			bound.Merge(pos.DEBUG_POSITION, bound);
		}

		vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
		vertexData.vertexBuffers.push_back(vertexBuffer);
		vertexData.vertexStart = 0;
		vertexData.vertexCount = ARRAY_SIZE(positions);
		vertexData.bound = bound;

		vertexBuffer->InitMemory(ARRAY_SIZE(positions), sizeof(positions[0]), positions);
		vertexBuffer->InitDevice(false);

		uint16_t indices[] =
		{
			0, 1, 2,
			3, 2, 1,
		};

		IKIndexBufferPtr indexBuffer = nullptr;
		m_Device->CreateIndexBuffer(indexBuffer);

		indexBuffer->InitMemory(IT_16, ARRAY_SIZE(indices), indices);
		indexBuffer->InitDevice(false);

		KIndexData indexData;
		indexData.indexBuffer = indexBuffer;
		indexData.indexStart = 0;
		indexData.indexCount = ARRAY_SIZE(indices);

		auto& subMeshes = pMesh->m_SubMeshes;

		KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
		newSubMesh->InitDebug(DEBUG_PRIMITIVE_TRIANGLE, &vertexData, &indexData, frameInFlight);

		subMeshes.push_back(newSubMesh);

		return true;
	}
	return false;
}

bool KMeshUtilityImpl::CreateCone(KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight)
{
	return false;
}

bool KMeshUtilityImpl::CreateCylinder(KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight)
{
	return false;
}

bool KMeshUtilityImpl::CreateCircle(KMesh* pMesh, float radius, size_t frameInFlight)
{
	return false;
}