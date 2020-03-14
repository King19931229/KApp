#pragma once
#include "Internal/Asset/KMesh.h"
#include "Publish/KAABBBox.h"
#include "KMeshUnilityInfo.h"

class KMeshUtilityImpl
{
protected:
	IKRenderDevice* m_Device;

	constexpr static float PI = 3.141592654f;
	constexpr static float HALF_PI = PI * 0.5f;
	constexpr static float TWO_PI = PI * 2.0f;
public:

	KMeshUtilityImpl(IKRenderDevice* device);
	~KMeshUtilityImpl();

	bool CreateBox(const KMeshBoxInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateQuad(const KMeshQuadInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateCone(const KMeshConeInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateCylinder(const KMeshCylinderInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateCircle(const KMeshCircleInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateSphere(const KMeshSphereInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateArc(const KMeshArcInfo& info, KMesh* pMesh, size_t frameInFlight);
};

namespace KMeshUtility
{
	bool CreateUtility(IKRenderDevice* device, KMesh* pMesh, const KMeshUnilityInfoPtr& info, size_t frameInFlight);
}