#pragma once
#include "Interface/IKQuery.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Internal/Asset/KMesh.h"
#include "Internal/Asset/Utility/KMeshUtilityInfo.h"
#include "Internal/Asset/KMaterial.h"
#include "Internal/Asset/KMaterialSubMesh.h"

class KRenderComponent : public IKRenderComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKRenderComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	KMeshRef m_Mesh;
	std::vector<KMaterialSubMeshPtr> m_MaterialSubMeshes;
	std::vector<IKQueryPtr> m_OCQueries;
	std::vector<IKQueryPtr> m_OCInstanceQueries;

	KMeshUtilityInfoPtr m_DebugUtility;

	bool m_HostVisible;
	bool m_UseMaterialTexture;
	bool m_OcclusionVisible;

	static constexpr const char* msType = "type";
	static constexpr const char* msPath = "path"; 
	static constexpr const char* msMaterial = "material";

	static const char* MeshResourceTypeToString(MeshResourceType type);
	static MeshResourceType StringToMeshResourceType(const char* str);

	std::string GetResourcePathString() const;
	std::string GetTypeString() const;

	void MeshPostInit();
public:
	KRenderComponent();
	virtual ~KRenderComponent();

	bool Save(IKXMLElementPtr element) override;
	bool Load(IKXMLElementPtr element) override;

	bool GetLocalBound(KAABBBox& bound) const override;
	bool Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;
	bool CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;

	bool GetPath(std::string& path) const override;
	bool GetHostVisible(bool& hostVisible) const override;

	bool SaveAsMesh(const std::string& path) const override;

	bool InitAsMesh(const std::string& mesh, bool hostVisible, bool async) override;
	bool InitAsAsset(const std::string& asset, bool hostVisible, bool async) override;
	bool UnInit() override;

	inline KMeshPtr GetMesh() { return *m_Mesh; }
	inline const std::vector<KMaterialSubMeshPtr>& GetMaterialSubMeshs() const { return m_MaterialSubMeshes; }

	bool InitUtility(const KMeshUtilityInfoPtr& info);
	bool UpdateUtility(const KMeshUtilityInfoPtr& info);
	bool IsUtility() const override { return m_Mesh && m_Mesh->GetType() == MRT_DEBUG_UTILITY; }

	bool GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as) override;

	IKQueryPtr GetOCQuery();
	IKQueryPtr GetOCInstacneQuery();
	bool SetOCInstanceQuery(IKQueryPtr query);

	inline void SetOcclusionVisible(bool visible) { m_OcclusionVisible = visible; }
	inline bool IsOcclusionVisible() const { return m_OcclusionVisible; }
};