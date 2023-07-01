#pragma once
#include "KBase/Interface/Component/IKDebugComponent.h"
#include "Internal/Asset/KMesh.h"

class KDebugComponent : public IKDebugComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKDebugComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	std::vector<KMeshRef> m_Meshes;
	std::vector<KMaterialSubMeshPtr> m_MaterialSubMeshes;
	std::vector<glm::vec4> m_Colors;
public:
	KDebugComponent();
	virtual ~KDebugComponent();

	bool Save(IKXMLElementPtr element) override
	{
		return true;
	}

	bool Load(IKXMLElementPtr element) override
	{
		return true;
	}

	bool AddDebugPart(const KDebugUtilityInfo& info, const glm::vec4& color) override;
	void DestroyAllDebugParts() override;

	bool GetBound(KAABBBox& bound) const override;

	inline const std::vector<glm::vec4>& GetColors() const { return m_Colors; }
	inline const std::vector<KMaterialSubMeshPtr>& GetMaterialSubMeshs() const { return m_MaterialSubMeshes; }
};