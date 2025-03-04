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

	KRange() {}

	KRange(uint32_t inBegin, uint32_t inEnd)
	{
		begin = inBegin;
		end = inEnd;
	}
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

	static constexpr uint32_t MAX_CLUSTER_REUSE_BATCH = 32;

	static constexpr uint32_t MAX_CLUSTER_PART_IN_PAGE_BITS = 10;
	static constexpr uint32_t MAX_CLUSTER_PART_IN_PAGE = 1 << MAX_CLUSTER_PART_IN_PAGE_BITS;

	static constexpr uint32_t MAX_CLUSTER_IN_PAGE_BITS = 5 + MAX_CLUSTER_PART_IN_PAGE_BITS;
	static constexpr uint32_t MAX_CLUSTER_IN_PAGE = 1 << MAX_CLUSTER_IN_PAGE_BITS;

	static constexpr uint32_t MAX_ROOT_PAGE_SIZE = 10 * 1024;
	static constexpr uint32_t MAX_STREAMING_PAGE_SIZE = 10 * 1024;
};
static_assert(!(KVirtualGeometryDefine::MAX_BVH_NODES & (KVirtualGeometryDefine::MAX_BVH_NODES - 1)), "MAX_BVH_NODES must be pow of 2");

struct KMeshClusterMaterialRange
{
	uint32_t start = 0;
	uint32_t length = 0;
	uint32_t materialIndex = 0;
	std::vector<uint32_t> batchTriCounts;
};

struct KMeshCluster
{
	struct Triangle
	{
		int32_t index[3] = { -1, -1 ,-1 };
	};

	std::vector<KMeshProcessorVertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> materialIndices;

	std::vector<KMeshClusterMaterialRange> materialRanges;

	// 严格意义上真正的Bound
	KAABBBox bound;
	// lodBound是计算Lod所用到的Bound 并不是严格意义上真正的Bound
	KAABBBox lodBound;
	// 所处的Group所在的index
	uint32_t groupIndex = KVirtualGeometryDefine::INVALID_INDEX;
	// 生成的Group所在的index
	uint32_t generatingGroupIndex = KVirtualGeometryDefine::INVALID_INDEX;
	// 所在的GroupPart的index
	uint32_t partIndex = KVirtualGeometryDefine::INVALID_INDEX;
	// 所在的GroupPart的第几个cluster
	uint32_t offsetInPart = 0;
	// 全局cluster index
	uint32_t index = KVirtualGeometryDefine::INVALID_INDEX;
	// LOD level
	uint32_t level = 0;
	// Has Cone

	glm::vec3 color;
	glm::vec4 coneCenter;
	glm::vec4 coneDirection;

	// lodError是计算Lod所用到的Error 并不是严格意义上真正的Error
	float lodError = 0;
	float edgeLength = 0;

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

	void CopyProperty(const KMeshCluster& cluster);

	void InitBound();
	void InitMaterial();
	void PostInit();
	bool BuildMaterialRange();
	void EnsureIndexOrder();

	void UnInit();
	void Init(KMeshClusterPtr* clusters, uint32_t numClusters);
	void Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<uint32_t>& inIndices, const std::vector<uint32_t>& inMaterialIndices);
	void Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<uint32_t>& inIndices, const std::vector<uint32_t>& inMaterialIndices, const KRange& range);
	void Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<Triangle>& inTriangles, const std::vector<idx_t>& inTriIndices, const std::vector<uint32_t>& inMaterialIndices, const KRange& range);
};

struct KMeshClusterGroup
{
	// 此Group生成了哪些Cluster
	std::vector<uint32_t> generatingClusters;
	// 此Group由哪些Cluster生成
	std::vector<uint32_t> clusters;
	KAABBBox parentLodBound;
	KAABBBox bound;
	glm::vec3 color;
	uint32_t level = 0;
	uint32_t index = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t pageStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t pageEnd = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t partStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t partEnd = KVirtualGeometryDefine::INVALID_INDEX;
	float maxParentError = 0;
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
		std::vector<uint32_t> materialIndices;
		KGraph graph;
	};

	bool BuildTriangleAdjacencies(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, Adjacency& context);
	void Partition(Adjacency& context);
public:
	bool UnInit();
	bool Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices, uint32_t minPartitionNum, uint32_t maxPartitionNum);

	inline void GetClusters(std::vector<KMeshClusterPtr>& clusters) const
	{
		clusters = m_Clusters;
	}
};

struct KMeshClusterGroupPart
{
	std::vector<KMeshClusterPtr> clusters;
	uint32_t groupIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t level = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t pageIndex = KVirtualGeometryDefine::INVALID_INDEX;
	// Start offset in page
	uint32_t clusterStart = 0;
	uint32_t clusterNum = 0;
	// Index in hierarchy
	uint32_t hierarchyIndex = 0;
	uint32_t index = 0;
	KAABBBox lodBound;
	float lodError = 0;
};
typedef std::shared_ptr<KMeshClusterGroupPart> KMeshClusterGroupPartPtr;

struct KMeshClusterBatch
{
	uint32_t leaf = 0;
	uint32_t vertexFloatOffset = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t indexIntOffset = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t materialIntOffset = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t partIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t triangleNum = 0;
	uint32_t batchNum = 0;
	uint32_t padding = 0;
	glm::vec4 lodBoundCenterError;
	glm::vec4 lodBoundHalfExtendRadius;
	glm::vec4 parentBoundCenterError;
	glm::vec4 parentBoundHalfExtendRadius;
	glm::vec4 coneCenter;
	glm::vec4 coneDirection;
};
static_assert((sizeof(KMeshClusterBatch) % 16) == 0, "Size must be a multiple of 16");
static_assert(sizeof(KMeshClusterBatch) == 16 * 6 + 8 * 4, "must match");

struct KMeshClustersVertexStorage
{
	// Each vertex 8 float: pos.xyz:3 normal.xyz:3 uv:2
	std::vector<float> vertices;
};

struct KMeshClustersIndexStorage
{
	std::vector<uint32_t> indices;
};

struct KMeshClustersMaterialStorage
{
	// Each material 3 int: mateialIndex, rangeBegin, rangeEnd
	std::vector<uint32_t> materials;
};

struct KMeshClustersMaterialStorages
{
	std::vector<uint32_t> storages;
};

struct KMeshClusterBatchStorage
{
	std::vector<KMeshClusterBatch> batches;
};

struct KVirtualGeometryEncoding
{
	enum
	{
		FLOAT_PER_VERTEX = 8,
		BYTE_SIZE_PER_VERTEX = 4 * FLOAT_PER_VERTEX,
		BYTE_SIZE_PER_INDEX = 4,
		INT_PER_MATERIAL = 3,
		BYTE_PER_MATERIAL = INT_PER_MATERIAL * 4,
		BYTE_PER_CLUSTER_BATCH = sizeof(KMeshClusterBatch)
	};
};

struct KVirtualGeometryPageStorage
{
	uint32_t vertexStorageByteOffset = 0;
	uint32_t indexStorageByteOffset = 0;
	uint32_t materialStorageByteOffset = 0;
	uint32_t batchStorageByteOffset = 0;
	KMeshClustersVertexStorage vertexStorage;
	KMeshClustersIndexStorage indexStorage;
	KMeshClustersMaterialStorage materialStorage;
	KMeshClusterBatchStorage batchStorage;
};

struct KVirtualGeometryPageStorages
{
	std::vector<KVirtualGeometryPageStorage> storages;
};

struct KVirtualGeomertyPageDependency
{
	std::vector<uint32_t> dependencies;
};

struct KVirtualGeomertyPageDependencies
{
	std::vector<KVirtualGeomertyPageDependency> pageDependencies;
};

struct KVirtualGeomertyPageCluster
{
	uint32_t partIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t offsetInPart = 0;
};

struct KVirtualGeomertyPageClusterGroup
{
	uint32_t groupPartStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t groupPartEnd = KVirtualGeometryDefine::INVALID_INDEX;
};

struct KVirtualGeomertyPageClusterGroupPart
{
	uint32_t hierarchyIndex = 0;
	uint32_t level = 0;
};

struct KVirtualGeomertyPageClustersData
{
	std::vector<KVirtualGeomertyPageCluster> clusters;
	std::vector<KVirtualGeomertyPageClusterGroup> groups;
	std::vector<KVirtualGeomertyPageClusterGroupPart> parts;
};

struct KMeshClusterBVHNode
{
	uint32_t partIndex = KVirtualGeometryDefine::INVALID_INDEX;
	std::vector<uint32_t> children;
	KAABBBox lodBound;
	float lodError = 0;
};
typedef std::shared_ptr<KMeshClusterBVHNode> KMeshClusterBVHNodePtr;

struct KMeshClusterHierarchy
{
	glm::vec4 lodBoundCenterError;
	glm::vec4 lodBoundHalfExtendRadius;
	uint32_t children[KVirtualGeometryDefine::MAX_BVH_NODES];
	uint32_t partIndex = KVirtualGeometryDefine::INVALID_INDEX;
};

struct KVirtualGeometryPage
{
	uint32_t clusterGroupPartStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t clusterGroupPartNum = 0;
	uint32_t clusterNum = 0;
	uint32_t dataByteSize = 0;
	bool isRootPage = false;
};

struct KVirtualGeometryPages
{
	std::vector<KVirtualGeometryPage> pages;
	uint32_t numRootPage = 0;
};

struct KVirtualGeomertyClusterFixup
{
	uint32_t fixupPage = KVirtualGeometryDefine::INVALID_INDEX;
	// Local page cluster index
	uint32_t clusterIndexInPage = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t dependencyPageStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t dependencyPageEnd = 0;
};

struct KVirtualGeomertyHierarchyFixup
{
	uint32_t fixupPage = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t partIndex = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t dependencyPageStart = KVirtualGeometryDefine::INVALID_INDEX;
	uint32_t dependencyPageEnd = 0;
};

struct KVirtualGeomertyFixup
{
	std::vector<std::vector<KVirtualGeomertyClusterFixup>> clusterFixups;
	std::vector<std::vector<KVirtualGeomertyHierarchyFixup>> hierarchyFixups;
};

class KVirtualGeometryBuilder
{
protected:
	std::vector<KMeshClusterPtr> m_Clusters;
	std::vector<KMeshClusterGroupPtr> m_ClusterGroups;
	std::vector<KMeshClusterGroupPartPtr> m_ClusterGroupParts;
	std::vector<KMeshClusterBVHNodePtr> m_BVHNodes;
	std::vector<KMeshClusterHierarchy> m_Hierarchies;

	KVirtualGeometryPages m_Pages;
	KVirtualGeometryPageStorages m_PageStorages;
	KVirtualGeomertyFixup m_PageFixup;
	KVirtualGeomertyPageDependencies m_PageDependencies;
	KVirtualGeomertyPageClustersData m_PageClusters;

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
	void ClusterTriangle(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices);
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
	uint32_t BuildMeshClusterHierarchies(uint32_t index);
	void BuildPageDependency(uint32_t pageIndex, KVirtualGeomertyPageDependency& dependency);

	void BuildDAG(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices, uint32_t minPartitionNum, uint32_t maxPartitionNum, uint32_t minClusterGroup, uint32_t maxClusterGroup);
	void BuildMaterialRanges();
	void SortClusterGroup();
	void ConstrainCluster();
	void BuildReuseBatch();
	void BuildPage();
	void BuildPageStorage();
	void BuildClusterBVH();
	void BuildFixup();
	void BuildStreaming();

	void RecurselyVisitBVH(uint32_t index, std::function<void(uint32_t index)> visitFunc);
	
	static bool ColorDebugClusters(const std::vector<KMeshClusterPtr>& clusters, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices);
	static bool ColorDebugClusterGroups(const std::vector<KMeshClusterPtr>& clusters, const std::vector<KMeshClusterGroupPtr>& groups, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices);
public:
	void FindDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<uint32_t>& clusterIndices, uint32_t& triangleCount, float& error) const;
	void ColorDebugDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices, uint32_t& triangleCount, float& error) const;
	void ColorDebugCluster(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices) const;
	void ColorDebugClusterGroup(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices) const;
	void GetAllBVHBounds(std::vector<KAABBBox>& bounds);
	void DumpClusterGroupAsOBJ(const std::string& saveRoot) const;
	void DumpClusterAsOBJ(const std::string& saveRoot) const;
	void DumpClusterInformation(const std::string& saveRoot) const;

	void Build(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices);

	bool GetMeshClusterStorages(KMeshClusterBatchStorage& batchStorage, KMeshClustersVertexStorage& vertexStroage, KMeshClustersIndexStorage& indexStorage, KMeshClustersMaterialStorage& materialStorage) const;
	bool GetMeshClusterHierarchies(std::vector<KMeshClusterHierarchy>& hierarchies) const;
	bool GetMeshClusterGroupParts(std::vector<KMeshClusterPtr>& clusters, std::vector<KMeshClusterGroupPartPtr>& parts, std::vector<KMeshClusterGroupPtr>& groups) const;
	bool GetPages(KVirtualGeometryPages& pages, KVirtualGeometryPageStorages& pageStorages, KVirtualGeomertyFixup& pageFixup, KVirtualGeomertyPageDependencies &pageDependencies, KVirtualGeomertyPageClustersData& pageClusters) const;

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