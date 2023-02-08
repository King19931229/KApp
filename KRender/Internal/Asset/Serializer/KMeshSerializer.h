#pragma once
#include "Internal/Asset/KMesh.h"

#define FOURCC(c0, c1, c2, c3) (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24))

#define MESH_HEAD FOURCC('M','E','S','H')
#define MESH_MAGIC FOURCC('L','G','Y','F')

enum MeshSerializerVersion : uint32_t
{
	MSV_VERSION_0_0 = FOURCC(0, 0, 0, 0),
	MSV_VERSION_NEWEST = MSV_VERSION_0_0,
};

namespace KMeshSerializer
{
	bool LoadFromFile(IKRenderDevice* device, KMesh* pMesh, const char* path, bool hostVisible);
	bool SaveAsFile(const KMesh* pMesh, const char* path, MeshSerializerVersion version);
}