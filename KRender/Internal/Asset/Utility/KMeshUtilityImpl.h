#pragma once
#include "Internal/Asset/KMesh.h"
#include "KBase/Publish/KAABBBox.h"
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

	bool CreateMesh(const std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, const std::vector<uint16_t>& indices, DebugPrimitive primtive, KMesh* pMesh);
	bool UpdateMesh(const std::vector<KVertexDefinition::DEBUG_POS_3F>& vertices, KMesh* pMesh);
public:
	KMeshUtilityImpl(IKRenderDevice* device);
	~KMeshUtilityImpl();

	template<typename T>
	bool CreateByInfo(const T& info, KMesh* pMesh)
	{
		if (pMesh)
		{
			std::vector<KVertexDefinition::DEBUG_POS_3F> vertices;
			std::vector<uint16_t> indices;
			DebugPrimitive primtive;

			PouplateData(info, vertices, indices, primtive);
			return CreateMesh(vertices, indices, primtive, pMesh);
		}
		return false;
	}

	template<typename T>
	bool UpdateByInfo(const T& info, KMesh* pMesh)
	{
		if (pMesh)
		{
			std::vector<KVertexDefinition::DEBUG_POS_3F> vertices;
			std::vector<uint16_t> indices;
			DebugPrimitive primtive;

			PouplateData(info, vertices, indices, primtive);
			return UpdateMesh(vertices, pMesh);
		}
		return false;
	}

	bool CreateBox(const KMeshBoxInfo& info, KMesh* pMesh);
	bool CreateQuad(const KMeshQuadInfo& info, KMesh* pMesh);
	bool CreateCone(const KMeshConeInfo& info, KMesh* pMesh);
	bool CreateCylinder(const KMeshCylinderInfo& info, KMesh* pMesh);
	bool CreateCircle(const KMeshCircleInfo& info, KMesh* pMesh);
	bool CreateSphere(const KMeshSphereInfo& info, KMesh* pMesh);
	bool CreateArc(const KMeshArcInfo& info, KMesh* pMesh);
	bool CreateTriangle(const KMeshTriangleInfo& info, KMesh* pMesh);
	bool CreateCube(const KMeshCubeInfo& info, KMesh* pMesh);

	bool UpdateBox(const KMeshBoxInfo& info, KMesh* pMesh);
	bool UpdateQuad(const KMeshQuadInfo& info, KMesh* pMesh);
	bool UpdateCone(const KMeshConeInfo& info, KMesh* pMesh);
	bool UpdateCylinder(const KMeshCylinderInfo& info, KMesh* pMesh);
	bool UpdateCircle(const KMeshCircleInfo& info, KMesh* pMesh);
	bool UpdateSphere(const KMeshSphereInfo& info, KMesh* pMesh);
	bool UpdateArc(const KMeshArcInfo& info, KMesh* pMesh);
	bool UpdateTriangle(const KMeshTriangleInfo& info, KMesh* pMesh);
	bool UpdateCube(const KMeshCubeInfo& info, KMesh* pMesh);
};

namespace KMeshUtility
{
	bool CreateUtility(KMesh* pMesh, const KMeshUtilityInfoPtr& info);
	bool UpdateUtility(KMesh* pMesh, const KMeshUtilityInfoPtr& info);
}