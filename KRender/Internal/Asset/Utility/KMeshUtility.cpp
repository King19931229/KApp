#include "KMeshUtility.h"
#include "Internal/Asset/KSubMesh.h"
#include "Internal/KVertexDefinition.h"
#include <assert.h>

namespace KMeshUtility
{
	bool CreateBox(IKRenderDevice* device, KMesh* pMesh, const KAABBBox& bound, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateBox(pMesh, bound, frameInFlight);
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

bool KMeshUtilityImpl::CreateBox(KMesh* pMesh, const KAABBBox& bound, size_t frameInFlight)
{
	assert(bound.IsDefault());
	if (pMesh)
	{
		pMesh->UnInit();

		KVertexData& vertexData = pMesh->m_VertexData;

		IKVertexBufferPtr vertexBuffer = nullptr;
		m_Device->CreateVertexBuffer(vertexBuffer);

		vertexData.vertexFormats.push_back(VF_POINT_NORMAL_UV);
		vertexData.vertexBuffers.push_back(vertexBuffer);
		vertexData.vertexStart = 0;
		vertexData.vertexCount = 8;
		vertexData.bound = bound;

		auto center = bound.GetCenter();
		auto halfExtend = bound.GetExtend() * 0.5f;

		KVertexDefinition::POS_3F_NORM_3F_UV_2F positions[] =
		{
			{ /*center + */halfExtend * glm::vec3(-1.0f, -1.0, -1.0f), glm::vec3(-1.0, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f) },
			{ /*center + */halfExtend * glm::vec3(-1.0, 1.0f, -1.0f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f) },
			{ /*center + */halfExtend * glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 0.0f) },
			{ /*center + */halfExtend * glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 1.0f) },

			{ /*center + */halfExtend * glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f) },
			{ /*center + */halfExtend * glm::vec3(-1.0, 1.0f, 1.0f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f) },
			{ /*center + */halfExtend * glm::vec3(-1.0, -1.0f, 1.0f), glm::vec3(-1.0, -1.0f, 1.0f), glm::vec2(1.0f, 0.0f) },
			{ /*center + */halfExtend * glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, -1.0f, 1.0f), glm::vec2(1.0f, 1.0f) }
		};

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
		newSubMesh->InitDebugOnly(&vertexData, std::move(indexData), frameInFlight);

		subMeshes.push_back(newSubMesh);

		return true;
	}
	return false;
}