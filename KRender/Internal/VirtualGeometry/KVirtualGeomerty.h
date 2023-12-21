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

	uint32_t clusterVertexStorageByteOffset = 0;
	uint32_t clusterVertexStorageByteSize = 0;

	uint32_t clusterIndexStorageByteOffset = 0;
	uint32_t clusterIndexStorageByteSize= 0;

	uint32_t clusterMaterialStorageByteOffset = 0;
	uint32_t clusterMaterialStorageByteSize = 0;

	uint32_t materialBaseIndex = 0;
	uint32_t materialNum = 0;

	uint32_t padding[3] = { 0 };
};
static_assert((sizeof(KVirtualGeometryResource) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryInstance
{
	glm::mat4 prevTransform;
	glm::mat4 transform;
	uint32_t resourceIndex;
	uint32_t binningBaseIndex;
	uint32_t padding[2] = { 0 };
};
static_assert((sizeof(KVirtualGeometryInstance) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryGlobal
{
	glm::mat4 worldToClip;
	glm::mat4 prevWorldToClip;
	glm::mat4 worldToView;
	glm::mat4 worldToTranslateView;
	glm::vec4 misc;
	glm::uvec4 miscs2;
};

struct KVirtualGeometryMaterial
{
	glm::uvec4 miscs3;
};

struct KVirtualGeometryQueueState
{
	uint32_t nodeReadOffset = 0;
	uint32_t nodePrevWriteOffset = 0;
	uint32_t nodeWriteOffset = 0;
	uint32_t nodeCount = 0;
	uint32_t clusterReadOffset = 0;
	uint32_t clusterWriteOffset = 0;
	uint32_t visibleClusterNum = 0;
	uint32_t binningWriteOffset = 0;
};
static_assert((sizeof(KVirtualGeometryQueueState) % 16) == 0, "Size must be a multiple of 16");

struct KVirtualGeometryStreamingRequest
{
	uint32_t resourceIndex;
	uint32_t pageStart;
	uint32_t pageNum;
	uint32_t priority;
};
static_assert((sizeof(KVirtualGeometryStreamingRequest) % 16) == 0, "Size must be a multiple of 16");

struct KMeshClusterHierarchyPackedNode
{
	glm::vec4 lodBoundCenterError;
	glm::vec4 lodBoundHalfExtendRadius;
	uint32_t children[KVirtualGeometryDefine::MAX_BVH_NODES];
	uint32_t isLeaf = 0;
	uint32_t clusterStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t clusterNum = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t clusterPageIndex = 0;
	uint32_t groupPageStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t groupPageNum = 0;
	uint32_t padding[2] = { 0 };
};

static_assert(sizeof(KMeshClusterHierarchyPackedNode) == KVirtualGeometryDefine::MAX_BVH_NODES * 4 + 64, "size check");
static_assert((sizeof(KMeshClusterHierarchyPackedNode) % 16) == 0, "Size must be a multiple of 16");

typedef KReferenceHolder<KVirtualGeometryResource*> KVirtualGeometryResourceRef;