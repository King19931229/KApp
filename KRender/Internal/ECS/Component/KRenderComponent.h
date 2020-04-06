#pragma once
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Internal/Asset/KMesh.h"
#include "Internal/Asset/Utility/KMeshUtilityInfo.h"

class KRenderComponent : public IKRenderComponent
{
protected:
	KMeshPtr m_Mesh;
public:
	KRenderComponent();
	virtual ~KRenderComponent();

	bool GetLocalBound(KAABBBox& bound) const override;
	bool Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;
	bool CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;


	bool Init(const char* path) override;
	bool InitFromAsset(const char* path) override;

	bool InitUtility(const KMeshUtilityInfoPtr& info);
	bool UnInit();

	bool UpdateUtility(const KMeshUtilityInfoPtr& info);

	inline KMeshPtr GetMesh() { return m_Mesh; }
};