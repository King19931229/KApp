#include "KMeshManager.h"
#include "Internal/Asset/Utility/KMeshUtility.h"

KMeshManager::KMeshManager()
	: m_Device(nullptr),
	m_FrameInFlight(0)
{
}

KMeshManager::~KMeshManager()
{
	assert(m_Meshes.empty());
	assert(m_SpecialMesh.empty());
}

bool KMeshManager::Init(IKRenderDevice* device, size_t frameInFlight)
{
	UnInit();

	m_Device = device;
	m_FrameInFlight = frameInFlight;

	return true;
}

bool KMeshManager::UnInit()
{
	assert(m_Meshes.empty());

	for(auto it = m_Meshes.begin(), itEnd = m_Meshes.end(); it != itEnd; ++it)
	{
		MeshUsingInfo& info = it->second;
		assert(info.mesh);
		info.mesh->UnInit();
	}
	m_Meshes.clear();

	for (KMeshPtr mesh : m_SpecialMesh)
	{
		mesh->UnInit();
	}
	m_SpecialMesh.clear();

	m_Device = nullptr;
	m_FrameInFlight = 0;

	return true;
}

bool KMeshManager::AcquireImpl(const char* path, bool fromAsset, KMeshPtr& ptr)
{
	auto it = m_Meshes.find(path);

	if(it != m_Meshes.end())
	{
		MeshUsingInfo& info = it->second;
		info.useCount += 1;
		ptr = info.mesh;
		return true;
	}

	ptr = KMeshPtr(new KMesh());

	bool bRetValue = false;
	if(fromAsset)
	{
		bRetValue = ptr->InitFromAsset(path, m_Device, m_FrameInFlight);
	}
	else
	{
		bRetValue = ptr->InitFromFile(path, m_Device, m_FrameInFlight);
	}

	if(bRetValue)
	{
		MeshUsingInfo info = { 1, ptr };
		m_Meshes[path] = info;
		return true;
	}

	ptr = nullptr;
	return false;
}

bool KMeshManager::Acquire(const char* path, KMeshPtr& ptr)
{
	return AcquireImpl(path, false, ptr);
}

bool KMeshManager::AcquireFromAsset(const char* path, KMeshPtr& ptr)
{
	return AcquireImpl(path, true, ptr);
}

bool KMeshManager::Release(KMeshPtr& ptr)
{
	if(ptr)
	{
		const auto& path = ptr->GetPath();

		if (!path.empty())
		{
			auto it = m_Meshes.find(path);
			if (it != m_Meshes.end())
			{
				MeshUsingInfo& info = it->second;
				info.useCount -= 1;

				if (info.useCount == 0)
				{
					ptr->UnInit();
					m_Meshes.erase(it);
				}

				ptr = nullptr;
				return true;
			}
		}
		else
		{
			auto it = m_SpecialMesh.find(ptr);
			if (it != m_SpecialMesh.end())
			{
				m_SpecialMesh.erase(it);
				ptr->UnInit();
				ptr = nullptr;
				return true;
			}
		}
	}
	return false;
}

bool KMeshManager::CreateBox(const glm::vec3& halfExtent, KMeshPtr& ptr)
{
	ptr = KMeshPtr(new KMesh());
	if (ptr->InitAsBox(halfExtent, m_Device, m_FrameInFlight))
	{
		m_SpecialMesh.insert(ptr);
		return true;
	}
	ptr = nullptr;
	return false;
}

bool KMeshManager::CreateQuad(const glm::mat4& transform, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, KMeshPtr& ptr)
{
	ptr = KMeshPtr(new KMesh());
	if (ptr->InitAsQuad(transform, lengthU, lengthV, axisU, axisV, m_Device, m_FrameInFlight))
	{
		m_SpecialMesh.insert(ptr);
		return true;
	}
	ptr = nullptr;
	return false;
}

bool KMeshManager::CreateCone(const glm::mat4& transform, float height, float radius, KMeshPtr& ptr)
{
	ptr = KMeshPtr(new KMesh());
	if (ptr->InitAsCone(transform, height, radius, m_Device, m_FrameInFlight))
	{
		m_SpecialMesh.insert(ptr);
		return true;
	}
	ptr = nullptr;
	return false;
}

bool KMeshManager::CreateCylinder(const glm::mat4& transform, float height, float radius, KMeshPtr& ptr)
{
	ptr = KMeshPtr(new KMesh());
	if (ptr->InitAsCylinder(transform, height, radius, m_Device, m_FrameInFlight))
	{
		m_SpecialMesh.insert(ptr);
		return true;
	}
	ptr = nullptr;
	return false;
}

bool KMeshManager::CreateCircle(const glm::mat4& transform, float radius, KMeshPtr& ptr)
{
	ptr = KMeshPtr(new KMesh());
	if (ptr->InitAsCircle(transform, radius, m_Device, m_FrameInFlight))
	{
		m_SpecialMesh.insert(ptr);
		return true;
	}
	ptr = nullptr;
	return false;
}

bool KMeshManager::CreateSphere(const glm::mat4& transform, float radius, KMeshPtr& ptr)
{
	ptr = KMeshPtr(new KMesh());
	if (ptr->InitAsSphere(transform, radius, m_Device, m_FrameInFlight))
	{
		m_SpecialMesh.insert(ptr);
		return true;
	}
	ptr = nullptr;
	return false;
}