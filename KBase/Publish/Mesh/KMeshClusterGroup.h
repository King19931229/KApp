#pragma once
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include <unordered_set>
#include <metis.h>

struct KMeshCluster
{
	std::vector<uint32_t> indices;

	KMeshCluster()
	{}

	void Init(const std::vector<idx_t>& InIndices, size_t begin, size_t end)
	{
		indices.resize(end - begin + 1);
		for (size_t i = 0; i < indices.size(); ++i)
		{
			indices[i] = (uint32_t)InIndices[begin + i];
		}
	}
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
	struct Range
	{
		size_t begin = 0;
		size_t end = 0;
	};

	Range NewRange(size_t offset, size_t num)
	{
		Range range;
		range.begin = offset;
		range.end = range.begin + num - 1;
		return range;
	};

	std::vector<idx_t> partitionIDs;
	std::vector<idx_t> oldIndices;
	std::vector<idx_t> indices;
	std::vector<idx_t> mapto;
	std::vector<idx_t> mapback;
	std::vector<Range> ranges;

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

	void PartitionBisect(KGraph* graph, size_t maxPartition)
	{
		real_t partitionWeights[2] = { 0.5f, 0.5f };

		idx_t options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(options);

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

			PartitionBisect(&partition0, maxPartition);
			PartitionBisect(&partition1, maxPartition);
		}
	}

	void PartitionStrict(KGraph* graph, size_t maxPartition)
	{
		Start(graph->num);		
		if (graph->num < maxPartition)
		{
			ranges.push_back(NewRange(0, graph->num));
		}
		else
		{
			PartitionBisect(graph, maxPartition);
		}
		Finish();
	}

	void PartitionRelex(KGraph* graph, size_t maxPartition)
	{
		Start(graph->num);

		idx_t numPartition = (idx_t)((graph->num + (maxPartition - 1)) / maxPartition);

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
					size_t begin = partitionBegin[i];
					size_t end = (i < numPartition - 1) ? partitionBegin[i + 1] : graph->num;
					ranges.push_back(NewRange(begin, end));
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

class KMeshClusterGroup
{
protected:
	struct Triangle
	{
		int32_t index[3] = { -1, -1 ,-1 };
	};

	uint32_t m_MaxPartitionNum = 128;

	std::vector<KMeshCluster> m_Clusters;
	std::vector<Triangle> m_Triangles;

	KGraph m_Graph;

	bool BuildTriangleAdjacencies(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (indices.size() > 0 && indices.size() % 3 == 0 && vertices.size() > 0)
		{
			uint32_t numVertices = (uint32_t)vertices.size();
			uint32_t numTriangles = (uint32_t)indices.size() / 3;

			m_Triangles.resize(numTriangles);

			std::vector<std::unordered_set<idx_t>> vertexAdjacencies;
			vertexAdjacencies.resize(numVertices);
			for (uint32_t triIndex = 0; triIndex < numTriangles; ++triIndex)
			{
				uint32_t idx0 = indices[3 * triIndex + 0];
				uint32_t idx1 = indices[3 * triIndex + 1];
				uint32_t idx2 = indices[3 * triIndex + 2];

				assert(idx0 < numVertices);
				assert(idx1 < numVertices);
				assert(idx2 < numVertices);

				Triangle newTriangle = { {(int32_t)idx0, (int32_t)idx1, (int32_t)idx2} };
				m_Triangles[triIndex] = newTriangle;

				vertexAdjacencies[idx0].insert(triIndex);
				vertexAdjacencies[idx1].insert(triIndex);
				vertexAdjacencies[idx2].insert(triIndex);
			}

			m_Graph.adjacencyOffset.clear();
			m_Graph.adjacencyOffset.reserve(numTriangles);

			m_Graph.adjacency.clear();
			m_Graph.adjacency.reserve(numTriangles * 3);

			m_Graph.adjacencyCost.clear();
			m_Graph.adjacencyCost.reserve(numTriangles * 3);

			for (uint32_t triIndex = 0; triIndex < numTriangles; ++triIndex)
			{
				std::unordered_set<idx_t> triangleAdjacencies;
				for (uint32_t i = 0; i < 3; ++i)
				{
					uint32_t vertIdx = m_Triangles[triIndex].index[i];
					for (uint32_t adjancyTriIndex : vertexAdjacencies[vertIdx])
					{
						if (adjancyTriIndex != triIndex)
						{
							triangleAdjacencies.insert(adjancyTriIndex);
						}
					}
				}

				m_Graph.adjacencyOffset.push_back((idx_t)m_Graph.adjacency.size());
				m_Graph.adjacency.insert(m_Graph.adjacency.end(), triangleAdjacencies.begin(), triangleAdjacencies.end());				
				m_Graph.adjacencyCost.insert(m_Graph.adjacencyCost.end(), triangleAdjacencies.size(), 1);
			}

			m_Graph.adjacencyOffset.push_back((idx_t)m_Graph.adjacency.size());

			m_Graph.offset = 0;
			m_Graph.num = (idx_t)numTriangles;

			return true;
		}
		return false;
	}

	void Partition()
	{
		KGraphPartitioner partitioner;
		partitioner.PartitionStrict(&m_Graph, m_MaxPartitionNum);

		for (KGraphPartitioner::Range& range : partitioner.ranges)
		{
			KMeshCluster cluster;
			cluster.Init(partitioner.indices, range.begin, range.end);
			m_Clusters.push_back(std::move(cluster));
		}
	}
public:
	bool Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, int32_t maxPartitionNum)
	{
		UnInit();
		m_MaxPartitionNum = maxPartitionNum;
		if (BuildTriangleAdjacencies(vertices, indices))
		{
			Partition();
			return true;
		}
		return false;
	}

	bool UnInit()
	{
		m_Graph.Clear();
		return true;
	}
};