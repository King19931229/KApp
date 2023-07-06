#pragma once
#include "KBase/Publish/Mesh/KVirtualGeometryBuilder.h"
#include "KBase/Publish/KReferenceHolder.h"

struct KVirtualGeometryResource
{
	uint32_t resourceIndex = 0;

	uint32_t clusterBatchOffset = 0;
	uint32_t clusterBatchSize = 0;

	uint32_t hierarchyPackedOffset = 0;
	uint32_t hierarchyPackedSize = 0;

	uint32_t clusterStorageOffset = 0;
	uint32_t clusterStorageSize = 0;
};

typedef KReferenceHolder<KVirtualGeometryResource*> KVirtualGeometryResourceRef;