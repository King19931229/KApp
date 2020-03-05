#pragma once
#include "KComponentBase.h"
#include "Internal/Asset/KMesh.h"

class KRenderComponent : public KComponentBase
{
protected:
	KMeshPtr m_Mesh;
public:
	KRenderComponent();
	virtual ~KRenderComponent();

	bool Init(const char* path);
	bool InitFromAsset(const char* path);

	bool InitAsBox(const glm::vec3& halfExtent);
	bool InitAsQuad(float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV);
	bool InitAsCone(const glm::vec3& org, float height, float radius);
	bool InitAsCylinder(const glm::vec3& org, float height, float radius);
	bool InitAsCircle(float radius);

	bool UnInit();

	inline KMeshPtr GetMesh() { return m_Mesh; }
};