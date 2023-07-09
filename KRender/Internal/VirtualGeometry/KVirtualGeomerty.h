#pragma once
#include "KBase/Publish/Mesh/KVirtualGeometryBuilder.h"
#include "KBase/Publish/KReferenceHolder.h"

struct KVirtualGeometryResource
{
	glm::vec4 boundCenter;
	glm::vec4 boundHalfExtend;

	uint32_t resourceIndex = 0;

	uint32_t clusterBatchOffset = 0;
	uint32_t clusterBatchSize = 0;

	uint32_t hierarchyPackedOffset = 0;
	uint32_t hierarchyPackedSize = 0;

	uint32_t clusterStorageOffset = 0;
	uint32_t clusterStorageSize = 0;
};

struct KVirtualGeometryInstance
{
	glm::mat4 transform;
	uint32_t resourceIndex;
};

struct KVirtualGeometryGlobal
{
	glm::mat4 worldToClip;
	uint32_t numInstance;
};

typedef KReferenceHolder<KVirtualGeometryResource*> KVirtualGeometryResourceRef;