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
	enum ResourceType
	{
		MESH,
		ASSET,
		UTILITY,
		NONE
	};

	KMeshPtr m_Mesh;
	IKMaterialPtr m_Material;
	std::vector<KMaterialSubMeshPtr> m_MaterialSubMeshes;

	ResourceType m_Type;

	std::string m_Path;
	KMeshUtilityInfoPtr m_UtilityInfo;

	std::string m_MaterialPath;

	bool m_HostVisible;
	bool m_UseMaterialTexture;
	bool m_OcclusionVisible;

	std::vector<IKQueryPtr> m_OCQueries;
	std::vector<IKQueryPtr> m_OCInstanceQueries;

	static constexpr const char* msType = "type";
	static constexpr const char* msPath = "path";
	static constexpr const char* msMaterialPath = "material";

	static const char* ResourceTypeToString(ResourceType type);
	static ResourceType StringToResourceType(const char* str);

	const std::string GetPathString() const { return m_Path; }
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

	bool SetMaterialPath(const char* path) override;
	bool ReloadMaterial() override;

	bool InitUtility(const KMeshUtilityInfoPtr& info);
	bool UpdateUtility(const KMeshUtilityInfoPtr& info);

	inline KMeshPtr GetMesh() { return m_Mesh; }
	IKMaterialPtr GetMaterial() override { return m_Material; }

	inline IKQueryPtr GetOCQuery(size_t frameIndex) { return frameIndex < m_OCQueries.size() ? m_OCQueries[frameIndex] : nullptr; }
	inline IKQueryPtr GetOCInstacneQuery(size_t frameIndex) { return frameIndex < m_OCInstanceQueries.size() ? m_OCInstanceQueries[frameIndex] : nullptr; }
	inline bool SetOCInstanceQuery(size_t frameIndex, IKQueryPtr query)
	{
		if (frameIndex < m_OCInstanceQueries.size())
		{
			m_OCInstanceQueries[frameIndex] = query;
			return true;
		}
		return false;
	}

	inline void SetOcclusionVisible(bool visible) { m_OcclusionVisible = visible; }
	inline bool IsOcclusionVisible() const { return m_OcclusionVisible; }

	bool Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand&)> func);
};