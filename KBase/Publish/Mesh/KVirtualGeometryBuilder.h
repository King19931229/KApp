#pragma once
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include "KBase/Publish/Mesh/KMeshSimplification.h"
#include "KBase/Publish/KFileTool.h"
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <metis.h>

struct KRange
{
	uint32_t begin = 0;
	uint32_t end = 0;
};

struct KMeshCluster;
typedef std::shared_ptr<KMeshCluster> KMeshClusterPtr;

struct KVirtualGeometryDefine
{
	static constexpr uint32_t MAX_BVH_NODES_BITS = 2;
	static constexpr uint32_t MAX_BVH_NODES_BIT_MAX = (1 << MAX_BVH_NODES_BITS) - 1;
	static constexpr uint32_t MAX_BVH_NODES = 1 << MAX_BVH_NODES_BITS;
	static constexpr uint32_t INVALID_INDEX = -1;

	static constexpr uint32_t MAX_CLUSTER_TRIANGLE = 128;
	static constexpr uint32_t MAX_CLUSTER_VERTEX = 256;
	static constexpr uint32_t MAX_CLUSTER_GROUP = 32;
};
static_assert(!(KVirtualGeometryDefine::MAX_BVH_NODES& (KVirtualGeometryDefine::MAX_BVH_NODES - 1)), "MAX_BVH_NODES must be pow of 2");

struct KMeshCluster
{
	struct Triangle
	{
		int32_t index[3] = { -1, -1 ,-1 };
	};

	std::vector<KMeshProcessorVertex> vertices;
	std::vector<uint32_t> indices;

	// lodBound是计算Lod所用到的Bound 并不是严格意义上真正的Bound
	KAABBBox lodBound;
	uint32_t groupIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t generatingGroupIndex = KVirtualGeometryDefine::INVALID_INDEX;

	uint32_t index = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t level = 0;

	// lodError是计算Lod所用到的Error 并不是严格意义上真正的Error
	float lodError = 0;
	float edgeLength = 0;

	glm::vec3 color;

	KMeshCluster()
	{}

	static glm::vec3 RandomColor()
	{
		glm::vec3 color;
		color[0] = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
		color[1] = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
		color[2] = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
		return color;
	}

	void InitBound();
	void UnInit();
	void Init(KMeshClusterPtr* clusters, size_t numClusters);
	void Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<uint32_t>& inIndices);
	void Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<Triangle>& inTriangles, const std::vector<idx_t>& inTriIndices, const KRange& range);
};

struct KMeshClusterGroup
{
	std::vector<uint32_t> generatingClusters;
	std::vector<uint32_t> clusters;
	KAABBBox lodBound;
	glm::vec3 color;
	uint32_t level = 0;
	uint32_t index = 0;
	float lodError = 0;
	float edgeLength = 0;
};

typedef std::shared_ptr<KMeshClusterGroup> KMeshClusterGroupPtr;

struct KGraph
{
	idx_t offset = 0;
	idx_t num = 0;

	std::vector<idx_t> adjacency;
	std::vector<idx_t> adjacencyOffset;
	std::vector<idx_t> adjacencyCost;

	void Clear()
	{
		offset = 0;
		num = 0;
		adjacency.clear();
		adjacencyOffset.clear();
		adjacencyCost.clear();
	}
};

struct KGraphPartitioner
{
	std::vector<idx_t> partitionIDs;
	std::vector<idx_t> oldIndices;
	std::vector<idx_t> indices;
	std::vector<idx_t> mapto;
	std::vector<idx_t> mapback;
	std::vector<KRange> ranges;

	KRange NewRange(uint32_t offset, uint32_t num);
	void Start(idx_t num);
	void Finish();

	void PartitionBisect(KGraph* graph, size_t minPartition, size_t maxPartition);
	void PartitionStrict(KGraph* graph, size_t minPartition, size_t maxPartition);
	void PartitionRelex(KGraph* graph, size_t minPartition, size_t maxPartition);
};

class KMeshTriangleClusterBuilder
{
protected:
	typedef KMeshCluster::Triangle Triangle;

	uint32_t m_MinPartitionNum = 124;
	uint32_t m_MaxPartitionNum = 128;
	std::vector<KMeshClusterPtr> m_Clusters;

	struct Adjacency
	{
		std::vector<KMeshProcessorVertex> vertices;
		std::vector<Triangle> triangles;
		KGraph graph;
	};

	bool BuildTriangleAdjacencies(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, Adjacency& context);
	void Partition(Adjacency& context);
public:
	bool UnInit();
	bool Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, uint32_t minPartitionNum, uint32_t maxPartitionNum);

	inline void GetClusters(std::vector<KMeshClusterPtr>& clusters) const
	{
		clusters = m_Clusters;
	}
};

struct KMeshClustersPart
{
	std::vector<uint32_t> clusters;
	uint32_t groupIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t level = KVirtualGeometryDefine::INVALID_INDEX;
	KAABBBox lodBound;
	float lodError = 0;
};

typedef std::shared_ptr<KMeshClustersPart> KMeshClustersPartPtr;

struct KMeshClusterBatch
{
	glm::vec4 lodBoundCenterError;
	glm::vec4 lodBoundHalfExtendRadius;
	glm::vec4 parentBoundCenterError;
	glm::vec4 parentBoundHalfExtendRadius;
	uint32_t vertexFloatOffset = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t indexIntOffset = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t storageIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t triangleNum = 0;
};
static_assert((sizeof(KMeshClusterBatch) % 16) == 0, "Size must be a multiple of 16");
static_assert(sizeof(KMeshClusterBatch) == 16 * 4 + 4 * 4, "must match");

struct KMeshClustersVertexStorage
{
	enum
	{
		FLOAT_PER_VERTEX = 8
	};
	// Each vertex 8 float: pos.xyz:3 normal.xyz:3 uv:2
	std::vector<float> vertices;
};

struct KMeshClustersIndexStorage
{
	std::vector<uint32_t> indices;
};

struct KMeshClusterBVHNode
{
	uint32_t storagePartIndex = KVirtualGeometryDefine::INVALID_INDEX;
	std::vector<uint32_t> children;
	KAABBBox lodBound;
	float lodError = 0;
};

struct KMeshClusterHierarchy
{
	glm::vec4 lodBoundCenterError;
	glm::vec4 lodBoundHalfExtendRadius;
	uint32_t children[KVirtualGeometryDefine::MAX_BVH_NODES];
	uint32_t storagePartIndex = KVirtualGeometryDefine::INVALID_INDEX;
};

typedef std::shared_ptr<KMeshClusterBVHNode> KMeshClusterBVHNodePtr;

class KVirtualGeometryBuilder
{
protected:
	std::vector<KMeshClusterPtr> m_Clusters;
	std::vector<KMeshClusterGroupPtr> m_ClusterGroups;
	std::vector<KMeshClustersPartPtr> m_ClusterStorageParts;
	std::vector<KMeshClusterBVHNodePtr> m_BVHNodes;

	KAABBBox m_Bound;

	uint32_t m_MinClusterGroup = 8;
	uint32_t m_MaxClusterGroup = 32;
	uint32_t m_MinPartitionNum = 124;
	uint32_t m_MaxPartitionNum = 128;
	uint32_t m_LevelNum = 0;

	uint32_t m_MaxClusterVertex = 256;

	uint32_t m_MaxTriangleNum = 0;
	uint32_t m_MinTriangleNum = 0;
	float m_MaxError = 0;

	uint32_t m_BVHRoot = KVirtualGeometryDefine::INVALID_INDEX;

	bool m_CheckClusterAdacency = false;

	void DAGReduce(uint32_t childrenBegin, uint32_t childrenEnd, uint32_t level);
	void ClusterTriangle(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices);
	std::vector<std::unordered_map<size_t, size_t>> MaskClustersAdjacency(const std::vector<idx_t>& clusterIndices);
	std::vector<std::unordered_map<size_t, size_t>> MaskClustersAdjacency(idx_t begin, idx_t end);

	bool CheckClustersIndex();
	bool CheckClustersAdjacency(const std::vector<idx_t>& clusterIndices);
	bool CheckClustersAdjacency(idx_t begin, idx_t end);
	KGraph BuildClustersAdjacency(idx_t begin, idx_t end);

	enum SortAxis
	{
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		AXIS_NUM
	};

	float SortBVHNodesByAxis(const std::vector<KMeshClusterBVHNodePtr>& bvhNodes, SortAxis axis, const std::vector<uint32_t>& indices, const KRange& range, std::vector<uint32_t>& sorted);
	void SortBVHNodes(const std::vector<KMeshClusterBVHNodePtr>& bvhNodes, std::vector<uint32_t>& indices);
	uint32_t BuildHierarchyTopDown(std::vector<KMeshClusterBVHNodePtr>& bvhNodes, std::vector<uint32_t>& indices, bool sort);

	void BuildDAG(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, uint32_t minPartitionNum, uint32_t maxPartitionNum, uint32_t minClusterGroup, uint32_t maxClusterGroup);
	void BuildClusterStorage();
	void BuildClusterBVH();

	void RecurselyVisitBVH(uint32_t index, std::function<void(uint32_t index)> visitFunc);

	uint32_t BuildMeshClusterHierarchies(std::vector<KMeshClusterHierarchy>& hierarchies, uint32_t index);

	static bool ColorDebugClusters(const std::vector<KMeshClusterPtr>& clusters, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices);
	static bool ColorDebugClusterGroups(const std::vector<KMeshClusterPtr>& clusters, const std::vector<KMeshClusterGroupPtr>& groups, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices);
public:
	void FindDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<uint32_t>& clusterIndices, uint32_t& triangleCount, float& error) const;
	void ColorDebugDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, uint32_t& triangleCount, float& error) const;
	void ColorDebugCluster(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices) const;
	void ColorDebugClusterGroup(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices) const;
	void GetAllBVHBounds(std::vector<KAABBBox>& bounds);
	void DumpClusterGroupAsOBJ(const std::string& saveRoot) const;
	void DumpClusterAsOBJ(const std::string& saveRoot) const;
	void DumpClusterInformation(const std::string& saveRoot) const;

	void Build(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices);

	bool GetMeshClusterStorages(std::vector<KMeshClusterBatch>& clusters, KMeshClustersVertexStorage& vertexStroage, KMeshClustersIndexStorage& indexStorage, std::vector<uint32_t>& clustersPartNum);
	bool GetMeshClusterHierarchies(std::vector<KMeshClusterHierarchy>& hierarchies);

	inline uint32_t GetLevelNum() const
	{
		return m_LevelNum;
	}

	inline uint32_t GetMaxTriangleNum() const
	{
		return m_MaxTriangleNum;
	}

	inline uint32_t GetMinTriangleNum() const
	{
		return m_MinTriangleNum;
	}

	inline float GetMaxError() const
	{
		return m_MaxError;
	}

	inline const KAABBBox& GetBound() const
	{
		return m_Bound;
	}
};