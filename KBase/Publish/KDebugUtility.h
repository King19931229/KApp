#pragma once
#include "glm/glm.hpp"
#include <vector>

enum DebugPrimitive
{
	DEBUG_PRIMITIVE_LINE,
	DEBUG_PRIMITIVE_TRIANGLE
};

struct KDebugUtilityInfo
{
	std::vector<glm::vec3> positions;
	std::vector<uint16_t> indices;
	DebugPrimitive primtive;
};

struct KMeshBoxInfo
{
	glm::mat4 transform;
	glm::vec3 halfExtend;
};

struct KMeshQuadInfo
{
	glm::mat4 transform;
	float lengthU;
	float lengthV;
	glm::vec3 axisU;
	glm::vec3 axisV;
};

struct KMeshConeInfo
{
	glm::mat4 transform;
	float height;
	float radius;
};

struct KMeshCylinderInfo
{
	glm::mat4 transform;
	float height;
	float radius;
};

struct KMeshCircleInfo
{
	glm::mat4 transform;
	float radius;
};

struct KMeshArcInfo
{
	glm::vec3 axis;
	glm::vec3 normal;
	float radius;
	float theta;
};

struct KMeshSphereInfo
{
	glm::mat4 transform;
	float radius;
};

struct KMeshTriangleInfo
{
	glm::vec3 origin;
	float lengthU;
	float lengthV;
	glm::vec3 axisU;
	glm::vec3 axisV;
};

struct KMeshCubeInfo
{
	glm::mat4 transform;
	glm::vec3 halfExtend;
};

namespace KDebugUtility
{
	KDebugUtilityInfo CreateBox(const KMeshBoxInfo& info);
	KDebugUtilityInfo CreateQuad(const KMeshQuadInfo& info);
	KDebugUtilityInfo CreateCone(const KMeshConeInfo& info);
	KDebugUtilityInfo CreateCylinder(const KMeshCylinderInfo& info);
	KDebugUtilityInfo CreateCircle(const KMeshCircleInfo& info);
	KDebugUtilityInfo CreateArc(const KMeshArcInfo& info);
	KDebugUtilityInfo CreateSphere(const KMeshSphereInfo& info);
	KDebugUtilityInfo CreateTriangle(const KMeshTriangleInfo& info);
	KDebugUtilityInfo CreateCube(const KMeshCubeInfo& info);
};