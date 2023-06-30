#pragma once

struct KMeshBoxInfo
{
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

namespace KMeshUtility
{
	KMeshUtilityInfo CreateBox(const KMeshBoxInfo& info);
	KMeshUtilityInfo CreateQuad(const KMeshQuadInfo& info);
	KMeshUtilityInfo CreateCone(const KMeshConeInfo& info);
	KMeshUtilityInfo CreateCylinder(const KMeshCylinderInfo& info);
	KMeshUtilityInfo CreateCircle(const KMeshCircleInfo& info);
	KMeshUtilityInfo CreateArc(const KMeshArcInfo& info);
	KMeshUtilityInfo CreateSphere(const KMeshSphereInfo& info);
	KMeshUtilityInfo CreateTriangle(const KMeshTriangleInfo& info);
	KMeshUtilityInfo CreateCube(const KMeshCubeInfo& info);
};