#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Asset/KMesh.h"
#include <unordered_map>
#include <unordered_set>

class KMeshManager
{
protected:
	struct MeshUsingInfo
	{
		size_t useCount;
		KMeshPtr mesh;
	};

	typedef std::unordered_map<std::string, MeshUsingInfo> MeshMap;
	typedef std::unordered_set<KMeshPtr> SpecialMesh;

	MeshMap m_Meshes;
	SpecialMesh m_SpecialMesh;

	IKRenderDevice* m_Device;
	size_t m_FrameInFlight;

	bool AcquireImpl(const char* path, bool fromAsset, KMeshPtr& ptr);
public:
	KMeshManager();
	~KMeshManager();

	bool Init(IKRenderDevice* device, size_t frameInFlight);
	bool UnInit();

	bool Acquire(const char* path, KMeshPtr& ptr);
	bool AcquireFromAsset(const char* path, KMeshPtr& ptr);
	bool Release(KMeshPtr& ptr);

	// utility
	bool CreateBox(const glm::vec3& halfExtent, KMeshPtr& ptr);
	bool CreateQuad(const glm::mat4& transform, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, KMeshPtr& ptr);
	bool CreateCone(const glm::mat4& transform, float height, float radius, KMeshPtr& ptr);
	bool CreateCylinder(const glm::mat4& transform, float height, float radius, KMeshPtr& ptr);
	bool CreateCircle(const glm::mat4& transform, float radius, KMeshPtr& ptr);
	bool CreateArc(const glm::mat4& transform, float radius, float theta, KMeshPtr& ptr);
	bool CreateSphere(const glm::mat4& transform, float radius, KMeshPtr& ptr);
};