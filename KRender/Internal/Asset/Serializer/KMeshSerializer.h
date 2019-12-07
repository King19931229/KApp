#pragma once
#include "Internal/Asset/KMesh.h"

enum MeshSerializeVersion
{
	MSV_VERSION_0,
	MSV_VERSION_COUNT,
};

namespace KMeshSerializer
{
	bool LoadFromFile(KMesh* pMesh, const char* path);
	bool SaveAsFile(KMesh* pMesh, const char* path, MeshSerializeVersion version);
}