#include "KMeshUtilityImpl.h"

namespace KMeshUtility
{
	KMeshUtilityInfoPtr CreateBox(const KMeshBoxInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshBoxUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateQuad(const KMeshQuadInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshQuadUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCone(const KMeshConeInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshConeUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCylinder(const KMeshCylinderInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshCylinderUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCircle(const KMeshCircleInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshCircleUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateArc(const KMeshArcInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshArcUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateSphere(const KMeshSphereInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshSphereUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateTriangle(const KMeshTriangleInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshTriangleUtilityInfo(info));
	}

	KMeshUtilityInfoPtr CreateCube(const KMeshCubeInfo& info)
	{
		return KMeshUtilityInfoPtr(KNEW KMeshCubeUtilityInfo(info));
	}
}