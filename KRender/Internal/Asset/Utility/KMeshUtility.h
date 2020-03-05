#pragma once
#include "Internal/Asset/KMesh.h"
#include "Publish/KAABBBox.h"

class KMeshUtilityImpl
{
protected:
	IKRenderDevice* m_Device;
public:
	KMeshUtilityImpl(IKRenderDevice* device);
	~KMeshUtilityImpl();

	bool CreateBox(KMesh* pMesh, const glm::vec3& halfExtend, size_t frameInFlight);
	bool CreateQuad(KMesh* pMesh, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, size_t frameInFlight);
	bool CreateCone(KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight);
	bool CreateCylinder(KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight);
	bool CreateCircle(KMesh* pMesh, float radius, size_t frameInFlight);
};

namespace KMeshUtility
{
	bool CreateBox(IKRenderDevice* device, KMesh* pMesh, const glm::vec3& halfExtent, size_t frameInFlight);
	bool CreateQuad(IKRenderDevice* device, KMesh* pMesh, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, size_t frameInFlight);
	bool CreateCone(IKRenderDevice* device, KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight);
	bool CreateCylinder(IKRenderDevice* device, KMesh* pMesh, const glm::vec3& org, float height, float radius, size_t frameInFlight);
	bool CreateCircle(IKRenderDevice* device, KMesh* pMesh, float radius, size_t frameInFlight);
}