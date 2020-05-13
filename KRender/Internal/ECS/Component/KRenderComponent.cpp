#include "KRenderComponent.h"
#include "Internal/KRenderGlobal.h"

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME KRenderComponent
#define KRTTR_REG_CLASS_NAME_STR "RenderComponent"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_PROPERTY_READ_ONLY(path, GetPathCStr, MDT_CSTR)
	KRTTR_REG_PROPERTY_READ_ONLY(type, GetTypeCStr, MDT_CSTR)
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}

KRenderComponent::KRenderComponent()
	: m_Mesh(nullptr),
	m_Type(NONE)
{}

KRenderComponent::~KRenderComponent()
{
	UnInit();
}

const char* KRenderComponent::ResourceTypeToString(KRenderComponent::ResourceType type)
{
#define ENUM(type) case type: return #type;
	switch (type)
	{
		ENUM(MESH);
		ENUM(ASSET);
		ENUM(UTILITY);
		ENUM(NONE);
	default:
		assert(false);
		return "UNKNOWN";
	}
#undef ENUM
}

KRenderComponent::ResourceType KRenderComponent::StringToResourceType(const char* str)
{
#define CMP(enum_string) if (!strcmp(str, #enum_string)) return enum_string;
	CMP(MESH);
	CMP(ASSET);
	CMP(UTILITY);
	CMP(NONE);
	return NONE;
#undef CMP
}

bool KRenderComponent::Save(IKXMLElementPtr element)
{
	if (m_Type == MESH || m_Type == ASSET)
	{
		IKXMLElementPtr typeEle = element->NewElement(msType);
		typeEle->SetText(ResourceTypeToString(m_Type));

		IKXMLElementPtr pathEle = element->NewElement(msPath);
		pathEle->SetText(m_Path.c_str());

		return true;
	}
	return false;
}

bool KRenderComponent::Load(IKXMLElementPtr element)
{
	IKXMLElementPtr typeEle = element->FirstChildElement(msType);	
	ACTION_ON_FAILURE(typeEle != nullptr && !typeEle->IsEmpty(), return false);

	ResourceType type = StringToResourceType(typeEle->GetText().c_str());
	ACTION_ON_FAILURE(type == MESH || type == ASSET, return false);

	IKXMLElementPtr pathEle = element->FirstChildElement(msPath);
	ACTION_ON_FAILURE(pathEle != nullptr && !pathEle->IsEmpty(), return false);

	std::string path = pathEle->GetText();
	ACTION_ON_FAILURE(!path.empty(), return false);

	UnInit();

	m_Type = type;
	m_Path = path;

	return Init();
}

bool KRenderComponent::GetLocalBound(KAABBBox& bound) const
{
	if (m_Mesh)
	{
		bound = m_Mesh->GetLocalBound();
		return true;
	}
	return false;
}

bool KRenderComponent::Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const
{
	if (m_Mesh)
	{
		const KTriangleMesh& triangleMesh = m_Mesh->GetTriangleMesh();
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
		const KTriangleMesh& triangleMesh = m_Mesh->GetTriangleMesh();
		if (triangleMesh.Pick(localOrigin, localDir, result))
		{
			return true;
		}
	}
	return false;
}

bool KRenderComponent::SetPathMesh(const char* path)
{
	if (path)
	{
		m_Type = MESH;
		m_Path = path;
		return true;
	}
	return false;
}

bool KRenderComponent::SetPathAsset(const char* path)
{
	if (path)
	{
		m_Type = ASSET;
		m_Path = path;
		return true;
	}
	return false;
}

bool KRenderComponent::GetPath(std::string& path) const
{
	if (m_Type == ASSET || m_Type == MESH)
	{
		path = m_Path;
		return true;
	}
	return false;
}

bool KRenderComponent::Init()
{
	UnInit();
	if (m_Type == MESH)
	{
		return KRenderGlobal::MeshManager.Acquire(m_Path.c_str(), m_Mesh);
	}
	else if (m_Type == ASSET)
	{
		return KRenderGlobal::MeshManager.AcquireFromAsset(m_Path.c_str(), m_Mesh);
	}
	else if (m_Type == UTILITY)
	{
		return KRenderGlobal::MeshManager.AcquireAsUtility(m_UtilityInfo, m_Mesh);
	}
	else
	{
		return false;
	}
}

bool KRenderComponent::UnInit()
{
	if (m_Mesh)
	{
		KRenderGlobal::MeshManager.Release(m_Mesh);
		m_Mesh = nullptr;
		return true;
	}
	return true;
}

bool KRenderComponent::InitUtility(const KMeshUtilityInfoPtr& info)
{
	UnInit();
	m_Type = UTILITY;
	m_UtilityInfo = info;
	return KRenderGlobal::MeshManager.AcquireAsUtility(info, m_Mesh);
}

bool KRenderComponent::UpdateUtility(const KMeshUtilityInfoPtr& info)
{
	ASSERT_RESULT(m_Type == UTILITY);
	ASSERT_RESULT(m_UtilityInfo->GetType() == info->GetType());
	m_UtilityInfo = info;
	return KRenderGlobal::MeshManager.UpdateUtility(info, m_Mesh);
}