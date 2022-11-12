#include "KRenderComponent.h"
#include "Internal/KRenderGlobal.h"

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME KRenderComponent
#define KRTTR_REG_CLASS_NAME_STR "RenderComponent"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_PROPERTY_READ_ONLY(path, GetResourcePathString, MDT_STDSTRING)
	KRTTR_REG_PROPERTY_READ_ONLY(material, GetMaterialPathString, MDT_STDSTRING)
	KRTTR_REG_PROPERTY_READ_ONLY(type, GetTypeString, MDT_STDSTRING)
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}

KRenderComponent::KRenderComponent()
	: m_Type(UNKNOWN),
	m_DebugUtility(nullptr),
	m_HostVisible(true),
	m_UseMaterialTexture(false),
	m_OcclusionVisible(true)
{}

KRenderComponent::~KRenderComponent()
{
	UnInit();
}

const char* KRenderComponent::ResourceTypeToString(RenderResourceType type)
{
#define ENUM(type) case type: return #type;
	switch (type)
	{
		ENUM(INTERNAL_MESH);
		ENUM(EXTERNAL_ASSET);
		ENUM(DEBUG_UTILITY);
		ENUM(UNKNOWN);
	default:
		assert(false);
		return "UNKNOWN";
	}
#undef ENUM
}

RenderResourceType KRenderComponent::StringToResourceType(const char* str)
{
#define CMP(enum_string) if (!strcmp(str, #enum_string)) return enum_string;
	CMP(INTERNAL_MESH);
	CMP(EXTERNAL_ASSET);
	CMP(DEBUG_UTILITY);
	CMP(UNKNOWN);
	return UNKNOWN;
#undef CMP
}

bool KRenderComponent::Save(IKXMLElementPtr element)
{
	if (m_Type == INTERNAL_MESH || m_Type == EXTERNAL_ASSET)
	{
		IKXMLElementPtr typeEle = element->NewElement(msType);
		typeEle->SetText(ResourceTypeToString(m_Type));

		IKXMLElementPtr pathEle = element->NewElement(msPath);
		pathEle->SetText(m_ResourcePath.c_str());

		IKXMLElementPtr materialEle = element->NewElement(msMaterialPath);
		materialEle->SetText(m_MaterialPath.c_str());

		return true;
	}
	return false;
}

bool KRenderComponent::Load(IKXMLElementPtr element)
{
	IKXMLElementPtr typeEle = element->FirstChildElement(msType);	
	ACTION_ON_FAILURE(typeEle != nullptr && !typeEle->IsEmpty(), return false);

	RenderResourceType type = StringToResourceType(typeEle->GetText().c_str());
	ACTION_ON_FAILURE(type == INTERNAL_MESH || type == EXTERNAL_ASSET, return false);

	IKXMLElementPtr pathEle = element->FirstChildElement(msPath);
	ACTION_ON_FAILURE(pathEle != nullptr && !pathEle->IsEmpty(), return false);

	IKXMLElementPtr materialEle = element->FirstChildElement(msMaterialPath);
	ACTION_ON_FAILURE(materialEle != nullptr && !materialEle->IsEmpty(), return false);

	std::string path = pathEle->GetText();
	ACTION_ON_FAILURE(!path.empty(), return false);

	std::string materialPath = materialEle->GetText();

	UnInit();

	m_Type = type;
	m_ResourcePath = path;
	m_MaterialPath = materialPath;

	return Init(true);
}

bool KRenderComponent::GetLocalBound(KAABBBox& bound) const
{
	if (m_Mesh)
	{
		bound = (*m_Mesh)->GetLocalBound();
		return true;
	}
	return false;
}

bool KRenderComponent::Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const
{
	if (m_Mesh)
	{
		const KTriangleMesh& triangleMesh = (*m_Mesh)->GetTriangleMesh();
		if (triangleMesh.CloestPickPoint(localOrigin, localDir, result))
		{
			return true;
		}
	}
	return false;
}

bool KRenderComponent::CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const
{
	if (m_Mesh)
	{
		const KTriangleMesh& triangleMesh = (*m_Mesh)->GetTriangleMesh();
		if (triangleMesh.Pick(localOrigin, localDir, result))
		{
			return true;
		}
	}
	return false;
}

bool KRenderComponent::SetMeshPath(const char* path)
{
	if (path)
	{
		m_Type = INTERNAL_MESH;
		m_ResourcePath = path;
		return true;
	}
	return false;
}

bool KRenderComponent::SetAssetPath(const char* path)
{
	if (path)
	{
		m_Type = EXTERNAL_ASSET;
		m_ResourcePath = path;
		return true;
	}
	return false;
}

bool KRenderComponent::SetMaterialPath(const char* path)
{
	if (path)
	{
		m_MaterialPath = path;
		return true;
	}
	return false;
}

bool KRenderComponent::ReloadMaterial()
{
	if (m_Mesh)
	{
		if (m_Material)
		{
			KRenderGlobal::MaterialManager.Release(m_Material);
			m_Material = nullptr;
		}

		if (m_MaterialPath.empty() || !KRenderGlobal::MaterialManager.Acquire(m_MaterialPath.c_str(), m_Material, true))
		{
			KRenderGlobal::MaterialManager.GetMissingMaterial(m_Material);
		}
		
		for (KMaterialSubMeshPtr& materialSubMesh : m_MaterialSubMeshes)
		{
			materialSubMesh->UnInit();
			materialSubMesh->Init(m_Material.get());
		}

		return true;
	}
	return false;
}

bool KRenderComponent::GetPath(std::string& path) const
{
	if (m_Type == EXTERNAL_ASSET || m_Type == INTERNAL_MESH)
	{
		path = m_ResourcePath;
		return true;
	}
	return false;
}

bool KRenderComponent::SaveAsMesh(const char* path) const
{
	if (m_Type == EXTERNAL_ASSET || m_Type == INTERNAL_MESH)
	{
		return (*m_Mesh)->SaveAsFile(path);
	}
	return false;
}

bool KRenderComponent::SetHostVisible(bool hostVisible)
{
	m_HostVisible = hostVisible;
	return true;
}

bool KRenderComponent::SetUseMaterialTexture(bool useMaterialTex)
{
	m_UseMaterialTexture = useMaterialTex;
	return true;
}

bool KRenderComponent::GetUseMaterialTexture() const
{
	return m_UseMaterialTexture;
}

bool KRenderComponent::Init(bool async)
{
	UnInit();
	if (m_Type == INTERNAL_MESH || m_Type == EXTERNAL_ASSET)
	{
		bool meshAcquire = false;
		if (m_Type == INTERNAL_MESH)
		{
			meshAcquire = KRenderGlobal::MeshManager.Acquire(m_ResourcePath.c_str(), m_Mesh, m_HostVisible);
		}
		else
		{
			meshAcquire = KRenderGlobal::MeshManager.AcquireFromAsset(m_ResourcePath.c_str(), m_Mesh, m_HostVisible);
		}
		if (meshAcquire)
		{
			if (m_MaterialPath.empty() || !KRenderGlobal::MaterialManager.Acquire(m_MaterialPath.c_str(), m_Material, async))
			{
				KRenderGlobal::MaterialManager.GetMissingMaterial(m_Material);
			}

			const std::vector<KSubMeshPtr>& subMeshes = (*m_Mesh)->GetSubMeshes();
			m_MaterialSubMeshes.reserve(subMeshes.size());
			for (KSubMeshPtr subMesh : subMeshes)
			{
				KMaterialSubMeshPtr materialSubMesh = KMaterialSubMeshPtr(KNEW KMaterialSubMesh(subMesh.get()));
				materialSubMesh->Init(m_Material.get());
				m_MaterialSubMeshes.push_back(materialSubMesh);
			}

			KRenderGlobal::MeshManager.AcquireOCQuery(m_OCQueries);
			m_OCInstanceQueries.resize(m_OCQueries.size());
			return true;
		}
	}
	else if (m_Type == DEBUG_UTILITY)
	{
		if (KRenderGlobal::MeshManager.AcquireAsUtility(m_DebugUtility, m_Mesh))
		{
			const std::vector<KSubMeshPtr>& subMeshes = (*m_Mesh)->GetSubMeshes();
			m_MaterialSubMeshes.reserve(subMeshes.size());
			for (KSubMeshPtr subMesh : subMeshes)
			{
				KMaterialSubMeshPtr materialSubMesh = KMaterialSubMeshPtr(KNEW KMaterialSubMesh(subMesh.get()));
				materialSubMesh->InitDebug(subMesh->GetDebugPrimitive());
				m_MaterialSubMeshes.push_back(materialSubMesh);
			}
		}
	}
	return false;
}

bool KRenderComponent::UnInit()
{
	for (KMaterialSubMeshPtr& materialSubMesh : m_MaterialSubMeshes)
	{
		materialSubMesh->UnInit();
	}
	m_MaterialSubMeshes.clear();
	if (m_Mesh)
	{
		m_Mesh.Release();
	}
	if (m_Material)
	{
		KRenderGlobal::MaterialManager.Release(m_Material);
		m_Material = nullptr;
	}
	if (!m_OCQueries.empty())
	{
		KRenderGlobal::MeshManager.ReleaseOCQuery(m_OCQueries);
		m_OCQueries.clear();
	}
	m_OCInstanceQueries.clear();
	return true;
}

bool KRenderComponent::InitUtility(const KMeshUtilityInfoPtr& info)
{
	UnInit();
	m_Type = DEBUG_UTILITY;
	m_DebugUtility = info;
	if (KRenderGlobal::MeshManager.AcquireAsUtility(info, m_Mesh))
	{
		const std::vector<KSubMeshPtr>& subMeshes = (*m_Mesh)->GetSubMeshes();
		m_MaterialSubMeshes.reserve(subMeshes.size());
		for (KSubMeshPtr subMesh : subMeshes)
		{
			KMaterialSubMeshPtr materialSubMesh = KMaterialSubMeshPtr(KNEW KMaterialSubMesh(subMesh.get()));
			materialSubMesh->InitDebug(subMesh->GetDebugPrimitive());
			m_MaterialSubMeshes.push_back(materialSubMesh);
		}
		return true;
	}
	return false;
}

bool KRenderComponent::UpdateUtility(const KMeshUtilityInfoPtr& info)
{
	ASSERT_RESULT(m_Type == DEBUG_UTILITY);
	ASSERT_RESULT(m_DebugUtility->GetType() == info->GetType());
	m_DebugUtility = info;
	return KRenderGlobal::MeshManager.UpdateUtility(info, m_Mesh);
}

bool KRenderComponent::Visit(PipelineStage stage, std::function<void(KRenderCommand&)> func)
{
	for (KMaterialSubMeshPtr& materialSubMesh : m_MaterialSubMeshes)
	{
		materialSubMesh->Visit(stage, func);
	}
	return true;
}

bool KRenderComponent::GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as)
{
	if (m_Mesh)
	{
		return (*m_Mesh)->GetAllAccelerationStructure(as);
	}
	return false;
}

IKQueryPtr KRenderComponent::GetOCQuery()
{
	return KRenderGlobal::CurrentFrameIndex < m_OCQueries.size() ? m_OCQueries[KRenderGlobal::CurrentFrameIndex] : nullptr;
}

IKQueryPtr KRenderComponent::GetOCInstacneQuery()
{
	return KRenderGlobal::CurrentFrameIndex < m_OCInstanceQueries.size() ? m_OCInstanceQueries[KRenderGlobal::CurrentFrameIndex] : nullptr;
}

bool KRenderComponent::SetOCInstanceQuery(IKQueryPtr query)
{
	if (KRenderGlobal::CurrentFrameIndex < m_OCInstanceQueries.size())
	{
		m_OCInstanceQueries[KRenderGlobal::CurrentFrameIndex] = query;
		return true;
	}
	return false;
}