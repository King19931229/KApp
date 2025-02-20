#pragma once
#include "KBase/Publish/Mesh/KVirtualGeometryBuilder.h"
#include "KBase/Publish/KReferenceHolder.h"
#include "Interface/IKRenderConfig.h"

enum VirtualGeometryBinding
{
	#define VIRTUAL_GEOMETRY_BINDING(SEMANTIC) BINDING_##SEMANTIC,
	#include "KVirtualGeomertyBinding.inl"
	#undef VIRTUAL_GEOMETRY_BINDING
};

enum VirtualGeometryConstant
{
	VG_GROUP_SIZE = 64,
	VG_MESH_SHADER_GROUP_SIZE = 128,
	MAX_CANDIDATE_NODE = 1024 * 1024,
	MAX_CANDIDATE_CLUSTERS = 1024 * 1024 * 4,
	MAX_STREAMING_REQUEST = 256 * 1024,
	// 避开全局UBO
	VG_BASEPASS_BINDING_OFFSET = SHADER_BINDING_TEXTURE0
};

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
	glm::uvec4 misc2;
};

struct KVirtualGeometryMaterial
{
	glm::uvec4 misc3;
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

	friend bool operator==(const KVirtualGeometryStreamingRequest& lhs, const KVirtualGeometryStreamingRequest& rhs);
};
static_assert((sizeof(KVirtualGeometryStreamingRequest) % 16) == 0, "Size must be a multiple of 16");

template<>
struct std::hash<KVirtualGeometryStreamingRequest>
{
	inline std::size_t operator()(const KVirtualGeometryStreamingRequest& desc) const
	{
		std::size_t hash = 0;
		KHash::HashCombine(hash, desc.resourceIndex);
		KHash::HashCombine(hash, desc.pageNum);
		KHash::HashCombine(hash, desc.pageStart);
		KHash::HashCombine(hash, desc.priority);
		return hash;
	}
};

inline bool operator==(const KVirtualGeometryStreamingRequest& lhs, const KVirtualGeometryStreamingRequest& rhs)
{
	return lhs.resourceIndex == rhs.resourceIndex && lhs.pageStart == rhs.pageStart && lhs.pageNum == rhs.pageNum && lhs.priority == rhs.priority;
}

struct KMeshClusterHierarchyPackedNode
{
	// Global page index
	uint32_t gpuPageIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t isLeaf = 0;
	// Global cluster start index
	uint32_t clusterStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t clusterNum = KVirtualGeometryDefine::INVALID_INDEX;
	// Local page cluster start index
	uint32_t clusterPageStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t groupPageStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t groupPageNum = 0;
	uint32_t padding = 0;
	uint32_t children[KVirtualGeometryDefine::MAX_BVH_NODES];
	glm::vec4 lodBoundCenterError;
	glm::vec4 lodBoundHalfExtendRadius;
};

static_assert(sizeof(KMeshClusterHierarchyPackedNode) == KVirtualGeometryDefine::MAX_BVH_NODES * 4 + 64, "size check");
static_assert((sizeof(KMeshClusterHierarchyPackedNode) % 16) == 0, "Size must be a multiple of 16");

typedef KReferenceHolder<KVirtualGeometryResource*> KVirtualGeometryResourceRef;