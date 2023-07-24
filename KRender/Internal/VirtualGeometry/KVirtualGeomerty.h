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

	uint32_t clusterVertexStorageOffset = 0;
	uint32_t clusterVertexStorageSize = 0;

	uint32_t clusterIndexStorageOffset = 0;
	uint32_t clusterIndexStorageSize = 0;

	uint32_t materialIndex = KVirtualGeometryDefine::INVALID_INDEX;

	uint32_t padding[2];
};
static_assert((sizeof(KVirtualGeometryResource) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryInstance
{
	glm::mat4 transform;
	uint32_t resourceIndex;
	uint32_t padding[3];
};
static_assert((sizeof(KVirtualGeometryInstance) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryGlobal
{
	glm::mat4 worldToClip;
	glm::mat4 worldToView;
	glm::vec4 misc;
	uint32_t numInstance;
};

struct KVirtualGeometryQueueState
{
	uint32_t nodeReadOffset = 0;
	uint32_t nodePrevWriteOffset = 0;
	uint32_t nodeWriteOffset = 0;
	uint32_t clusterReadOffset = 0;
	uint32_t clusterWriteOffset = 0;
	uint32_t selectedClusterReadOffset = 0;
	uint32_t selectedClusterWriteOffset = 0;
	uint32_t padding = 0;
};
static_assert((sizeof(KVirtualGeometryQueueState) % 16) == 0, "Size must be a multiple of 16");

struct KMeshClusterHierarchyPackedNode
{
	glm::vec4 lodBoundCenterError;
	glm::vec4 lodBoundHalfExtendRadius;
	uint32_t children[KVirtualGeometryDefine::MAX_BVH_NODES];
	uint32_t isLeaf = 0;
	uint32_t clusterStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t clusterNum = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t padding = 0;
};

static_assert(sizeof(KMeshClusterHierarchyPackedNode) == KVirtualGeometryDefine::MAX_BVH_NODES * 4 + 16 + 32, "size check");
static_assert((sizeof(KMeshClusterHierarchyPackedNode) % 16) == 0, "Size must be a multiple of 16");

typedef KReferenceHolder<KVirtualGeometryResource*> KVirtualGeometryResourceRef;