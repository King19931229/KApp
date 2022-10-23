#pragma once
#include "Interface/IKQuery.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Internal/Asset/KMesh.h"
#include "Internal/Asset/Utility/KMeshUtilityInfo.h"
#include "Internal/Asset/KMaterial.h"
#include "Internal/Asset/KMaterialSubMesh.h"

enum RenderResourceType
{
	INTERNAL_MESH,
	EXTERNAL_ASSET,
	DEBUG_UTILITY,
	UNKNOWN,
};

class KRenderComponent : public IKRenderComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKRenderComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	KMeshPtr m_Mesh;
	IKMaterialPtr m_Material;
	std::vector<KMaterialSubMeshPtr> m_MaterialSubMeshes;

	RenderResourceType m_Type;

	std::string m_ResourcePath;
	std::string m_MaterialPath;

	KMeshUtilityInfoPtr m_DebugUtility;

	bool m_HostVisible;
	bool m_UseMaterialTexture;
	bool m_OcclusionVisible;

	std::vector<IKQueryPtr> m_OCQueries;
	std::vector<IKQueryPtr> m_OCInstanceQueries;

	static constexpr const char* msType = "type";
	static constexpr const char* msPath = "path";
	static constexpr const char* msMaterialPath = "material";

	static const char* ResourceTypeToString(RenderResourceType type);
	static RenderResourceType StringToResourceType(const char* str);

	const std::string GetResourcePathString() const { return m_ResourcePath; }
	const std::string GetMaterialPathString() const { return m_MaterialPath; }
	const std::string GetTypeString() const { return std::string(ResourceTypeToString(m_Type)); }
public:
	KRenderComponent();
	virtual ~KRenderComponent();

	bool Save(IKXMLElementPtr element) override;
	bool Load(IKXMLElementPtr element) override;

	bool GetLocalBound(KAABBBox& bound) const override;
	bool Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;
	bool CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const override;

	bool SetMeshPath(const char* path) override;
	bool SetAssetPath(const char* path) override;	
	bool GetPath(std::string& path) const override;

	bool SaveAsMesh(const char* path) const override;
	bool SetHostVisible(bool hostVisible) override;
	bool SetUseMaterialTexture(bool useMaterialTex) override;
	bool GetUseMaterialTexture() const override;

	bool Init(bool async) override;
	bool UnInit() override;

	inline KMeshPtr GetMesh() { return m_Mesh; }
	IKMaterialPtr GetMaterial() override { return m_Material; }

	bool SetMaterialPath(const char* path) override;
	bool ReloadMaterial() override;

	bool InitUtility(const KMeshUtilityInfoPtr& info);
	bool UpdateUtility(const KMeshUtilityInfoPtr& info);
	bool IsUtility() const override { return m_Type == DEBUG_UTILITY; }

	bool GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as) override;

	IKQueryPtr GetOCQuery();
	IKQueryPtr GetOCInstacneQuery();
	bool SetOCInstanceQuery(IKQueryPtr query);

	inline void SetOcclusionVisible(bool visible) { m_OcclusionVisible = visible; }
	inline bool IsOcclusionVisible() const { return m_OcclusionVisible; }

	bool Visit(PipelineStage stage, std::function<void(KRenderCommand&)> func);
};