#include "KMeshUtilityImpl.h"

namespace KMeshUtility
{
	KMeshUtilityInfoPtr CreateBox(const KMeshBoxInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshBoxUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateQuad(const KMeshQuadInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshQuadUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCone(const KMeshConeInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshConeUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCylinder(const KMeshCylinderInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshCylinderUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCircle(const KMeshCircleInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshCircleUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateArc(const KMeshArcInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshArcUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateSphere(const KMeshSphereInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshSphereUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateTriangle(const KMeshTriangleInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshTriangleUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCube(const KMeshCubeInfo& info)
	{
		return KMeshUtilityInfoPtr(new KMeshCubeUtilityInfo(info));
	}
}