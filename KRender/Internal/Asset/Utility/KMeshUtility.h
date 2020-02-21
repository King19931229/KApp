#pragma once
#include "Internal/Asset/KMesh.h"
#include "Publish/KAABBBox.h"

class KMeshUtilityImpl
{
protected:
	IKRenderDevice* m_Device;
public:
	KMeshUtilityImpl(IKRenderDevice* device);
	~KMeshUtilityImpl();

	bool CreateBox(KMesh* pMesh, const KAABBBox& bound, size_t frameInFlight);
};

namespace KMeshUtility
{
	bool CreateBox(IKRenderDevice* device, KMesh* pMesh, const KAABBBox& bound, size_t frameInFlight);
}