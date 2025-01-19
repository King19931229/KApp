#include "KRenderComponent.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KRenderThread.h"

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME KRenderComponent
#define KRTTR_REG_CLASS_NAME_STR "RenderComponent"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_PROPERTY_READ_ONLY(path, GetResourcePathString, MDT_STDSTRING)
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}

KRenderComponent::KRenderComponent()
	: m_UseMaterialTexture(false)
	, m_OcclusionVisible(true)
{}

KRenderComponent::~KRenderComponent()
{
	UnInit();
}

const char* KRenderComponent::MeshResourceTypeToString(MeshResourceType type)
{
#define ENUM(type) case type: return #type;
	switch (type)
	{
		ENUM(MRT_INTERNAL_MESH);
		ENUM(MRT_EXTERNAL_ASSET);
		ENUM(MRT_DEBUG_UTILITY);
		ENUM(MRT_UNKNOWN);
		default:
			assert(false);
			return "MRT_UNKNOWN";
	}
#undef ENUM
}

MeshResourceType KRenderComponent::StringToMeshResourceType(const char* str)
{
#define CMP(enum_string) if (!strcmp(str, #enum_string)) return enum_string;
	CMP(MRT_INTERNAL_MESH);
	CMP(MRT_EXTERNAL_ASSET);
	CMP(MRT_DEBUG_UTILITY);
	CMP(MRT_UNKNOWN);
	return MRT_UNKNOWN;
#undef CMP
}

std::string KRenderComponent::GetResourcePathString() const
{
	return m_Mesh ? m_Mesh->GetPath() : "";
}

std::string KRenderComponent::GetTypeString() const
{
	return m_Mesh ? MeshResourceTypeToString(m_Mesh->GetType()) : "";
}

bool KRenderComponent::Save(IKXMLElementPtr element)
{
	if (m_Mesh && m_Mesh->GetType() != MRT_DEBUG_UTILITY)
	{
		IKXMLElementPtr typeEle = element->NewElement(msType);
		typeEle->SetText(GetTypeString().c_str());

		IKXMLElementPtr pathEle = element->NewElement(msPath);
		pathEle->SetText(GetResourcePathString().c_str());

		return true;
	}
	return false;
}

bool KRenderComponent::Load(IKXMLElementPtr element)
{
	IKXMLElementPtr typeEle = element->FirstChildElement(msType);	
	ACTION_ON_FAILURE(typeEle != nullptr && !typeEle->IsEmpty(), return false);

	MeshResourceType type = StringToMeshResourceType(typeEle->GetText().c_str());
	ACTION_ON_FAILURE(type == MRT_INTERNAL_MESH || type == MRT_EXTERNAL_ASSET, return false);

	IKXMLElementPtr pathEle = element->FirstChildElement(msPath);
	ACTION_ON_FAILURE(pathEle != nullptr && !pathEle->IsEmpty(), return false);

	std::string path = pathEle->GetText();
	ACTION_ON_FAILURE(!path.empty(), return false);

	UnInit();

	if (type == MRT_INTERNAL_MESH)
	{
		return InitAsMesh(path, true);
	}
	else if (type == MRT_EXTERNAL_ASSET)
	{
		return InitAsAsset(path, true);
	}

	return false;
}

bool KRenderComponent::GetLocalBound(KAABBBox& bound) const
{
	if (m_Mesh)
	{
		bound = (*m_Mesh)->GetLocalBound();
		return true;
	}
	else if (m_VGResource)
	{
		bound.InitFromHalfExtent(m_VGResource->boundCenter, m_VGResource->boundHalfExtend);
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

bool KRenderComponent::GetPath(std::string& path) const
{
	if (m_Mesh)
	{
		path = m_Mesh->GetPath();
		return true;
	}
	return false;
}

bool KRenderComponent::SaveAsMesh(const std::string& path) const
{
	if (m_Mesh && m_Mesh->GetType() != MRT_DEBUG_UTILITY)
	{
		return (*m_Mesh)->SaveAsFile(path.c_str());
	}
	return false;
}

void KRenderComponent::MeshPostInit()
{
	KRenderGlobal::MeshManager.AcquireOCQuery(m_OCQueries);
	m_OCInstanceQueries.resize(m_OCQueries.size());

	const std::vector<KSubMeshPtr>& subMeshes = m_Mesh->GetSubMeshes();
	const std::vector<KMaterialRef>& subMaterials = m_Mesh->GetSubMaterials();

	m_MaterialSubMeshes.reserve(subMeshes.size());
	for (size_t i = 0; i < subMeshes.size(); ++i)
	{
		KMaterialSubMeshPtr materialSubMesh = KMaterialSubMeshPtr(KNEW KMaterialSubMesh());
		m_MaterialSubMeshes.push_back(materialSubMesh);
		materialSubMesh->Init(subMeshes[i], subMaterials[i]);
	}

	for (RenderComponentObserverFunc* callback : m_Callbacks)
	{
		(*callback)(this, true);
	}
}

void KRenderComponent::VirtualGeometryPostInit()
{
	for (RenderComponentObserverFunc* callback : m_Callbacks)
	{
		(*callback)(this, true);
	}
}

bool KRenderComponent::InitAsMesh(const std::string& mesh, bool async)
{
	UnInit();
	ENQUEUE_RENDER_COMMAND(KRenderComponent_InitAsAsset)([this, mesh, async]()
	{
		bool meshAcquire = KRenderGlobal::MeshManager.Acquire(mesh.c_str(), m_Mesh);
		if (meshAcquire)
		{
			MeshPostInit();
			return true;
		}
		return false;
	});

	return true;
}

bool KRenderComponent::InitAsAsset(const std::string& asset, bool async)
{
	UnInit();
	ENQUEUE_RENDER_COMMAND(KRenderComponent_InitAsAsset)([this, asset, async]()
	{
		bool meshAcquire = KRenderGlobal::MeshManager.AcquireFromAsset(asset.c_str(), m_Mesh);
		if (meshAcquire)
		{
			MeshPostInit();
			return true;
		}
		return false;
	});

	return true;
}

bool KRenderComponent::InitAsUserData(const KMeshRawData& userData, const std::string& label, bool async)
{
	UnInit();
	ENQUEUE_RENDER_COMMAND(KRenderComponent_UnInit)([this, userData, label]()
	{
		bool meshAcquire = KRenderGlobal::MeshManager.AcquireFromUserData(userData, label, m_Mesh);
		if (meshAcquire)
		{
			MeshPostInit();
			return true;
		}
		return false;
	});

	return true;
}

bool KRenderComponent::UnInit()
{
	ENQUEUE_RENDER_COMMAND(KRenderComponent_UnInit)([this]()
	{
		KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThread);

		for (RenderComponentObserverFunc* callback : m_Callbacks)
		{
			(*callback)(this, false);
		}
		m_Callbacks.clear();

		for (KMaterialSubMeshPtr& materialSubMesh : m_MaterialSubMeshes)
		{
			materialSubMesh->UnInit();
		}
		m_MaterialSubMeshes.clear();
		if (m_Mesh)
		{
			m_Mesh.Release();
		}
		if (!m_OCQueries.empty())
		{
			KRenderGlobal::MeshManager.ReleaseOCQuery(m_OCQueries);
			m_OCQueries.clear();
		}
		m_OCInstanceQueries.clear();
		m_VGResource.Release();
	});
	FLUSH_RENDER_COMMAND();
	return true;
}

bool KRenderComponent::InitAsDebugUtility(const KDebugUtilityInfo& info)
{
	UnInit();

	ENQUEUE_RENDER_COMMAND(KRenderComponent_InitAsDebugUtility)([this, info]()
	{
		KRenderGlobal::MeshManager.New(m_Mesh);
		m_Mesh->InitFromUtility(info);

		const std::vector<KSubMeshPtr>& subMeshes = m_Mesh->GetSubMeshes();
		m_MaterialSubMeshes.reserve(subMeshes.size());
		for (size_t i = 0; i < subMeshes.size(); ++i)
		{
			KMaterialSubMeshPtr materialSubMesh = KMaterialSubMeshPtr(KNEW KMaterialSubMesh());
			materialSubMesh->InitDebug(subMeshes[i], subMeshes[i]->GetDebugPrimitive());
			m_MaterialSubMeshes.push_back(materialSubMesh);
		}
	});

	return true;
}

bool KRenderComponent::InitAsVirtualGeometry(const KMeshRawData& userData, const std::string& label)
{
	UnInit();
	ENQUEUE_RENDER_COMMAND(KRenderComponent_UnInit)([this, userData, label]()
	{
		if (KRenderGlobal::VirtualGeometryManager.AcquireFromUserData(userData, label, m_VGResource))
		{
			VirtualGeometryPostInit();
			return true;
		}
		return false;
	});

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

bool KRenderComponent::GetAllTextrueBinding(std::vector<IKMaterialTextureBindingPtr>& binding)
{
	binding.clear();
	binding.reserve(m_MaterialSubMeshes.size());
	for (KMaterialSubMeshPtr& materialSubMesh : m_MaterialSubMeshes)
	{
		KMaterialRef material = materialSubMesh->GetMaterial();
		if (!material || !material->GetTextureBinding())
		{
			binding.clear();
			return false;
		}
		binding.push_back(material->GetTextureBinding());
	}
	return true;
}

bool KRenderComponent::RegisterCallback(RenderComponentObserverFunc* callback)
{
	if (callback)
	{
		auto it = std::find(m_Callbacks.begin(), m_Callbacks.end(), callback);
		if (it == m_Callbacks.end())
		{
			m_Callbacks.push_back(callback);
		}
		return true;
	}
	return false;
}

bool KRenderComponent::UnRegisterCallback(RenderComponentObserverFunc* callback)
{
	if (callback)
	{
		auto it = std::find(m_Callbacks.begin(), m_Callbacks.end(), callback);
		if (it != m_Callbacks.end())
		{
			m_Callbacks.erase(it);
		}
		return true;
	}
	return false;
}

IKQueryPtr KRenderComponent::GetOCQuery()
{
	return KRenderGlobal::CurrentInFlightFrameIndex < m_OCQueries.size() ? m_OCQueries[KRenderGlobal::CurrentInFlightFrameIndex] : nullptr;
}

IKQueryPtr KRenderComponent::GetOCInstacneQuery()
{
	return KRenderGlobal::CurrentInFlightFrameIndex < m_OCInstanceQueries.size() ? m_OCInstanceQueries[KRenderGlobal::CurrentInFlightFrameIndex] : nullptr;
}

bool KRenderComponent::SetOCInstanceQuery(IKQueryPtr query)
{
	if (KRenderGlobal::CurrentInFlightFrameIndex < m_OCInstanceQueries.size())
	{
		m_OCInstanceQueries[KRenderGlobal::CurrentInFlightFrameIndex] = query;
		return true;
	}
	return false;
}