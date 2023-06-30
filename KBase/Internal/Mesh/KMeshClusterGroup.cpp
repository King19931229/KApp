#include "Publish/Mesh/KMeshClusterGroup.h"

void KMeshCluster::UnInit()
{
	vertices.clear();
	indices.clear();
	bound.SetNull();
	color = glm::vec3(0);
}

void KMeshCluster::InitBound()
{
	glm::vec3 maxPos = glm::vec3(std::numeric_limits<float>::min());
	glm::vec3 minPos = glm::vec3(std::numeric_limits<float>::max());
	for (const KMeshProcessorVertex& vertex : vertices)
	{
		maxPos = glm::max(maxPos, vertex.pos);
		minPos = glm::min(minPos, vertex.pos);
	}
	bound.InitFromMinMax(minPos, maxPos);
}

void KMeshCluster::Init(KMeshClusterPtr* clusters, size_t numClusters)
{
	UnInit();

	color = RandomColor();

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

	InitBound();
}

void KMeshCluster::Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<uint32_t>& inIndices)
{
	UnInit();
	color = RandomColor();
	vertices = inVertices;
	indices = inIndices;
	InitBound();
}

void KMeshCluster::Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<Triangle>& inTriangles, const std::vector<idx_t>& inTriIndices, const KRange& range)
{
	UnInit();

	color = RandomColor();

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

	InitBound();
}

KRange KGraphPartitioner::NewRange(uint32_t offset, uint32_t num)
{
	KRange range;
	range.begin = offset;
	range.end = range.begin + num - 1;
	return range;
}

void KGraphPartitioner::Start(idx_t num)
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

void KGraphPartitioner::Finish()
{
	oldIndices.clear();
	partitionIDs.clear();
	mapto.clear();
	mapback.clear();
}

void KGraphPartitioner::PartitionBisect(KGraph* graph, size_t minPartition, size_t maxPartition)
{
	if (graph->num <= maxPartition)
	{
		ranges.push_back(NewRange(graph->offset, graph->num));
		return;
	}

	idx_t expectedPartition = idx_t(minPartition + maxPartition) / 2;
	idx_t expectedNumParts = std::max(2, KMath::DivideAndRoundNearest(graph->num, (idx_t)expectedPartition));

	real_t partitionWeights[2] = { 0.5f, 0.5f };
	partitionWeights[0] = (expectedNumParts >> 1) / (real_t)expectedNumParts;
	partitionWeights[1] = 1 - partitionWeights[0];

	idx_t options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);

	bool bLoose = expectedNumParts >= 128 || maxPartition / minPartition > 1;
	bool bSlow = graph->num < 4096;
	/*
		ChatGPT:
		METIS_OPTION_UFACTOR是METIS图分区软件用于控制不平衡因子的参数。
		该参数用于限制每个分区的大小与平均分区大小之间的最大比率。
		通过调整METIS_OPTION_UFACTOR值，可以控制分区大小的平衡，从而获得更好的负载平衡，避免某些分区的计算开销过大。
		METIS_OPTION_UFACTOR的默认值为30，这意味着每个分区的大小不能超过平均分区大小的30倍。
		如果METIS_OPTION_UFACTOR的值很大，则每个分区的大小与平均分区大小之间的最大差异就会很大，这可能导致分区的负载不平衡，从而影响分区质量和计算效率。
		因此，在选择METIS_OPTION_UFACTOR的值时，需要平衡分区结果的质量和计算成本之间的权衡，以获得较好的分区效果，同时确保分区的计算成本是合理的。
	*/
	options[METIS_OPTION_UFACTOR] = bLoose ? 200 : 1;
	/*
		ChatGPT:
		METIS_OPTION_NCUTS是METIS图分区软件中的一个选项，它指定在进行分区过程中所使用的割边数目最小化算法的数量。
		METIS使用多种算法进行图分区，并且其中一些算法是基于谱图的，需要计算割边数量来实现分区优化。
		METIS_OPTION_NCUTS选项设置为一个正整数n，则METIS将使用n种不同的割边数目最小化算法来进行分区。
		较大的NCUTS值将导致METIS对多种不同的目标函数进行优化，从而增加了分区的时间和计算成本，但在某些情况下可能会导致更好的分区结果。
		通常情况下，较小的NCUTS值能够提供较好的分区结果，并且需要较少的计算资源。
	*/
	options[METIS_OPTION_NCUTS] = graph->num < 1024 ? 8 : (graph->num < 4096 ? 4 : 1);
	// options[METIS_OPTION_NCUTS] = bSlow ? 4 : 1;
	/*
		ChatGPT:
		METIS_OPTION_NITER是METIS图分区软件中用于控制迭代次数的参数。
		该参数指定METIS在分区过程中最多运行的迭代次数。METIS图分区是一种迭代算法，也就是说，它通过多次迭代来优化分区质量。
		增加迭代次数可以提高分区质量，但同时也会增加METIS的计算时间。METIS_OPTION_NITER的默认值为10，这意味着METIS在分区过程中将运行10次迭代，以优化分区质量。
		如果需要更高质量的分区，可以将METIS_OPTION_NITER增加到更高的值。但需要注意的是，增加迭代次数会显著增加METIS的计算成本，应该根据分区质量和计算时间等因素平衡考虑。
	*/
	options[METIS_OPTION_NITER] = bSlow ? 20 : 10;
	/*
		ChatGPT:
		METIS_OPTION_MINCONN是METIS图分区软件中的一个选项，它指定在图分区过程中是否强制要求保持子图的连通性。
		如果将此选项设为true，则METIS生成的所有子图都保证是连通的。这在一些应用中非常重要，比如电路设计和网络优化等领域。
		但是，为了保证连通性，可能会导致各个子图的平衡性较差，因此需要将此选项与其他划分目标（如负载平衡和最小化通信量）相平衡。
	*/
	options[METIS_OPTION_MINCONN] = 1;

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

	assert(r == METIS_OK);
	if (r == METIS_OK)
	{
		idx_t start = (idx_t)graph->offset;
		idx_t end = (idx_t)(graph->offset + graph->num - 1);
#if 1
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
#else
		while (start <= end)
		{
			while (start <= end && partitionIDs[start] == 0)
			{
				mapback[start] = mapto[start] = start;
				++start;
			}

			while (start <= end && partitionIDs[end] == 1)
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
#endif
		KGraph partition0, partition1;

		partition0.offset = graph->offset;
		partition0.num = split - graph->offset;
		partition1.offset = split;
		partition1.num = graph->offset + graph->num - split;
		assert(partition0.num + partition1.num == graph->num);

		for (idx_t i = partition0.offset; i < partition0.offset + partition0.num; ++i)
		{
			assert(partitionIDs[mapback[i]] == 0);
		}

		for (idx_t i = partition1.offset; i < partition1.offset + partition1.num; ++i)
		{
			assert(partitionIDs[mapback[i]] == 1);
		}

		assert(partition0.num > 1);
		assert(partition1.num > 1);

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
					idx_t adjIndex = mapto[graph->offset + adj] - childPartition->offset;
					if (adjIndex >= 0 && adjIndex < childPartition->num)
					{
						childPartition->adjacency.push_back(adjIndex);
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
}

void KGraphPartitioner::PartitionStrict(KGraph* graph, size_t minPartition, size_t maxPartition)
{
	Start(graph->num);
	if (graph->num < maxPartition)
	{
		ranges.push_back(NewRange(graph->offset, graph->num));
	}
	else
	{
		PartitionBisect(graph, minPartition, maxPartition);
	}
	Finish();
}

void KGraphPartitioner::PartitionRelex(KGraph* graph, size_t minPartition, size_t maxPartition)
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


bool KMeshTriangleClusterBuilder::BuildTriangleAdjacencies(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, Adjacency& context)
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

void KMeshTriangleClusterBuilder::Partition(Adjacency& context)
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

bool KMeshTriangleClusterBuilder::UnInit()
{
	m_Clusters.clear();
	return true;
}

bool KMeshTriangleClusterBuilder::Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, uint32_t minPartitionNum, uint32_t maxPartitionNum)
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

void KVirtualGeometryBuilder::DAGReduce(uint32_t childrenBegin, uint32_t childrenEnd, uint32_t level)
{
	if (m_CheckClusterAdacency)
	{
		assert(CheckClustersAdjacency(childrenBegin, childrenEnd));
	}

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

		KMeshClusterGroupPtr newGroup = KMeshClusterGroupPtr(new KMeshClusterGroup());
		newGroup->level = level + 1;
		newGroup->index = (uint32_t)m_ClusterGroups.size();
		newGroup->color = KMeshCluster::RandomColor();

		newGroup->childrenClusters.resize(numChildren);
		for (uint32_t i = 0; i < numChildren; ++i)
		{
			uint32_t idx = childrenBegin + i;
			m_Clusters[idx]->groupIndex = newGroup->index;
			error = std::max(error, m_Clusters[idx]->error);
			// 这里要这样获取index 因为图划分后Clusters被重排了
			newGroup->childrenClusters[i] = m_Clusters[idx]->index;
		}

		newGroup->clusters.resize(numParent);
		for (uint32_t i = 0; i < numParent; ++i)
		{
			uint32_t idx = parentBegin + i;
			m_Clusters[idx]->index = parentBegin + i;
			m_Clusters[idx]->error = error;
			m_Clusters[idx]->level = newGroup->level;
			m_Clusters[idx]->generatingGroupIndex = newGroup->index;
			newGroup->clusters[i] = idx;
			newGroup->bound = newGroup->bound.Merge(m_Clusters[idx]->bound);
		}

		newGroup->maxError = error;

		for (uint32_t i = 0; i < numChildren; ++i)
		{
			uint32_t idx = childrenBegin + i;
			m_Clusters[idx]->maxParentError = error;
		}

		m_ClusterGroups.push_back(newGroup);
	}
}

void KVirtualGeometryBuilder::ClusterTriangle(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices)
{
	KMeshTriangleClusterBuilder builder;
	builder.Init(vertices, indices, m_MinPartitionNum, m_MaxPartitionNum);
	builder.GetClusters(m_Clusters);
}

std::vector<std::unordered_map<size_t, size_t>> KVirtualGeometryBuilder::MaskClustersAdjacency(const std::vector<idx_t>& clusterIndices)
{
	std::unordered_map<size_t, size_t> vertNewIdxMap;

	size_t verticeHashSize = 0;
	for (size_t idx : clusterIndices)
	{
		verticeHashSize += m_Clusters[idx]->indices.size();
	}

	std::vector<size_t> verticeHash;
	verticeHash.resize(verticeHashSize);

	size_t verticeHashIdxBegin = 0;

	for (size_t idx : clusterIndices)
	{
		const KMeshCluster& cluster = *m_Clusters[idx];
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

	size_t clusterID = 0;
	for (size_t idx : clusterIndices)
	{
		const KMeshCluster& cluster = *m_Clusters[idx];
		for (size_t i = 0; i < cluster.indices.size(); ++i)
		{
			size_t v0Hash = verticeHash[verticeHashIdxBegin + i];
			size_t v1Hash = verticeHash[verticeHashIdxBegin + 3 * (i / 3) + (i + 1) % 3];
			int32_t newV0 = (int32_t)vertNewIdxMap[v0Hash];
			int32_t newV1 = (int32_t)vertNewIdxMap[v1Hash];
			edgeHash.AddEdgeHash((int32_t)newV0, (int32_t)newV1, (int32_t)clusterID);
		}
		verticeHashIdxBegin += cluster.indices.size();
		++clusterID;
	}

	size_t clusterNum = clusterID;

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

	for (size_t i = 0; i < clusterNum; ++i)
	{
		for (size_t j = i + 1; j < clusterNum; ++j)
		{
			auto it = clusterAdj[i].find(j);
			if (it != clusterAdj[i].end())
			{
				size_t cost = it->second;
				auto itBack = clusterAdj[j].find(i);
				assert(itBack != clusterAdj[j].end());
				size_t costBack = itBack->second;
				assert(cost == costBack);
			}
		}
	}

	return clusterAdj;
}

std::vector<std::unordered_map<size_t, size_t>> KVirtualGeometryBuilder::MaskClustersAdjacency(idx_t begin, idx_t end)
{
	std::vector<idx_t> clusterIndices;
	clusterIndices.resize(end - begin + 1);
	for (idx_t i = begin; i <= end; ++i)
	{
		clusterIndices[i - begin] = i;
	}
	return MaskClustersAdjacency(clusterIndices);
}

bool KVirtualGeometryBuilder::CheckClustersIndex()
{
	for (size_t i = 0; i < m_Clusters.size(); ++i)
	{
		if (m_Clusters[i]->index != (uint32_t)i)
		{
			return false;
		}
	}
	return true;
}

bool KVirtualGeometryBuilder::CheckClustersAdjacency(const std::vector<idx_t>& clusterIndices)
{
	std::vector<std::unordered_map<size_t, size_t>> clusterAdj = MaskClustersAdjacency(clusterIndices);
	for (size_t i = 0; i < clusterAdj.size(); ++i)
	{
		if (clusterAdj[i].empty())
		{
			return false;
		}
	}
	return true;
}

bool KVirtualGeometryBuilder::CheckClustersAdjacency(idx_t begin, idx_t end)
{
	std::vector<std::unordered_map<size_t, size_t>> clusterAdj = MaskClustersAdjacency(begin, end);
	for (size_t i = 0; i < clusterAdj.size(); ++i)
	{
		if (clusterAdj[i].empty())
		{
			return false;
		}
	}
	return true;
}

KGraph KVirtualGeometryBuilder::BuildClustersAdjacency(idx_t begin, idx_t end)
{
	std::vector<std::unordered_map<size_t, size_t>> clusterAdj = MaskClustersAdjacency(begin, end);
	size_t clusterNum = end - begin + 1;

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
			bool sibling = m_Clusters[clusterID]->generatingGroupIndex != KVirtualGeometryDefine::INVALID_INDEX && m_Clusters[clusterID]->generatingGroupIndex == m_Clusters[anotherClusterID]->generatingGroupIndex;
			cost *= sibling ? 1 : 16;
			cost += 4;
			graph.adjacency.push_back((idx_t)anotherClusterID);
			graph.adjacencyCost.push_back((idx_t)cost);
		}
	}
	graph.adjacencyOffset.push_back((idx_t)graph.adjacency.size());

	return graph;
}

bool KVirtualGeometryBuilder::ColorDebugClusters(const std::vector<KMeshClusterPtr>& clusters, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
{
	vertices.clear();
	indices.clear();

	uint32_t clusterIndexBegin = 0;

	for (uint32_t id : ids)
	{
		const KMeshCluster& cluster = *clusters[id];
		std::vector<KMeshProcessorVertex> clusterVertices = cluster.vertices;
		std::vector<uint32_t> clusterIndices = cluster.indices;
		const glm::vec3& clusterColor = cluster.color;

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

bool KVirtualGeometryBuilder::ColorDebugClusterGroups(const std::vector<KMeshClusterPtr>& clusters, const std::vector<KMeshClusterGroupPtr>& groups, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
{
	vertices.clear();
	indices.clear();

	uint32_t clusterIndexBegin = 0;

	for (uint32_t id : ids)
	{
		const KMeshClusterGroup& group = *groups[id];
		const glm::vec3& clusterGruopColor = group.color;

		for (uint32_t clusterIndex : group.clusters)
		{
			const KMeshCluster& cluster = *clusters[clusterIndex];
			std::vector<KMeshProcessorVertex> clusterVertices = cluster.vertices;
			std::vector<uint32_t> clusterIndices = cluster.indices;

			for (uint32_t& index : clusterIndices)
			{
				index += clusterIndexBegin;
			}
			for (KMeshProcessorVertex& vertex : clusterVertices)
			{
				vertex.color[0] = clusterGruopColor;
			}

			vertices.insert(vertices.end(), clusterVertices.begin(), clusterVertices.end());
			indices.insert(indices.end(), clusterIndices.begin(), clusterIndices.end());

			clusterIndexBegin += (uint32_t)clusterVertices.size();
		}
	}

	return true;
}

void KVirtualGeometryBuilder::BuildDAG(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, uint32_t minPartitionNum, uint32_t maxPartitionNum)
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
		KMeshClusterGroupPtr newGroup = KMeshClusterGroupPtr(new KMeshClusterGroup());
		uint32_t num = end - begin + 1;
		newGroup->level = 0;
		newGroup->clusters.resize(num);
		newGroup->index = (uint32_t)m_ClusterGroups.size();
		newGroup->color = KMeshCluster::RandomColor();
		for (uint32_t i = 0; i < num; ++i)
		{
			uint32_t idx = begin + i;
			m_Clusters[idx]->index = idx;
			m_Clusters[idx]->generatingGroupIndex = newGroup->index;
			newGroup->clusters[i] = idx;
			newGroup->bound = newGroup->bound.Merge(m_Clusters[idx]->bound);
		}
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
			for (const KRange& range : partitioner.ranges)
			{
				for (uint32_t idx = range.begin; idx <= range.end; ++idx)
				{
					newOrderCluster[idx] = m_Clusters[levelClusterBegin + partitioner.indices[idx]];
				}
			}

			if (m_CheckClusterAdacency)
			{
				for (const KRange& range : partitioner.ranges)
				{
					std::vector<idx_t> clusterIndices;
					clusterIndices.resize(range.end - range.begin + 1);
					for (uint32_t idx = range.begin; idx <= range.end; ++idx)
					{
						clusterIndices[idx - range.begin] = levelClusterBegin + partitioner.indices[idx];
					}
					assert(CheckClustersAdjacency(clusterIndices));
				}
			}

			for (size_t i = 0; i < levelClusterNum; ++i)
			{
				m_Clusters[i + levelClusterBegin] = newOrderCluster[i];
			}

			if (m_CheckClusterAdacency)
			{
				for (const KRange& range : partitioner.ranges)
				{
					std::vector<idx_t> clusterIndices;
					clusterIndices.resize(range.end - range.begin + 1);
					for (uint32_t idx = range.begin; idx <= range.end; ++idx)
					{
						clusterIndices[idx - range.begin] = levelClusterBegin + idx;
					}
					assert(CheckClustersAdjacency(clusterIndices));
				}
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

	std::sort(m_Clusters.begin(), m_Clusters.end(), [](const KMeshClusterPtr& lhs, const KMeshClusterPtr& rhs) -> bool { return lhs->index < rhs->index; });

	m_LevelNum = currentLevel;

	m_MinTriangleNum = 0;
	m_MaxTriangleNum = 0;
	m_MaxError = 0;

	for (size_t i = m_ClusterGroups.size(); i >= 1; --i)
	{
		size_t groupIndex = i - 1;
		const KMeshClusterGroup& group = *m_ClusterGroups[groupIndex];
		for (uint32_t clusterIndex : group.clusters)
		{
			KMeshClusterPtr cluster = m_Clusters[clusterIndex];
			if (cluster->groupIndex == KVirtualGeometryDefine::INVALID_INDEX)
			{
				m_MinTriangleNum += (uint32_t)cluster->indices.size() / 3;
			}
			if (cluster->level == 0)
			{
				m_MaxTriangleNum += (uint32_t)cluster->indices.size() / 3;
			}
			m_MaxError = glm::max(m_MaxError, cluster->error);
		}
	}
}

void KVirtualGeometryBuilder::BuildClusterStorage()
{
	m_ClusterStorageParts.clear();
	m_ClusterStorageParts.reserve(m_ClusterGroups.size());

	for (uint32_t groupIndex = 0; groupIndex < (uint32_t)m_ClusterGroups.size(); ++groupIndex)
	{
		const KMeshClusterGroup& group = *m_ClusterGroups[groupIndex];
		KMeshClusterStoragePartPtr part = KMeshClusterStoragePartPtr(new KMeshClusterStoragePart());
		part->clusters = group.clusters;
		part->groupIndex = groupIndex;
		part->level = group.level;
		part->bound = group.bound;
		m_ClusterStorageParts.push_back(part);
	}
}

float KVirtualGeometryBuilder::SortBVHNodesByAxis(const std::vector<KMeshClusterBVHNodePtr>& bvhNodes, SortAxis axis, const std::vector<uint32_t>& indices, const KRange& range, std::vector<uint32_t>& sorted)
{
	assert(sorted.size() == (range.end - range.begin + 1));
	for (uint32_t i = range.begin; i <= range.end; ++i)
	{
		sorted[i - range.begin] = indices[i];
	}

	std::sort(sorted.begin(), sorted.end(), [&bvhNodes, axis](uint32_t lhs, uint32_t rhs)
	{
		if (axis == AXIS_X)
		{
			return bvhNodes[lhs]->bound.GetCenter().x < bvhNodes[lhs]->bound.GetCenter().x;
		}
		else if (axis == AXIS_Y)
		{
			return bvhNodes[lhs]->bound.GetCenter().y < bvhNodes[lhs]->bound.GetCenter().y;
		}
		else
		{
			return bvhNodes[lhs]->bound.GetCenter().z < bvhNodes[lhs]->bound.GetCenter().z;
		}
	});

	uint32_t num = range.end - range.begin + 1;
	uint32_t halfNum = (range.end - range.begin + 1) / 2;

	KAABBBox halfBound[2];

	for (uint32_t i = 0; i < halfNum; ++i)
	{
		halfBound[0] = halfBound[0].Merge(bvhNodes[sorted[i]]->bound);
		halfBound[1] = halfBound[1].Merge(bvhNodes[sorted[i + halfNum]]->bound);
	}

	for (uint32_t i = 2 * halfNum; i < num; ++i)
	{
		halfBound[1] = halfBound[1].Merge(bvhNodes[sorted[i]]->bound);
	}

	return halfBound[0].Surface() + halfBound[1].Surface();
}

void KVirtualGeometryBuilder::SortBVHNodes(const std::vector<KMeshClusterBVHNodePtr>& bvhNodes, std::vector<uint32_t>& indices)
{
	for (uint32_t level = 0; level < KVirtualGeometryDefine::MAX_BVH_NODES_BITS; ++level)
	{
		uint32_t bucketNum = 1 << level;
		uint32_t numPerBucket = KVirtualGeometryDefine::MAX_BVH_NODES / bucketNum;
		// uint32_t halfNumPerBucket = numPerBucket / 2;

		for (uint32_t bucketIndex = 0; bucketIndex < bucketNum; ++bucketIndex)
		{
			std::vector<uint32_t> sorted;
			std::vector<uint32_t> bestSorted;

			sorted.resize(numPerBucket);
			bestSorted.resize(numPerBucket);

			float best = std::numeric_limits<float>::max();

			KRange range;
			range.begin = bucketIndex * numPerBucket;
			range.end = range.begin + numPerBucket - 1;

			for (uint32_t axis = 0; axis < SortAxis::AXIS_NUM; ++axis)
			{
				float score = SortBVHNodesByAxis(bvhNodes, (SortAxis)axis, indices, range, sorted);
				if (score < best)
				{
					best = score;
					bestSorted = sorted;
				}
			}

			for (uint32_t i = 0; i < numPerBucket; ++i)
			{
				indices[i + range.begin] = bestSorted[i];
			}
		}
	}
}

uint32_t KVirtualGeometryBuilder::BuildHierarchyTopDown(std::vector<KMeshClusterBVHNodePtr>& bvhNodes, std::vector<uint32_t>& indices, bool sort)
{
	uint32_t nodeNum = (uint32_t)indices.size();
	
	if (nodeNum == 1)
	{
		return indices[0];
	}

	uint32_t rootIndex = (uint32_t)bvhNodes.size();
	KMeshClusterBVHNodePtr root = KMeshClusterBVHNodePtr(new KMeshClusterBVHNode());
	bvhNodes.push_back(root);

	if (nodeNum <= KVirtualGeometryDefine::MAX_BVH_NODES)
	{
		for (uint32_t childIndex : indices)
		{
			root->bound = root->bound.Merge(bvhNodes[childIndex]->bound);
		}
		root->children = indices;
		return rootIndex;
	}

	uint32_t maxNode = KVirtualGeometryDefine::MAX_BVH_NODES;
	while (maxNode * KVirtualGeometryDefine::MAX_BVH_NODES <= nodeNum)
	{
		maxNode *= KVirtualGeometryDefine::MAX_BVH_NODES;
	}

	uint32_t maxNodeNum = maxNode;
	uint32_t minNodeNum = maxNode / KVirtualGeometryDefine::MAX_BVH_NODES;
	uint32_t maxAddNodeNum = maxNodeNum - minNodeNum;
	uint32_t restNodeNum = nodeNum - maxNode;

	uint32_t childNodeNums[KVirtualGeometryDefine::MAX_BVH_NODES] = { 0 };

	for (uint32_t child = 0; child < KVirtualGeometryDefine::MAX_BVH_NODES; ++child)
	{
		uint32_t addNodeNum = std::min(restNodeNum, maxAddNodeNum);
		uint32_t childNodeNum = minNodeNum + addNodeNum;
		childNodeNums[child] = childNodeNum;
		restNodeNum -= addNodeNum;
	}
	assert(restNodeNum == 0);

	if (sort)
	{
		SortBVHNodes(bvhNodes, indices);
	}

	root->children.resize(KVirtualGeometryDefine::MAX_BVH_NODES);

	uint32_t childOffset = 0;
	for (uint32_t child = 0; child < KVirtualGeometryDefine::MAX_BVH_NODES; ++child)
	{
		uint32_t childNum = childNodeNums[child];
		std::vector<uint32_t> childIndices = std::vector<uint32_t>(indices.begin() + childOffset, indices.begin() + childOffset + childNum);
		uint32_t childIndex = BuildHierarchyTopDown(bvhNodes, childIndices, true);
		root->children[child] = childIndex;
		root->bound = root->bound.Merge(bvhNodes[childIndex]->bound);
		childOffset += childNum;
	}

	return rootIndex;
}

void KVirtualGeometryBuilder::BuildClusterBVH()
{
	std::vector<KMeshClusterBVHNodePtr> bvhNodes;
	bvhNodes.resize(m_ClusterStorageParts.size());

	for (uint32_t partIndex = 0; partIndex < (uint32_t)m_ClusterStorageParts.size(); ++partIndex)
	{
		KMeshClusterBVHNodePtr newLeaf = KMeshClusterBVHNodePtr(new KMeshClusterBVHNode());
		KMeshClusterStoragePartPtr clusterPart = m_ClusterStorageParts[partIndex];
		newLeaf->partIndex = partIndex;
		newLeaf->bound = clusterPart->bound;
		bvhNodes[partIndex] = newLeaf;
	}

	uint32_t rootIndex = 0;

	if (bvhNodes.size() == 1)
	{
		KMeshClusterBVHNodePtr root = KMeshClusterBVHNodePtr(new KMeshClusterBVHNode());
		root->children = { 0 };
		bvhNodes.push_back(root);
		rootIndex = 1;
	}
	else
	{
		uint32_t maxLevel = 0;
		for (KMeshClusterBVHNodePtr node : bvhNodes)
		{
			if (m_ClusterStorageParts[node->partIndex]->level > maxLevel)
			{
				maxLevel = m_ClusterStorageParts[node->partIndex]->level;
			}
		}

		std::vector<std::vector<uint32_t>> indicesByLevel;
		indicesByLevel.resize(maxLevel + 1);

		for (uint32_t index = 0; index < (uint32_t)bvhNodes.size(); ++index)
		{
			KMeshClusterBVHNodePtr node = bvhNodes[index];
			uint32_t level = m_ClusterStorageParts[node->partIndex]->level;
			indicesByLevel[level].push_back(index);
		}

		std::vector<uint32_t> roots;
		roots.reserve(maxLevel);

		for (uint32_t level = 0; level < maxLevel; ++level)
		{
			uint32_t levelRootIndex = BuildHierarchyTopDown(bvhNodes, indicesByLevel[level], true);
			if (bvhNodes[levelRootIndex]->partIndex != KVirtualGeometryDefine::INVALID_INDEX ||
				bvhNodes[levelRootIndex]->children.size() == KVirtualGeometryDefine::MAX_BVH_NODES)
			{
				roots.push_back(levelRootIndex);
			}
			else
			{
				for (uint32_t child : bvhNodes[levelRootIndex]->children)
				{
					roots.push_back(child);
				}
			}
		}

		rootIndex = BuildHierarchyTopDown(bvhNodes, roots, false);
	}

	m_BVHNodes = std::move(bvhNodes);
	m_BVHRoot = rootIndex;
}

void KVirtualGeometryBuilder::FindDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<uint32_t>& clusterIndices, uint32_t& triangleCount, float& error) const
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
		const KMeshClusterGroup& group = *m_ClusterGroups[groupIndex];
		for (uint32_t clusterIndex : group.clusters)
		{
			KMeshClusterPtr cluster = m_Clusters[clusterIndex];
			if (cluster->groupIndex == KVirtualGeometryDefine::INVALID_INDEX)
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

		const KMeshClusterGroup& group = *m_ClusterGroups[cluster->generatingGroupIndex];
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

void KVirtualGeometryBuilder::ColorDebugDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, uint32_t& triangleCount, float& error) const
{
	std::vector<uint32_t> clusterIndices;
	FindDAGCut(targetTriangleCount, targetError, clusterIndices, triangleCount, error);
	ColorDebugClusters(m_Clusters, clusterIndices, vertices, indices);
}

void KVirtualGeometryBuilder::ColorDebugCluster(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices) const
{
	vertices.clear();
	indices.clear();

	std::vector<uint32_t> clusterIndices;
	for (KMeshClusterGroupPtr group : m_ClusterGroups)
	{
		if (group->level == level)
		{
			for (uint32_t id : group->clusters)
			{
				clusterIndices.push_back(id);
			}
		}
	}

	ColorDebugClusters(m_Clusters, clusterIndices, vertices, indices);
}

void KVirtualGeometryBuilder::ColorDebugClusterGroup(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices) const
{
	vertices.clear();
	indices.clear();

	std::vector<uint32_t> groupIndices;
	for (size_t i = 0; i < m_ClusterGroups.size(); ++i)
	{
		const KMeshClusterGroup& group = *m_ClusterGroups[i];
		std::vector<uint32_t> clusterIndices;
		if (group.level == level)
		{
			groupIndices.push_back((uint32_t)i);
		}
	}

	ColorDebugClusterGroups(m_Clusters, m_ClusterGroups, groupIndices, vertices, indices);
}

void KVirtualGeometryBuilder::DumpClusterGroupAsOBJ(const std::string& saveRoot) const
{
	for (size_t groupIdx = 0; groupIdx < m_ClusterGroups.size(); ++groupIdx)
	{
		const KMeshClusterGroup& group = *m_ClusterGroups[groupIdx];

		std::stringstream ss;
		ss << "cluster_group_" << group.level << "_" << groupIdx;
		std::string objName = ss.str();
		std::string filePath;
		if (KFileTool::PathJoin(saveRoot, objName + ".obj", filePath))
		{
			if (!KFileTool::IsPathExist(saveRoot))
			{
				KFileTool::CreateFolder(saveRoot, true);
			}

			IKDataStreamPtr dataStream = GetDataStream(IT_FILEHANDLE);
			if (!dataStream->Open(filePath.c_str(), IM_WRITE))
			{
				dataStream->Close();
				continue;
			}

			std::stringstream fileSS;
			fileSS << std::fixed << std::setprecision(10);
			fileSS << "o " << objName << std::endl;

			std::vector<size_t> clusterIndexOffset;
			clusterIndexOffset.resize(group.clusters.size());
			clusterIndexOffset[0] = 0;

			size_t t = 0;
			for (uint32_t clusterIdx : group.clusters)
			{
				KMeshClusterPtr cluster = m_Clusters[clusterIdx];
				for (size_t i = 0; i < cluster->vertices.size(); ++i)
				{
					const KMeshProcessorVertex& vertex = cluster->vertices[i];
					fileSS << "# " << i << std::endl;
					fileSS << "v " << vertex.pos.x << " " << vertex.pos.y << " " << vertex.pos.z << std::endl;
					fileSS << "vt " << vertex.uv.x << " " << vertex.uv.y << std::endl;
					fileSS << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << std::endl;
				}
				if (group.clusters.size() >= 1 && t < group.clusters.size() - 1)
				{
					clusterIndexOffset[t + 1] = clusterIndexOffset[t] + cluster->vertices.size();
				}
				++t;
			}

			t = 0;
			for (uint32_t clusterIdx : group.clusters)
			{
				KMeshClusterPtr cluster = m_Clusters[clusterIdx];
				uint32_t indexOffset = (uint32_t)clusterIndexOffset[t];
				for (size_t i = 0; i < cluster->indices.size(); i += 3)
				{
					uint32_t idx0 = indexOffset + 1 + cluster->indices[i];
					uint32_t idx1 = indexOffset + 1 + cluster->indices[i + 1];
					uint32_t idx2 = indexOffset + 1 + cluster->indices[i + 2];
					fileSS << "f ";
					fileSS << idx0 << "/" << idx0 << "/" << idx0;
					fileSS << " ";
					fileSS << idx1 << "/" << idx1 << "/" << idx1;
					fileSS << " ";
					fileSS << idx2 << "/" << idx2 << "/" << idx2;
					fileSS << std::endl;
				}
				++t;
			}

			std::string data = fileSS.str();
			dataStream->Write(data.c_str(), data.length());
			dataStream->Close();
		}
	}
}

void KVirtualGeometryBuilder::DumpClusterAsOBJ(const std::string& saveRoot) const
{
	for (size_t clusterIdx = 0; clusterIdx < m_Clusters.size(); ++clusterIdx)
	{
		KMeshClusterPtr cluster = m_Clusters[clusterIdx];
		std::stringstream ss;
		ss << "cluster" << clusterIdx << "_" << cluster->level;
		std::string objName = ss.str();
		std::string filePath;
		if (KFileTool::PathJoin(saveRoot, objName + ".obj", filePath))
		{
			if (!KFileTool::IsPathExist(saveRoot))
			{
				KFileTool::CreateFolder(saveRoot, true);
			}

			IKDataStreamPtr dataStream = GetDataStream(IT_FILEHANDLE);
			if (!dataStream->Open(filePath.c_str(), IM_WRITE))
			{
				dataStream->Close();
				continue;
			}

			std::stringstream fileSS;
			fileSS << std::fixed << std::setprecision(10);
			fileSS << "o " << objName << std::endl;
			for (size_t i = 0; i < cluster->vertices.size(); ++i)
			{
				const KMeshProcessorVertex& vertex = cluster->vertices[i];
				fileSS << "# " << i << std::endl;
				fileSS << "v " << vertex.pos.x << " " << vertex.pos.y << " " << vertex.pos.z << std::endl;
				fileSS << "vt " << vertex.uv.x << " " << vertex.uv.y << std::endl;
				fileSS << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << std::endl;
			}
			fileSS << "s off" << std::endl;
			for (size_t i = 0; i < cluster->indices.size(); i += 3)
			{
				uint32_t idx0 = 1 + cluster->indices[i];
				uint32_t idx1 = 1 + cluster->indices[i + 1];
				uint32_t idx2 = 1 + cluster->indices[i + 2];
				fileSS << "f ";
				fileSS << idx0 << "/" << idx0 << "/" << idx0;
				fileSS << " ";
				fileSS << idx1 << "/" << idx1 << "/" << idx1;
				fileSS << " ";
				fileSS << idx2 << "/" << idx2 << "/" << idx2;
				fileSS << std::endl;
			}

			std::string data = fileSS.str();
			dataStream->Write(data.c_str(), data.length());
			dataStream->Close();
		}
	}
}

void KVirtualGeometryBuilder::DumpClusterInformation(const std::string& saveRoot) const
{
	std::string filePath;
	if (KFileTool::PathJoin(saveRoot, "cluster.csv", filePath))
	{
		if (!KFileTool::IsPathExist(saveRoot))
		{
			KFileTool::CreateFolder(saveRoot, true);
		}

		IKDataStreamPtr dataStream = GetDataStream(IT_FILEHANDLE);
		if (!dataStream->Open(filePath.c_str(), IM_WRITE))
		{
			dataStream->Close();
			return;
		}

		std::stringstream ss;
		ss << "Index,LodLevel,Root,TriangleNum,Error,AdjacenciesAsChildren,AdjacenciesAsParent,Children,Parent," << std::endl;
		for (size_t i = 0; i < m_Clusters.size(); ++i)
		{
			KMeshClusterPtr cluster = m_Clusters[i];

			std::string adjacenciesAsChildren;
			std::string adjacenciesAsParent;
			std::string children;
			std::string parents;

			if (cluster->groupIndex != KVirtualGeometryDefine::INVALID_INDEX)
			{
				const KMeshClusterGroup& groupAsChildren = *m_ClusterGroups[cluster->groupIndex];

				{
					size_t t = 0;
					std::stringstream ssParents;
					ssParents << "\"";
					for (uint32_t parent : groupAsChildren.clusters)
					{
						++t;
						ssParents << parent;
						if (t != groupAsChildren.clusters.size())
						{
							ssParents << ",";
						}
					}
					ssParents << "\"";
					parents = ssParents.str();
				}

				{
					size_t t = 0;
					std::stringstream ssAdjacencies;
					ssAdjacencies << "\"";
					for (uint32_t adj : groupAsChildren.childrenClusters)
					{
						if (adj == i)
						{
							continue;
						}
						++t;
						ssAdjacencies << adj;
						if (t != groupAsChildren.childrenClusters.size() - 1)
						{
							ssAdjacencies << ",";
						}
					}
					ssAdjacencies << "\"";
					adjacenciesAsChildren = ssAdjacencies.str();
				}
			}

			if (cluster->generatingGroupIndex != KVirtualGeometryDefine::INVALID_INDEX)
			{
				const KMeshClusterGroup& groupAsParent = *m_ClusterGroups[cluster->generatingGroupIndex];

				{
					std::stringstream ssChildren;
					ssChildren << "\"";
					size_t t = 0;
					for (uint32_t child : groupAsParent.childrenClusters)
					{
						++t;
						ssChildren << child;
						if (t != groupAsParent.childrenClusters.size())
						{
							ssChildren << ",";
						}
					}
					ssChildren << "\"";
					children = ssChildren.str();
				}

				{
					size_t t = 0;
					std::stringstream ssAdjacencies;
					ssAdjacencies << "\"";
					for (uint32_t adj : groupAsParent.clusters)
					{
						if (adj == i)
						{
							continue;
						}
						++t;
						ssAdjacencies << adj;
						if (t != groupAsParent.clusters.size() - 1)
						{
							ssAdjacencies << ",";
						}
					}
					ssAdjacencies << "\"";
					adjacenciesAsParent = ssAdjacencies.str();
				}
			}

			ss << i << "," << cluster->level << "," << (cluster->groupIndex == KVirtualGeometryDefine::INVALID_INDEX) << "," << cluster->indices.size() / 3 << "," << cluster->error << "," << adjacenciesAsChildren << "," << adjacenciesAsParent << "," << children << "," << parents;
			ss << std::endl;
		}

		std::string data = ss.str();
		dataStream->Write(data.c_str(), data.length());
		dataStream->Close();
	}
}

void KVirtualGeometryBuilder::Build(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices)
{
	BuildDAG(vertices, indices, 124, 128);
	BuildClusterStorage();
	BuildClusterBVH();
}