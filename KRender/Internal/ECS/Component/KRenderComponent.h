#pragma once
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Internal/Asset/KMesh.h"
#include "Internal/Asset/Utility/KMeshUtilityInfo.h"

class KRenderComponent : public IKRenderComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKRenderComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	KMeshPtr m_Mesh;
	enum ResourceType
	{
		MESH,
		ASSET,
		UTILITY,
		NONE
	}m_Type;
	std::string m_Path;
	KMeshUtilityInfoPtr m_UtilityInfo;

	static constexpr const char* msType = "type";
	static constexpr const char* msPath = "path";

	static const char* ResourceTypeToString(ResourceType type);
	static ResourceType StringToResourceType(const char* str);

	const std::string GetPathString() const { return m_Path; }
	const std::string GetTypeString() const { return std::string(ResourceTypeToString(m_Type)); }
public:
	KRenderComponent();
	virtual ~KRenderComponent();

	bool Save(IKXMLElementPtr element) override;
	bool Load(IKXMLElementPtr element) override;

	bool GetLocalBound(KAABBBox& bound) const override;
	bool Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;
	bool CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;

	bool SetPathMesh(const char* path) override;
	bool SetPathAsset(const char* path) override;
	bool GetPath(std::string& path) const override;

	bool Init() override;
	bool UnInit() override;

	bool InitUtility(const KMeshUtilityInfoPtr& info);
	bool UpdateUtility(const KMeshUtilityInfoPtr& info);

	inline KMeshPtr GetMesh() { return m_Mesh; }
};