#pragma once
#include "Internal/Asset/KMesh.h"
#include "KBase/Publish/KAABBBox.h"
#include "KMeshUtilityInfo.h"

struct KMeshUtilityImpl
{
	static void PouplateData(const KMeshBoxInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshQuadInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshConeInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshCylinderInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshCircleInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshSphereInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshArcInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshTriangleInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
	static void PouplateData(const KMeshCubeInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive);
};