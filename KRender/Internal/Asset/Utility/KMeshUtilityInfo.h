#pragma once

class KMeshUnilityInfo
{
public:
	enum UnilityType
	{
		UT_BOX,
		UT_QUAD,
		UT_CONE,
		UT_CYLINDER,
		UT_CIRLCE,
		UT_ARC,
		UT_SPHERE
	};

	virtual ~KMeshUnilityInfo() {}
	virtual UnilityType GetType() = 0;
};

typedef std::shared_ptr<KMeshUnilityInfo> KMeshUnilityInfoPtr;

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

namespace KMeshUtility
{
	KMeshUnilityInfoPtr CreateBox(const KMeshBoxInfo& info);
	KMeshUnilityInfoPtr CreateQuad(const KMeshQuadInfo& info);
	KMeshUnilityInfoPtr CreateCone(const KMeshConeInfo& info);
	KMeshUnilityInfoPtr CreateCylinder(const KMeshCylinderInfo& info);
	KMeshUnilityInfoPtr CreateCircle(const KMeshCircleInfo& info);
	KMeshUnilityInfoPtr CreateArc(const KMeshArcInfo& info);
	KMeshUnilityInfoPtr CreateSphere(const KMeshSphereInfo& info);
};

class KMeshBoxUnilityInfo : public KMeshUnilityInfo
{
public:
	KMeshBoxInfo info;
	KMeshBoxUnilityInfo(const KMeshBoxInfo& _info)
		: info(_info)
	{
	}

	UnilityType GetType() final { return UT_BOX; }
};

class KMeshQuadUnilityInfo : public KMeshUnilityInfo
{
public:
	KMeshQuadInfo info;
	KMeshQuadUnilityInfo(const KMeshQuadInfo& _info)
		: info(_info)
	{
	}

	UnilityType GetType() final { return UT_QUAD; }
};

class KMeshConeUnilityInfo : public KMeshUnilityInfo
{
public:
	KMeshConeInfo info;
	KMeshConeUnilityInfo(const KMeshConeInfo& _info)
		: info(_info)
	{
	}

	UnilityType GetType() final { return UT_CONE; }
};

class KMeshCylinderUnilityInfo : public KMeshUnilityInfo
{
public:
	KMeshCylinderInfo info;
	KMeshCylinderUnilityInfo(const KMeshCylinderInfo& _info)
		: info(_info)
	{
	}

	UnilityType GetType() final { return UT_CYLINDER; }
};

class KMeshCircleUnilityInfo : public KMeshUnilityInfo
{
public:
	KMeshCircleInfo info;
	KMeshCircleUnilityInfo(const KMeshCircleInfo& _info)
		: info(_info)
	{
	}

	UnilityType GetType() final { return UT_CIRLCE; }
};

class KMeshArcUnilityInfo : public KMeshUnilityInfo
{
public:
	KMeshArcInfo info;
	KMeshArcUnilityInfo(const KMeshArcInfo& _info)
		: info(_info)
	{
	}

	UnilityType GetType() final { return UT_ARC; }
};

class KMeshSphereUnilityInfo : public KMeshUnilityInfo
{
public:
	KMeshSphereInfo info;
	KMeshSphereUnilityInfo(const KMeshSphereInfo& _info)
		: info(_info)
	{
	}

	UnilityType GetType() final { return UT_SPHERE; }
};