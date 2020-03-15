#pragma once
#include "Internal/Asset/KMesh.h"
#include "Publish/KAABBBox.h"
#include "KMeshUtilityInfo.h"

class KMeshUtilityImpl
{
protected:
	IKRenderDevice* m_Device;

	constexpr static float PI = 3.141592654f;
	constexpr static float HALF_PI = PI * 0.5f;
	constexpr static float TWO_PI = PI * 2.0f;

	void PouplateData(const KMeshBoxInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshQuadInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshConeInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshCylinderInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshCircleInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshSphereInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshArcInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshTriangleInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	void PouplateData(const KMeshCubeInfo& info, std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);

	bool CreateMesh(const std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, const std::vector<uint16_t>& indices, DebugPrimitive primtive, KMesh* pMesh, size_t frameInFlight);
	bool UpdateMesh(const std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, KMesh* pMesh, size_t frameInFlight);
public:
	KMeshUtilityImpl(IKRenderDevice* device);
	~KMeshUtilityImpl();

	template<typename T>
	bool CreateByInfo(const T& info, KMesh* pMesh, size_t frameInFlight)
	{
		if (pMesh)
		{
			std::vector<KVertexDefinition::DEBUG_POS_3F> vertices;
			std::vector<uint16_t> indices;
			DebugPrimitive primtive;

			PouplateData(info, vertices, indices, primtive);
			return CreateMesh(vertices, indices, primtive, pMesh, frameInFlight);
		}
		return false;
	}

	template<typename T>
	bool UpdateByInfo(const T& info, KMesh* pMesh, size_t frameInFlight)
	{
		if (pMesh)
		{
			std::vector<KVertexDefinition::DEBUG_POS_3F> vertices;
			std::vector<uint16_t> indices;
			DebugPrimitive primtive;

			PouplateData(info, vertices, indices, primtive);
			return UpdateMesh(vertices, pMesh, frameInFlight);
		}
		return false;
	}

	bool CreateBox(const KMeshBoxInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateQuad(const KMeshQuadInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateCone(const KMeshConeInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateCylinder(const KMeshCylinderInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateCircle(const KMeshCircleInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateSphere(const KMeshSphereInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateArc(const KMeshArcInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateTriangle(const KMeshTriangleInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool CreateCube(const KMeshCubeInfo& info, KMesh* pMesh, size_t frameInFlight);

	bool UpdateBox(const KMeshBoxInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateQuad(const KMeshQuadInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateCone(const KMeshConeInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateCylinder(const KMeshCylinderInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateCircle(const KMeshCircleInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateSphere(const KMeshSphereInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateArc(const KMeshArcInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateTriangle(const KMeshTriangleInfo& info, KMesh* pMesh, size_t frameInFlight);
	bool UpdateCube(const KMeshCubeInfo& info, KMesh* pMesh, size_t frameInFlight);
};

namespace KMeshUtility
{
	bool CreateUtility(IKRenderDevice* device, KMesh* pMesh, const KMeshUtilityInfoPtr& info, size_t frameInFlight);
	bool UpdateUtility(IKRenderDevice* device, KMesh* pMesh, const KMeshUtilityInfoPtr& info, size_t frameInFlight);
}