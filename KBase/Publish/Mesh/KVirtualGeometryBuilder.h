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
	static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

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
	uint32_t groupIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t generatingGroupIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t index = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t level = 0;
	float error = 0;
	float maxParentError = 0;
	KAABBBox bound;
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
	std::vector<uint32_t> clusters;
	std::vector<uint32_t> childrenClusters;
	KAABBBox bound;
	glm::vec3 color;
	uint32_t level = 0;
	uint32_t index = 0;
	float maxError = 0;
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
	KAABBBox bound;
};

typedef std::shared_ptr<KMeshClustersPart> KMeshClustersPartPtr;

struct KMeshClusterBatch
{
	uint32_t vertexOffset = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t indexOffset = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t storageIndex = KVirtualGeometryDefine::INVALID_INDEX;
	glm::vec4 boundCenter;
	glm::vec4 boundHalfExtend;
};

struct KMeshClustersStorage
{
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<uint32_t> indices;
};

struct KMeshClusterBVHNode
{
	uint32_t partIndex = KVirtualGeometryDefine::INVALID_INDEX;
	std::vector<uint32_t> children;
	KAABBBox bound;
};

struct KMeshClusterHierarchy
{
	uint32_t children[KVirtualGeometryDefine::MAX_BVH_NODES];
	glm::vec4 boundCenter;
	glm::vec4 boundHalfExtend;
	uint32_t partIndex;
};

static_assert(sizeof(KMeshClusterHierarchy) == KVirtualGeometryDefine::MAX_BVH_NODES * 4 + 32 + 4, "always");

typedef std::shared_ptr<KMeshClusterBVHNode> KMeshClusterBVHNodePtr;

class KVirtualGeometryBuilder
{
protected:
	std::vector<KMeshClusterPtr> m_Clusters;
	std::vector<KMeshClusterGroupPtr> m_ClusterGroups;
	std::vector<KMeshClustersPartPtr> m_ClusterStorageParts;
	std::vector<KMeshClusterBVHNodePtr> m_BVHNodes;

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

	bool GetMeshClusterStorages(std::vector<KMeshClusterBatch>& clusters, std::vector<KMeshClustersStorage>& stroages);
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
};