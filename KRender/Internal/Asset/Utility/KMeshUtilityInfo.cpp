#include "KMeshUtilityImpl.h"
#include "KMeshUtilityInfo.h"

namespace KMeshUtility
{
	KMeshUtilityInfo CreateBox(const KMeshBoxInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateQuad(const KMeshQuadInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateCone(const KMeshConeInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateCylinder(const KMeshCylinderInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateCircle(const KMeshCircleInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateArc(const KMeshArcInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateSphere(const KMeshSphereInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateTriangle(const KMeshTriangleInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}

	KMeshUtilityInfo CreateCube(const KMeshCubeInfo& info)
	{
		KMeshUtilityInfo ret;
		KMeshUtilityImpl::PouplateData(info, ret.positions, ret.indices, ret.primtive);
		return ret;
	}
}