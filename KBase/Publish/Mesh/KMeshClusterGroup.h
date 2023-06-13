#pragma once
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include "KBase/Publish/Mesh/KMeshSimplification.h"
#include <unordered_set>
#include <metis.h>

struct KRange
{
	uint32_t begin = 0;
	uint32_t end = 0;
};

struct KMeshCluster;
typedef std::shared_ptr<KMeshCluster> KMeshClusterPtr;

struct KMeshCluster
{
	struct Triangle
	{
		int32_t index[3] = { -1, -1 ,-1 };
	};

	constexpr static uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

	std::vector<KMeshProcessorVertex> vertices;
	std::vector<uint32_t> indices;
	uint32_t generatingGroupIndex = INVALID_INDEX;
	uint32_t level = 0;
	float error = 0;
	bool root = true;
	glm::vec3 color;

	KMeshCluster()
	{}

	void AssignRandomColor()
	{
		float random = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
		color = glm::vec3(0, 0, 0);
		color[rand() % 3] = powf(random, 0.5f);
	}

	void UnInit()
	{
		vertices.clear();
		indices.clear();
	}

	void Init(KMeshClusterPtr* clusters, size_t numClusters)
	{
		UnInit();
		AssignRandomColor();

		uint32_t sumVertexNum = 0;
		uint32_t sumIndexNum = 0;

		for (size_t i = 0; i < numClusters; ++i)
		{
			sumVertexNum += (uint32_t)clusters[i]->vertices.size();
			sumIndexNum += (uint32_t)clusters[i]->indices.size();
		}

		vertices.reserve(sumVertexNum);
		indices.reserve(sumIndexNum);

		std::unordered_map<size_t, uint32_t> vertexMap;

		for (size_t i = 0; i < numClusters; ++i)
		{
			KMeshCluster& cluster = *clusters[i];
			for (uint32_t index : cluster.indices)
			{
				const KMeshProcessorVertex& vertex = cluster.vertices[index];
				size_t hash = KMeshProcessorVertexHash(vertex);
				uint32_t newIndex = 0;
				auto it = vertexMap.find(hash);
				if (it == vertexMap.end())
				{
					newIndex = (uint32_t)vertices.size();
					vertices.push_back(vertex);
					vertexMap.insert({ hash, newIndex });
				}
				else
				{
					newIndex = it->second;
				}
				indices.push_back(newIndex);
			}
		}

		vertices.shrink_to_fit();
		indices.shrink_to_fit();
	}

	void Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<uint32_t>& inIndices)
	{
		UnInit();
		AssignRandomColor();
		vertices = inVertices;
		indices = inIndices;
	}

	void Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<Triangle>& inTriangles, const std::vector<idx_t>& inTriIndices, const KRange& range)
	{
		UnInit();
		AssignRandomColor();

		size_t num = range.end - range.begin + 1;

		vertices.reserve(num);
		indices.reserve(num * 3);

		std::unordered_map<uint32_t, uint32_t> indexMap;

		size_t begin = range.begin;
		size_t end = range.end;
		for (size_t idx = begin; idx <= end; ++idx)
		{
			uint32_t triIndex = inTriIndices[idx];
			for (uint32_t i = 0; i < 3; ++i)
			{
				uint32_t index = inTriangles[triIndex].index[i];
				uint32_t newIndex = 0;
				auto it = indexMap.find(index);
				if (it == indexMap.end())
				{
					newIndex = (uint32_t)vertices.size();
					indexMap.insert({ index, newIndex });
					vertices.push_back(inVertices[index]);
				}
				else
				{
					newIndex = it->second;
				}
				indices.push_back(newIndex);
			}
		}

		vertices.shrink_to_fit();
		indices.shrink_to_fit();
	}
};

struct KMeshClusterGroup
{
	std::vector<uint32_t> clusters;
	std::vector<uint32_t> childrenClusters;
	KAABBBox bound;
	uint32_t level = 0;
	uint32_t index = 0;
	float maxError = 0;
};

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

	KRange NewRange(uint32_t offset, uint32_t num)
	{
		KRange range;
		range.begin = offset;
		range.end = range.begin + num - 1;
		return range;
	}

	void Start(idx_t num)
	{
		oldIndices.resize(num);
		indices.resize(num);
		partitionIDs.resize(num);
		for (idx_t i = 0; i < num; ++i)
		{
			indices[i] = i;
		}
		mapto.resize(num);
		mapback.resize(num);
		ranges.clear();
	}

	void Finish()
	{
		oldIndices.clear();
		partitionIDs.clear();
		mapto.clear();
		mapback.clear();
	}

	void PartitionBisect(KGraph* graph, size_t minPartition, size_t maxPartition)
	{
		idx_t expectedPartition = idx_t(minPartition + maxPartition) / 2;
		idx_t expectedNumParts = std::max(2, (graph->num + (idx_t)expectedPartition - 1) / (idx_t)expectedPartition);

		real_t partitionWeights[2] = { 0.5f, 0.5f };
		partitionWeights[0] = (expectedNumParts >> 1) / (real_t)expectedNumParts;
		partitionWeights[1] = 1 - partitionWeights[0];

		idx_t options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(options);

		bool bLoose = expectedNumParts >= 128 || maxPartition / minPartition > 1;
		options[METIS_OPTION_UFACTOR] = bLoose ? 200 : 1;

		idx_t numConstraints = 1;
		idx_t numParts = 2;
		idx_t edgesCut = 0;

		int r = METIS_PartGraphRecursive(
			&graph->num,
			&numConstraints,				// number of balancing constraints
			graph->adjacencyOffset.data(),
			graph->adjacency.data(),
			NULL,							// Vert weights
			NULL,							// Vert sizes for computing the total communication volume
			graph->adjacencyCost.data(),	// Edge weights
			&numParts,
			partitionWeights,				// Target partition weight
			NULL,							// Allowed load imbalance tolerance
			options,
			&edgesCut,
			partitionIDs.data() + graph->offset
		);

		idx_t start = (idx_t)graph->offset;
		idx_t end = (idx_t)(graph->offset + graph->num - 1);

		idx_t partNum[2] = { 0, 0 };
		for (idx_t i = start; i <= end; ++i)
		{
			oldIndices[i] = indices[i];
			++partNum[partitionIDs[i]];
		}

		idx_t partIndexer[2] = { start, start + partNum[0] };
		for (idx_t i = start; i <= end; ++i)
		{
			mapto[i] = partIndexer[partitionIDs[i]]++;
			indices[mapto[i]] = oldIndices[i];
			mapback[mapto[i]] = i;
		}

		idx_t split = start + partNum[0];

		/*
		while (start <= end)
		{
			while (partitionIDs[start] == 0)
			{
				mapback[start] = mapto[start] = start;
				++start;
			}

			while (partitionIDs[end] == 1)
			{
				mapback[end] = mapto[end] = end;
				--end;
			}

			if (start < end)
			{
				std::swap(indices[start], indices[end]);
				mapback[start] = mapto[start] = end;
				mapback[end] = mapto[end] = start;
				++start;
				--end;
			}
		}
		idx_t split = start;
		*/

		KGraph partition0, partition1;
		partition0.offset = graph->offset;
		partition0.num = split - partition0.offset;
		partition1.offset = split;
		partition1.num = graph->num - partition0.num;

		if (partition0.num <= maxPartition && partition1.num <= maxPartition)
		{
			ranges.push_back(NewRange(partition0.offset, partition0.num));
			ranges.push_back(NewRange(partition1.offset, partition1.num));
		}
		else
		{
			partition0.adjacency.reserve(graph->adjacency.size() >> 1);
			partition0.adjacencyCost.reserve(graph->adjacencyCost.size() >> 1);
			partition0.adjacencyOffset.reserve(graph->adjacencyOffset.size() >> 1);

			partition1.adjacency.reserve(graph->adjacency.size() >> 1);
			partition1.adjacencyCost.reserve(graph->adjacencyCost.size() >> 1);
			partition1.adjacencyOffset.reserve(graph->adjacencyOffset.size() >> 1);

			// for each new
			for (size_t i = 0; i < graph->num; ++i)
			{
				KGraph* childPartition = (i >= partition0.num) ? &partition1 : &partition0;
				childPartition->adjacencyOffset.push_back((idx_t)childPartition->adjacency.size());
				// map to old
				idx_t orgLocalIdx = mapback[graph->offset + i] - graph->offset;
				for (idx_t k = graph->adjacencyOffset[orgLocalIdx]; k < graph->adjacencyOffset[orgLocalIdx + 1]; ++k)
				{
					idx_t adj = graph->adjacency[k];
					idx_t adjCost = graph->adjacencyCost[k];
					// map to new
					idx_t adjIndex = mapto[graph->offset + adj];
					if (adjIndex >= childPartition->offset && adjIndex < (childPartition->offset + childPartition->num))
					{
						idx_t adjChildIndex = adjIndex - childPartition->offset;
						childPartition->adjacency.push_back(adjChildIndex);
						childPartition->adjacencyCost.push_back(adjCost);
					}
				}
			}

			partition0.adjacencyOffset.push_back((idx_t)partition0.adjacency.size());
			partition1.adjacencyOffset.push_back((idx_t)partition1.adjacency.size());

			assert(partition0.adjacencyOffset.size() + partition1.adjacencyOffset.size() - 1 == graph->adjacencyOffset.size());

			PartitionBisect(&partition0, minPartition, maxPartition);
			PartitionBisect(&partition1, minPartition, maxPartition);
		}
	}

	void PartitionStrict(KGraph* graph, size_t minPartition, size_t maxPartition)
	{
		Start(graph->num);		
		if (graph->num < maxPartition)
		{
			ranges.push_back(NewRange(0, graph->num));
		}
		else
		{
			PartitionBisect(graph, minPartition, maxPartition);
		}
		Finish();
	}

	void PartitionRelex(KGraph* graph, size_t minPartition, size_t maxPartition)
	{
		Start(graph->num);

		idx_t expectedPartition = idx_t(minPartition + maxPartition) / 2;
		idx_t numPartition = (idx_t)((graph->num + (expectedPartition - 1)) / expectedPartition);

		if (numPartition > 1)
		{
			idx_t numConstraints = 1;
			idx_t edgesCut = 0;

			idx_t options[METIS_NOPTIONS];
			METIS_SetDefaultOptions(options);
			options[METIS_OPTION_UFACTOR] = 200;

			int r = METIS_PartGraphKway(
				&graph->num,
				&numConstraints,				// number of balancing constraints
				graph->adjacencyOffset.data(),
				graph->adjacency.data(),
				NULL,							// Vert weights
				NULL,							// Vert sizes for computing the total communication volume
				graph->adjacencyCost.data(),	// Edge weights
				&numPartition,
				NULL,							// Target partition weight
				NULL,							// Allowed load imbalance tolerance
				options,
				&edgesCut,
				partitionIDs.data());

			assert(r == METIS_OK);
			if (r == METIS_OK)
			{
				std::vector<uint32_t> partitionNum;
				partitionNum.resize(numPartition);

				for (idx_t i = 0; i < graph->num; ++i)
				{
					++partitionNum[partitionIDs[i]];
				}

				std::vector<uint32_t> partitionBegin;
				partitionBegin.resize(numPartition);

				uint32_t begin = 0;
				for (idx_t i = 0; i < numPartition; ++i)
				{
					partitionBegin[i] = begin;
					begin += partitionNum[i];
				}

				std::vector<uint32_t> partitionIndexer = partitionBegin;

				for (idx_t i = 0; i < graph->num; ++i)
				{
					idx_t partitionId = partitionIDs[i];
					indices[partitionIndexer[partitionId]++] = i;
				}

				for (idx_t i = 0; i < numPartition; ++i)
				{
					uint32_t begin = partitionBegin[i];
					uint32_t end = (i < numPartition - 1) ? partitionBegin[i + 1] : graph->num;
					uint32_t num = end - begin;
					ranges.push_back(NewRange(begin, num));
				}
			}
		}
		else
		{
			ranges.push_back(NewRange(0, graph->num));
		}

		Finish();
	}
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

	bool BuildTriangleAdjacencies(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, Adjacency& context)
	{
		if (indices.size() > 0 && indices.size() % 3 == 0 && vertices.size() > 0)
		{
			uint32_t numVertices = (uint32_t)vertices.size();
			uint32_t numTriangles = (uint32_t)indices.size() / 3;

			KEdgeHash edgeHash;
			edgeHash.Init(numVertices);

			context.triangles.resize(numTriangles);

			for (uint32_t triIndex = 0; triIndex < numTriangles; ++triIndex)
			{
				uint32_t v0 = indices[3 * triIndex + 0];
				uint32_t v1 = indices[3 * triIndex + 1];
				uint32_t v2 = indices[3 * triIndex + 2];

				assert(v0 < numVertices);
				assert(v1 < numVertices);
				assert(v2 < numVertices);

				Triangle newTriangle = { {(int32_t)v0, (int32_t)v1, (int32_t)v2} };
				context.triangles[triIndex] = newTriangle;

				edgeHash.AddEdgeHash(v0, v1, triIndex);
				edgeHash.AddEdgeHash(v1, v2, triIndex);
				edgeHash.AddEdgeHash(v2, v0, triIndex);
			}

			context.graph.adjacencyOffset.clear();
			context.graph.adjacencyOffset.reserve(numTriangles);

			context.graph.adjacency.clear();
			context.graph.adjacency.reserve(numTriangles * 3);

			context.graph.adjacencyCost.clear();
			context.graph.adjacencyCost.reserve(numTriangles * 3);

			for (uint32_t triIndex = 0; triIndex < numTriangles; ++triIndex)
			{
				std::vector<idx_t> triangleAdjacencies;
				triangleAdjacencies.reserve(3);
				for (uint32_t i = 0; i < 3; ++i)
				{
					uint32_t v0 = context.triangles[triIndex].index[i];
					uint32_t v1 = context.triangles[triIndex].index[(i + 1) % 3];
					edgeHash.ForEachTri(v1, v0, [triIndex, &triangleAdjacencies](int32_t adjTriIndex)
					{
						if (triIndex != adjTriIndex)
						{
							triangleAdjacencies.push_back(adjTriIndex);
						}
					});
				}

				context.graph.adjacencyOffset.push_back((idx_t)context.graph.adjacency.size());
				context.graph.adjacency.insert(context.graph.adjacency.end(), triangleAdjacencies.begin(), triangleAdjacencies.end());
				context.graph.adjacencyCost.insert(context.graph.adjacencyCost.end(), triangleAdjacencies.size(), 1);
			}

			context.graph.adjacencyOffset.push_back((idx_t)context.graph.adjacency.size());

			context.graph.offset = 0;
			context.graph.num = (idx_t)numTriangles;

			context.vertices = vertices;

			return true;
		}
		return false;
	}

	void Partition(Adjacency& context)
	{
		KGraphPartitioner partitioner;
		partitioner.PartitionStrict(&context.graph, m_MinPartitionNum, m_MaxPartitionNum);

		m_Clusters.clear();
		m_Clusters.reserve(partitioner.ranges.size());

		for (const KRange& range : partitioner.ranges)
		{
			KMeshClusterPtr cluster = KMeshClusterPtr(new KMeshCluster());
			cluster->Init(context.vertices, context.triangles, partitioner.indices, range);
			m_Clusters.push_back(std::move(cluster));
		}
	}
public:
	bool UnInit()
	{
		m_Clusters.clear();
		return true;
	}

	bool Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, uint32_t minPartitionNum, uint32_t maxPartitionNum)
	{
		UnInit();
		m_MinPartitionNum = minPartitionNum;
		m_MaxPartitionNum = maxPartitionNum;
		Adjacency adjacency;
		if (BuildTriangleAdjacencies(vertices, indices, adjacency))
		{
			Partition(adjacency);
			return true;
		}
		return false;
	}

	void GetClusters(std::vector<KMeshClusterPtr>& clusters)
	{
		clusters = m_Clusters;
	}

	static bool ColorDebugCluster(const std::vector<KMeshClusterPtr>& clusters, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
	{
		vertices.clear();
		indices.clear();

		uint32_t clusterIndexBegin = 0;

		for (uint32_t id : ids)
		{
			const KMeshCluster& cluster = *clusters[id];
			std::vector<KMeshProcessorVertex> clusterVertices = cluster.vertices;
			std::vector<uint32_t> clusterIndices = cluster.indices;
			glm::vec3 clusterColor = cluster.color;

			for (uint32_t& index : clusterIndices)
			{
				index += clusterIndexBegin;
			}
			for (KMeshProcessorVertex& vertex : clusterVertices)
			{
				vertex.color[0] = clusterColor;
			}

			vertices.insert(vertices.end(), clusterVertices.begin(), clusterVertices.end());
			indices.insert(indices.end(), clusterIndices.begin(), clusterIndices.end());

			clusterIndexBegin += (uint32_t)clusterVertices.size();
		}

		return true;
	}
};

class KVirtualGeometryBuilder
{
protected:
	std::vector<KMeshClusterPtr> m_Clusters;
	std::vector<KMeshClusterGroup> m_ClusterGroups;

	uint32_t m_MinClusterGroup = 8;
	uint32_t m_MaxClusterGroup = 32;
	uint32_t m_MinPartitionNum = 124;
	uint32_t m_MaxPartitionNum = 128;
	uint32_t m_LevelNum = 0;

	uint32_t m_MaxTriangleNum = 0;
	uint32_t m_MinTriangleNum = 0;
	float m_MaxError = 0;

	void DAGReduce(uint32_t childrenBegin, uint32_t childrenEnd, uint32_t level)
	{
		uint32_t numChildren = childrenEnd - childrenBegin + 1;
		assert(childrenBegin < m_Clusters.size());
		assert(childrenEnd < m_Clusters.size());

		KMeshClusterPtr mergedCluster = KMeshClusterPtr(new KMeshCluster());
		mergedCluster->Init(m_Clusters.data() + childrenBegin, numChildren);

		uint32_t numParent = KMath::DivideAndRoundUp((uint32_t)mergedCluster->indices.size(), (uint32_t)(6 * m_MaxPartitionNum));
		m_Clusters.reserve(m_Clusters.size() + numParent);

		uint32_t minTargetTriangleNum = m_MaxPartitionNum * numParent / 2;
		KMeshSimplification simplification;
		simplification.Init(mergedCluster->vertices, mergedCluster->indices, 3, minTargetTriangleNum);

		std::vector<KMeshProcessorVertex> vertices;
		std::vector<uint32_t> indices;

		float error = 0;
		uint32_t parentBegin = (uint32_t)m_Clusters.size();

		for (uint32_t partitionNum = m_MaxPartitionNum - 2; partitionNum >= m_MaxPartitionNum / 2; partitionNum -= 2)
		{
			uint32_t targetTriangleNum = partitionNum * numParent;
			if (!simplification.Simplify(MeshSimplifyTarget::TRIANGLE, targetTriangleNum, vertices, indices, error))
			{
				continue;
			}

			if (numParent == 1)
			{
				mergedCluster->Init(vertices, indices);
				m_Clusters.push_back(mergedCluster);
				break;
			}

			KMeshTriangleClusterBuilder builder;
			if (!builder.Init(vertices, indices, m_MinPartitionNum, m_MaxPartitionNum))
			{
				continue;
			}

			std::vector<KMeshClusterPtr> parentClusters;
			builder.GetClusters(parentClusters);

			if (parentClusters.size() <= numParent)
			{
				m_Clusters.insert(m_Clusters.end(), parentClusters.begin(), parentClusters.end());
				break;
			}
		}

		uint32_t parentEnd = (uint32_t)m_Clusters.size() - 1;
		if (parentBegin <= parentEnd)
		{
			numParent = parentEnd - parentBegin + 1;

			KMeshClusterGroup newGroup;
			newGroup.level = level + 1;

			newGroup.childrenClusters.resize(numChildren);
			for (uint32_t i = 0; i < numChildren; ++i)
			{
				newGroup.childrenClusters[i] = childrenBegin + i;
				m_Clusters[childrenBegin + i]->root = false;
				error = std::max(error, m_Clusters[childrenBegin + i]->error);
			}

			newGroup.maxError = error;
			newGroup.index = (uint32_t)m_ClusterGroups.size();

			newGroup.clusters.resize(numParent);
			for (uint32_t i = 0; i < numParent; ++i)
			{
				newGroup.clusters[i] = parentBegin + i;
				m_Clusters[parentBegin + i]->error = error;
				m_Clusters[parentBegin + i]->level = newGroup.level;
				m_Clusters[parentBegin + i]->generatingGroupIndex = newGroup.index;
			}

			m_ClusterGroups.push_back(newGroup);
		}
	}

	void ClusterTriangle(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		KMeshTriangleClusterBuilder builder;
		builder.Init(vertices, indices, m_MinPartitionNum, m_MaxPartitionNum);
		builder.GetClusters(m_Clusters);
	}

	KGraph BuildClustersAdjacency(size_t begin, size_t end)
	{
		std::unordered_map<size_t, size_t> vertNewIdxMap;

		size_t verticeHashSize = 0;
		for (size_t idx = begin; idx <= end; ++idx)
		{
			verticeHashSize += m_Clusters[idx]->indices.size();
		}

		std::vector<size_t> verticeHash;
		verticeHash.resize(verticeHashSize);

		size_t verticeHashIdxBegin = 0;

		for (size_t idx = begin; idx <= end; ++idx)
		{
			const KMeshCluster& cluster = *m_Clusters[idx];
			size_t clusterID = idx - begin;
			for (size_t i = 0; i < cluster.indices.size(); ++i)
			{
				uint32_t vertIdx = cluster.indices[i];
				const KMeshProcessorVertex& vertex = cluster.vertices[vertIdx];
				size_t hash = KMeshProcessorVertexHash(vertex);
				if (vertNewIdxMap.find(hash) == vertNewIdxMap.end())
				{
					uint32_t newVertIdx = (uint32_t)vertNewIdxMap.size();
					vertNewIdxMap.insert({ hash, newVertIdx });
				}
				verticeHash[verticeHashIdxBegin + i] = hash;
			}
			verticeHashIdxBegin += cluster.indices.size();
		}

		size_t verticesNum = vertNewIdxMap.size();

		KEdgeHash edgeHash;
		edgeHash.Init(verticesNum);

		verticeHashIdxBegin = 0;

		for (size_t idx = begin; idx <= end; ++idx)
		{
			const KMeshCluster& cluster = *m_Clusters[idx];
			size_t clusterID = idx - begin;
			for (size_t i = 0; i < cluster.indices.size(); ++i)
			{
				size_t v0Hash = verticeHash[verticeHashIdxBegin + i];
				size_t v1Hash = verticeHash[verticeHashIdxBegin + 3 * (i / 3) + (i + 1) % 3];
				int32_t newV0 = (int32_t)vertNewIdxMap[v0Hash];
				int32_t newV1 = (int32_t)vertNewIdxMap[v1Hash];
				edgeHash.AddEdgeHash((int32_t)newV0, (int32_t)newV1, (int32_t)clusterID);
			}
			verticeHashIdxBegin += cluster.indices.size();
		}

		size_t clusterNum = end - begin + 1;

		std::vector<std::unordered_map<size_t, size_t>> clusterAdj;
		clusterAdj.resize(clusterNum);

		for (int32_t v0 = 0; v0 < (int32_t)verticesNum; ++v0)
		{
			edgeHash.ForEach(v0, [v0, &clusterAdj, &edgeHash](int32_t v1, int32_t clusterID)
			{
				edgeHash.ForEachTri(v1, v0, [&clusterAdj, clusterID](int32_t anotherClusterID)
				{
					if (clusterID != anotherClusterID)
					{
						auto it = clusterAdj[clusterID].find(anotherClusterID);
						if (it == clusterAdj[clusterID].end())
						{
							clusterAdj[clusterID].insert({ anotherClusterID, 1 });
						}
						else
						{
							++it->second;
						}
					}
				});
			});
		}

		KGraph graph;
		graph.offset = 0;
		graph.num = (uint32_t)clusterNum;

		graph.adjacencyOffset.reserve(clusterNum + 1);
		graph.adjacency.reserve(clusterNum * 3);
		graph.adjacencyCost.reserve(clusterNum * 3);

		for (size_t clusterID = 0; clusterID < clusterNum; ++clusterID)
		{
			graph.adjacencyOffset.push_back((idx_t)graph.adjacency.size());
			for (auto& pair : clusterAdj[clusterID])
			{
				size_t anotherClusterID = pair.first;
				size_t cost = pair.second;
				graph.adjacency.push_back((idx_t)anotherClusterID);
				graph.adjacencyCost.push_back((idx_t)cost);
			}
		}
		graph.adjacencyOffset.push_back((idx_t)graph.adjacency.size());

		return graph;
	}
public:
	bool Build(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, uint32_t minPartitionNum, uint32_t maxPartitionNum)
	{
		m_MinPartitionNum = minPartitionNum;
		m_MaxPartitionNum = maxPartitionNum;
		m_LevelNum = 0;

		ClusterTriangle(vertices, indices);

		uint32_t levelClusterBegin = 0;
		uint32_t levelClusterEnd = 0;
		uint32_t levelClusterNum = (uint32_t)m_Clusters.size();
		uint32_t newLevelBegin = 0;
		uint32_t currentLevel = 0;

		auto AddBaseGroup = [this](uint32_t begin, uint32_t end)
		{
			KMeshClusterGroup newGroup;
			uint32_t num = end - begin + 1;
			newGroup.level = 0;
			newGroup.clusters.resize(num);
			for (uint32_t i = 0; i < num; ++i)
			{
				newGroup.clusters[i] = begin + i;
			}
			newGroup.index = (uint32_t)m_ClusterGroups.size();
			m_ClusterGroups.push_back(newGroup);
		};

		while (levelClusterNum)
		{
			levelClusterEnd = levelClusterBegin + levelClusterNum - 1;
			if (levelClusterNum <= 1)
			{
				if (currentLevel == 0)
				{
					AddBaseGroup(levelClusterBegin, levelClusterEnd);
				}
				++currentLevel;
				break;
			}

			if (levelClusterNum <= m_MaxClusterGroup)
			{
				if (currentLevel == 0)
				{
					AddBaseGroup(levelClusterBegin, levelClusterEnd);
				}
				newLevelBegin = (uint32_t)m_Clusters.size();
				DAGReduce(levelClusterBegin, levelClusterEnd, currentLevel);
				levelClusterBegin = newLevelBegin;
				levelClusterNum = (uint32_t)(m_Clusters.size() - levelClusterBegin);
				++currentLevel;
				continue;
			}

			KGraph clusterGraph = BuildClustersAdjacency(levelClusterBegin, levelClusterEnd);

			KGraphPartitioner partitioner;
			partitioner.PartitionStrict(&clusterGraph, m_MinClusterGroup, m_MaxClusterGroup);

			{
				std::vector<KMeshClusterPtr> newOrderCluster;
				newOrderCluster.resize(levelClusterNum);
				for (idx_t index : partitioner.indices)
				{
					newOrderCluster[index] = m_Clusters[index + levelClusterBegin];
				}
				for (size_t i = 0; i < levelClusterNum; ++i)
				{
					m_Clusters[i + levelClusterBegin] = std::move(newOrderCluster[i]);
				}
			}

			if (currentLevel == 0)
			{
				for (const KRange& range : partitioner.ranges)
				{
					uint32_t clusterBegin = levelClusterBegin + range.begin;
					uint32_t clusterEnd = levelClusterBegin + range.end;
					AddBaseGroup(clusterBegin, clusterEnd);
				}
			}

			newLevelBegin = (uint32_t)m_Clusters.size();
			for (const KRange& range : partitioner.ranges)
			{
				uint32_t clusterBegin = levelClusterBegin + range.begin;
				uint32_t clusterEnd = levelClusterBegin + range.end;
				DAGReduce(clusterBegin, clusterEnd, currentLevel);
			}
			levelClusterBegin = newLevelBegin;
			levelClusterNum = (uint32_t)m_Clusters.size() - levelClusterBegin;

			++currentLevel;
		}

		m_LevelNum = currentLevel;
		
		m_MinTriangleNum = 0;
		m_MaxTriangleNum = 0;
		m_MaxError = 0;

		for (size_t i = m_ClusterGroups.size(); i >= 1; --i)
		{
			size_t groupIndex = i - 1;
			const KMeshClusterGroup& group = m_ClusterGroups[groupIndex];
			for (uint32_t clusterIndex : group.clusters)
			{
				KMeshClusterPtr cluster = m_Clusters[clusterIndex];
				if (cluster->root)
				{
					m_MinTriangleNum += (uint32_t)cluster->indices.size() / 3;
				}
				else if (cluster->generatingGroupIndex == KMeshCluster::INVALID_INDEX)
				{
					m_MaxTriangleNum += (uint32_t)cluster->indices.size() / 3;
				}
				m_MaxError = glm::max(m_MaxError, cluster->error);
			}
		}

		return true;
	}

	void FindDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<uint32_t>& clusterIndices, uint32_t& triangleCount, float& error)
	{
		struct DAGCutElement
		{
			uint32_t clusterIndex;
			float error = 0;

			bool operator<(const DAGCutElement& rhs) const
			{
				return error < rhs.error;
			}

			DAGCutElement(uint32_t index, float err)
				: clusterIndex(index)
				, error(err)
			{
			}
		};

		std::vector<bool> clusterInHeap;
		clusterInHeap.resize(m_Clusters.size());

		std::priority_queue<DAGCutElement> clusterHeap;

		uint32_t curTriangleCount = 0;

		for (size_t i = m_ClusterGroups.size(); i >= 1; --i)
		{
			size_t groupIndex = i - 1;
			const KMeshClusterGroup& group = m_ClusterGroups[groupIndex];
			for (uint32_t clusterIndex : group.clusters)
			{
				KMeshClusterPtr cluster = m_Clusters[clusterIndex];
				if (cluster->root)
				{
					DAGCutElement element(clusterIndex, cluster->error);
					clusterHeap.push(element);
					curTriangleCount += (uint32_t)cluster->indices.size() / 3;
					clusterInHeap[clusterIndex] = true;
				}
			}
		}

		float minError = std::numeric_limits<float>::max();
		float curError = -1;

		while (!clusterHeap.empty())
		{
			DAGCutElement element = clusterHeap.top();

			KMeshClusterPtr cluster = m_Clusters[element.clusterIndex];
			curError = element.error;
			assert(curError == cluster->error);

			if (cluster->level == 0)
			{
				break;
			}

			bool targetHit = false;
			if (curError <= targetError || curTriangleCount >= targetTriangleCount)
			{
				targetHit = true;
			}

			if (targetHit && curError < minError)
			{
				break;
			}

			minError = std::min(minError, curError);

			clusterHeap.pop();
			curTriangleCount -= (uint32_t)m_Clusters[element.clusterIndex]->indices.size() / 3;

			const KMeshClusterGroup& group = m_ClusterGroups[cluster->generatingGroupIndex];
			for (uint32_t clusterIndex : group.childrenClusters)
			{
				if (clusterInHeap[clusterIndex])
				{
					continue;
				}
				DAGCutElement element(clusterIndex, m_Clusters[clusterIndex]->error);
				clusterHeap.push(element);
				curTriangleCount += (uint32_t)m_Clusters[clusterIndex]->indices.size() / 3;
				clusterInHeap[clusterIndex] = true;
			}
		}

		assert(curError <= targetError || curTriangleCount >= targetTriangleCount);

		triangleCount = curTriangleCount;
		error = curError;

		clusterIndices.clear();
		clusterIndices.reserve(clusterHeap.size());

		while (!clusterHeap.empty())
		{
			DAGCutElement element = clusterHeap.top();
			clusterIndices.push_back(element.clusterIndex);
			clusterHeap.pop();
		}
	}

	void ColorDebugDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, uint32_t& triangleCount, float& error)
	{
		std::vector<uint32_t> clusterIndices;
		FindDAGCut(targetTriangleCount, targetError, clusterIndices, triangleCount, error);
		KMeshTriangleClusterBuilder::ColorDebugCluster(m_Clusters, clusterIndices, vertices, indices);
	}

	void ColorDebugClusterGroup(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
	{
		std::vector<uint32_t> clusterIndices;
		for (const KMeshClusterGroup& group : m_ClusterGroups)
		{
			if (group.level == level)
			{
				for (uint32_t id : group.clusters)
				{
					clusterIndices.push_back(id);
				}
			}
		}
		KMeshTriangleClusterBuilder::ColorDebugCluster(m_Clusters, clusterIndices, vertices, indices);
	}

	uint32_t GetLevelNum() const
	{
		return m_LevelNum;
	}

	uint32_t GetMaxTriangleNum() const
	{
		return m_MaxTriangleNum;
	}

	uint32_t GetMinTriangleNum() const
	{
		return m_MinTriangleNum;
	}
	
	float GetMaxError() const
	{
		return m_MaxError;
	}
};