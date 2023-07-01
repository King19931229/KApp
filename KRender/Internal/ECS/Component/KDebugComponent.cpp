#include "KDebugComponent.h"
#include "Internal/KRenderGlobal.h"

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME KDebugComponent
#define KRTTR_REG_CLASS_NAME_STR "DebugComponent"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}

KDebugComponent::KDebugComponent()
{}

KDebugComponent::~KDebugComponent()
{
	DestroyAllDebugParts();
}

bool KDebugComponent::AddDebugPart(const KDebugUtilityInfo& info, const glm::vec4& color)
{
	KMeshRef newMesh;
	KRenderGlobal::MeshManager.New(newMesh);

	if (newMesh->InitFromUtility(info))
	{
		const std::vector<KSubMeshPtr>& subMeshes = newMesh->GetSubMeshes();
		assert(subMeshes.size() == 1);
		m_MaterialSubMeshes.reserve(m_MaterialSubMeshes.size() + subMeshes.size());
		for (size_t i = 0; i < subMeshes.size(); ++i)
		{
			KMaterialSubMeshPtr materialSubMesh = KMaterialSubMeshPtr(KNEW KMaterialSubMesh());
			materialSubMesh->InitDebug(subMeshes[i], subMeshes[i]->GetDebugPrimitive());
			m_MaterialSubMeshes.push_back(materialSubMesh);
		}

		m_Meshes.push_back(newMesh);
		m_Colors.push_back(color);

		return true;
	}
	else
	{
		return false;
	}
}

void KDebugComponent::DestroyAllDebugParts()
{
	for (KMaterialSubMeshPtr& materialSubMesh : m_MaterialSubMeshes)
	{
		materialSubMesh->UnInit();
	}
	m_MaterialSubMeshes.clear();
	m_Meshes.clear();
	m_Colors.clear();
}

bool KDebugComponent::GetBound(KAABBBox& bound) const
{
	bound.SetNull();
	for (KMeshRef mesh : m_Meshes)
	{
		bound = bound.Merge(mesh->GetLocalBound());
	}
	return true;
}