#include "KMeshUtilityImpl.h"

namespace KMeshUtility
{
	KMeshUnilityInfoPtr CreateBox(const KMeshBoxInfo& info)
	{
		return KMeshUnilityInfoPtr(new KMeshBoxUnilityInfo(info));
	}

	KMeshUnilityInfoPtr CreateQuad(const KMeshQuadInfo& info)
	{
		return KMeshUnilityInfoPtr(new KMeshQuadUnilityInfo(info));
	}

	KMeshUnilityInfoPtr CreateCone(const KMeshConeInfo& info)
	{
		return KMeshUnilityInfoPtr(new KMeshConeUnilityInfo(info));
	}

	KMeshUnilityInfoPtr CreateCylinder(const KMeshCylinderInfo& info)
	{
		return KMeshUnilityInfoPtr(new KMeshCylinderUnilityInfo(info));
	}

	KMeshUnilityInfoPtr CreateCircle(const KMeshCircleInfo& info)
	{
		return KMeshUnilityInfoPtr(new KMeshCircleUnilityInfo(info));
	}

	KMeshUnilityInfoPtr CreateArc(const KMeshArcInfo& info)
	{
		return KMeshUnilityInfoPtr(new KMeshArcUnilityInfo(info));
	}

	KMeshUnilityInfoPtr CreateSphere(const KMeshSphereInfo& info)
	{
		return KMeshUnilityInfoPtr(new KMeshSphereUnilityInfo(info));
	}
}