#pragma once

class KMeshUtilityInfo
{
public:
	enum UtilityType
	{
		UT_BOX,
		UT_QUAD,
		UT_CONE,
		UT_CYLINDER,
		UT_CIRLCE,
		UT_ARC,
		UT_SPHERE,
		UT_TRIANGLE,
		UT_CUBE
	};

	virtual ~KMeshUtilityInfo() {}
	virtual UtilityType GetType() = 0;
};

typedef std::shared_ptr<KMeshUtilityInfo> KMeshUtilityInfoPtr;

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
	KMeshUtilityInfoPtr CreateBox(const KMeshBoxInfo& info);
	KMeshUtilityInfoPtr CreateQuad(const KMeshQuadInfo& info);
	KMeshUtilityInfoPtr CreateCone(const KMeshConeInfo& info);
	KMeshUtilityInfoPtr CreateCylinder(const KMeshCylinderInfo& info);
	KMeshUtilityInfoPtr CreateCircle(const KMeshCircleInfo& info);
	KMeshUtilityInfoPtr CreateArc(const KMeshArcInfo& info);
	KMeshUtilityInfoPtr CreateSphere(const KMeshSphereInfo& info);
	KMeshUtilityInfoPtr CreateTriangle(const KMeshTriangleInfo& info);
	KMeshUtilityInfoPtr CreateCube(const KMeshCubeInfo& info);
};

class KMeshBoxUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshBoxInfo info;
	KMeshBoxUtilityInfo(const KMeshBoxInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_BOX; }
};

class KMeshQuadUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshQuadInfo info;
	KMeshQuadUtilityInfo(const KMeshQuadInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_QUAD; }
};

class KMeshConeUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshConeInfo info;
	KMeshConeUtilityInfo(const KMeshConeInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_CONE; }
};

class KMeshCylinderUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshCylinderInfo info;
	KMeshCylinderUtilityInfo(const KMeshCylinderInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_CYLINDER; }
};

class KMeshCircleUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshCircleInfo info;
	KMeshCircleUtilityInfo(const KMeshCircleInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_CIRLCE; }
};

class KMeshArcUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshArcInfo info;
	KMeshArcUtilityInfo(const KMeshArcInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_ARC; }
};

class KMeshSphereUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshSphereInfo info;
	KMeshSphereUtilityInfo(const KMeshSphereInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_SPHERE; }
};

class KMeshTriangleUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshTriangleInfo info;
	KMeshTriangleUtilityInfo(const KMeshTriangleInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_TRIANGLE; }
};

class KMeshCubeUtilityInfo : public KMeshUtilityInfo
{
public:
	KMeshCubeInfo info;
	KMeshCubeUtilityInfo(const KMeshCubeInfo& _info)
		: info(_info)
	{
	}

	UtilityType GetType() final { return UT_CUBE; }
};