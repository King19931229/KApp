#include "Publish/Mesh/KVirtualGeometryBuilder.h"

void KMeshCluster::UnInit()
{
	vertices.clear();
	indices.clear();
	materialIndices.clear();
	lodBound.SetNull();
	color = glm::vec3(0);
	lodError = 0;
	edgeLength = 0;
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
	lodBound = bound;

	assert(indices.size() % 3 == 0);
	float maxEdgeLengthSquare = 0;

	for (uint32_t idx = 0; idx < (uint32_t)indices.size() - 1; ++idx)
	{
		uint32_t id0 = indices[idx];
		uint32_t id1 = indices[3 * (idx / 3) + (idx + 1) % 3];
		glm::vec3 d = vertices[id0].pos - vertices[id1].pos;
		maxEdgeLengthSquare = std::max(glm::dot(d, d), maxEdgeLengthSquare);
	}
	edgeLength = sqrt(maxEdgeLengthSquare);
}

void KMeshCluster::InitMaterial()
{
	struct MaterialIndexSort
	{
		uint32_t triIndex = 0;
		uint32_t materialIndex = 0;
	};

	std::vector<MaterialIndexSort> materialIndexSorter;
	materialIndexSorter.resize(materialIndices.size());

	for (size_t i = 0; i < materialIndices.size(); ++i)
	{
		materialIndexSorter[i].triIndex = (uint32_t)i;
		materialIndexSorter[i].materialIndex = materialIndices[i];
	}

	std::sort(materialIndexSorter.begin(), materialIndexSorter.end(), [](const MaterialIndexSort& lhs, const MaterialIndexSort& rhs)
	{
		if (lhs.materialIndex != rhs.materialIndex)
			return lhs.materialIndex < rhs.materialIndex;
		else
			return lhs.triIndex < rhs.triIndex;
	});

	std::vector<uint32_t> oldIndices = std::move(indices);
	indices.resize(oldIndices.size());

	for (size_t i = 0; i < materialIndices.size(); ++i)
	{
		materialIndices[i] = materialIndexSorter[i].materialIndex;
		uint32_t triIndex = materialIndexSorter[i].triIndex;
		for (size_t idx = 0; idx < 3; ++idx)
		{
			indices[3 * i + idx] = oldIndices[3 * triIndex + idx];
		}
	}

	EnsureIndexOrder();
}

void KMeshCluster::EnsureIndexOrder()
{
	for (size_t i = 0; i < indices.size() / 3; ++i)
	{
		uint32_t index0 = indices[3 * i];
		uint32_t index1 = indices[3 * i + 1];
		uint32_t index2 = indices[3 * i + 2];
		if (index1 < index0 && index1 < index2)
		{
			indices[3 * i] = index1;
			indices[3 * i + 1] = index2;
			indices[3 * i + 2] = index0;
		}
		else if (index2 < index0 && index2 < index1)
		{
			indices[3 * i] = index2;
			indices[3 * i + 1] = index0;
			indices[3 * i + 2] = index1;
		}
	}
}

bool KMeshCluster::BuildMaterialRange()
{
	materialRanges.clear();

	if (materialIndices.size() > 0)
	{
		uint32_t lastMaterialIndex = materialIndices[0];
		uint32_t lastMaterialIndexRangeBegin = 0;

		for (uint32_t i = 1; i <= (uint32_t)materialIndices.size(); ++i)
		{
			if (i == materialIndices.size() || materialIndices[i] != lastMaterialIndex)
			{
				KMeshClusterMaterialRange newRange;

				newRange.start = lastMaterialIndexRangeBegin;
				newRange.length = i - lastMaterialIndexRangeBegin;
				newRange.materialIndex = lastMaterialIndex;
				newRange.batchTriCounts = { newRange.length };

				materialRanges.push_back(newRange);

				if (i == materialIndices.size())
				{
					break;
				}

				lastMaterialIndex = materialIndices[i];
				lastMaterialIndexRangeBegin = i;
			}
		}
	}

	uint32_t oldToNew[KVirtualGeometryDefine::MAX_CLUSTER_VERTEX];
	uint32_t newToOld[KVirtualGeometryDefine::MAX_CLUSTER_VERTEX];
	uint32_t optimized[KVirtualGeometryDefine::MAX_CLUSTER_TRIANGLE * 3];

	memset(oldToNew, -1, sizeof(oldToNew));
	memset(newToOld, -1, sizeof(newToOld));
	memset(optimized, -1, sizeof(optimized));

	uint32_t currentVertexNum = 0;
	uint32_t newVertexNum = 0;

	for (const KMeshClusterMaterialRange& range : materialRanges)
	{
		uint32_t triIndex = range.start;
		for (uint32_t triangleCount : range.batchTriCounts)
		{
			for (uint32_t i = 0; i < triangleCount; ++i)
			{
				newVertexNum = currentVertexNum;

				uint32_t oldIndex[3] = {};
				uint32_t* pNewIndex[3] = {};

				for (uint32_t i = 0; i < 3; ++i)
				{
					oldIndex[i] = indices[3 * triIndex + i];
					pNewIndex[i] = &oldToNew[oldIndex[i]];
					newVertexNum += (*pNewIndex[i] == 0xFFFFFFFF);
				}

				do
				{
					for (uint32_t i = 0; i < 3; ++i)
					{
						if (*pNewIndex[i] != 0xFFFFFFFF && newVertexNum - *pNewIndex[i] >= KVirtualGeometryDefine::MAX_CLUSTER_REUSE_BATCH)
						{
							*pNewIndex[i] = 0xFFFFFFFF;
							++newVertexNum;
							continue;
						}
					}
					break;
				} while (true);

				for (uint32_t i = 0; i < 3; ++i)
				{
					if (*pNewIndex[i] == 0xFFFFFFFF)
					{
						if (currentVertexNum == KVirtualGeometryDefine::MAX_CLUSTER_VERTEX)
						{
							return false;
						}
						newToOld[currentVertexNum] = oldIndex[i];
						*pNewIndex[i] = currentVertexNum++;
					}

					optimized[3 * triIndex + i] = *pNewIndex[i];
				}

				assert(currentVertexNum == newVertexNum);

				++triIndex;
			}
		}
	}

	std::vector<KMeshProcessorVertex> oldVertices;
	oldVertices.resize(currentVertexNum);
	oldVertices.swap(vertices);

	for (uint32_t v = 0; v < currentVertexNum; ++v)
	{
		uint32_t oldVertex = newToOld[v];
		vertices[v] = oldVertices[oldVertex];
	}

	for (size_t i = 0; i < indices.size(); ++i)
	{
		indices[i] = optimized[i];
	}

	EnsureIndexOrder();

	return true;
}

void KMeshCluster::PostInit()
{
	InitBound();
	InitMaterial();
}

void KMeshCluster::CopyProperty(const KMeshCluster& cluster)
{
	lodBound = cluster.lodBound;
	groupIndex = cluster.groupIndex;
	generatingGroupIndex = cluster.generatingGroupIndex;
	index = cluster.index;
	level = cluster.level;
	lodError = cluster.lodError;
	edgeLength = cluster.edgeLength;
	color = cluster.color;
}

void KMeshCluster::Init(KMeshClusterPtr* clusters, uint32_t numClusters)
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
	materialIndices.reserve(sumIndexNum / 3);

	std::unordered_map<size_t, uint32_t> vertexMap;

	for (uint32_t i = 0; i < numClusters; ++i)
	{
		KMeshCluster& cluster = *clusters[i];
		for (uint32_t idx = 0; idx < (uint32_t)cluster.indices.size(); ++idx)
		{
			const uint32_t index = cluster.indices[idx];
			const uint32_t triangleIndex = (uint32_t)idx / 3;

			const uint32_t materialIndex = cluster.materialIndices[triangleIndex];
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
			if (idx % 3 == 0)
			{
				materialIndices.push_back(materialIndex);
			}
		}

		lodError = std::max(lodError, cluster.lodError);
		edgeLength = std::max(edgeLength, cluster.edgeLength);
	}

	vertices.shrink_to_fit();
	indices.shrink_to_fit();
	materialIndices.shrink_to_fit();

	PostInit();
}

void KMeshCluster::Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<uint32_t>& inIndices, const std::vector<uint32_t>& inMaterialIndices)
{
	UnInit();

	color = RandomColor();
	vertices = inVertices;
	indices = inIndices;
	materialIndices = inMaterialIndices;

	PostInit();
}

void KMeshCluster::Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<uint32_t>& inIndices, const std::vector<uint32_t>& inMaterialIndices, const KRange& range)
{
	UnInit();

	color = RandomColor();

	uint32_t num = range.end - range.begin + 1;

	vertices.reserve(num);
	indices.reserve(num);
	materialIndices.reserve(num / 3);
	assert(num % 3 == 0);

	std::unordered_map<uint32_t, uint32_t> indexMap;

	for (uint32_t i = 0; i < num; ++i)
	{
		uint32_t idx = i + range.begin;
		const uint32_t triIndex = idx / 3;

		uint32_t index = inIndices[idx];
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

		if (i % 3 == 0)
		{
			const uint32_t materialIndex = inMaterialIndices[triIndex];
			materialIndices.push_back(materialIndex);
		}
	}

	vertices.shrink_to_fit();
	indices.shrink_to_fit();

	PostInit();
}

void KMeshCluster::Init(const std::vector<KMeshProcessorVertex>& inVertices, const std::vector<Triangle>& inTriangles, const std::vector<idx_t>& inTriIndices, const std::vector<uint32_t>& inMaterialIndices, const KRange& range)
{
	UnInit();

	color = RandomColor();

	size_t num = range.end - range.begin + 1;

	vertices.reserve(num);
	indices.reserve(num * 3);
	materialIndices.reserve(num);

	std::unordered_map<uint32_t, uint32_t> indexMap;

	uint32_t begin = range.begin;
	uint32_t end = range.end;
	for (uint32_t idx = begin; idx <= end; ++idx)
	{
		const uint32_t triIndex = inTriIndices[idx];
		const uint32_t materialIndex = inMaterialIndices[triIndex];
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
		materialIndices.push_back(materialIndex);
	}

	vertices.shrink_to_fit();
	indices.shrink_to_fit();

	PostInit();
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
		edgeHash.Init();

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

		for (size_t triIndex = 0; triIndex < numTriangles; ++triIndex)
		{
			std::vector<idx_t> triangleAdjacencies;
			triangleAdjacencies.reserve(3);
			for (size_t i = 0; i < 3; ++i)
			{
				size_t v0 = context.triangles[triIndex].index[i];
				size_t v1 = context.triangles[triIndex].index[(i + 1) % 3];
				edgeHash.ForEachTri(v1, v0, [triIndex, &triangleAdjacencies](size_t adjTriIndex)
				{
					if (triIndex != adjTriIndex)
					{
						triangleAdjacencies.push_back((idx_t)adjTriIndex);
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
		KMeshClusterPtr cluster = KMeshClusterPtr(KNEW KMeshCluster());
		cluster->Init(context.vertices, context.triangles, partitioner.indices, context.materialIndices, range);
		m_Clusters.push_back(std::move(cluster));
	}
}

bool KMeshTriangleClusterBuilder::UnInit()
{
	m_Clusters.clear();
	return true;
}

bool KMeshTriangleClusterBuilder::Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices, uint32_t minPartitionNum, uint32_t maxPartitionNum)
{
	UnInit();
	m_MinPartitionNum = minPartitionNum;
	m_MaxPartitionNum = maxPartitionNum;
	Adjacency adjacency;
	adjacency.materialIndices = materialIndices;
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

	KMeshClusterPtr mergedCluster = KMeshClusterPtr(KNEW KMeshCluster());
	mergedCluster->Init(m_Clusters.data() + childrenBegin, numChildren);

	uint32_t numParent = KMath::DivideAndRoundUp((uint32_t)mergedCluster->indices.size(), (uint32_t)(6 * m_MaxPartitionNum));
	m_Clusters.reserve(m_Clusters.size() + numParent);

	uint32_t minTargetTriangleNum = m_MaxPartitionNum * numParent / 2;
	KMeshSimplification simplification;
	simplification.Init(mergedCluster->vertices, mergedCluster->indices, mergedCluster->materialIndices, 3, minTargetTriangleNum);

	std::vector<KMeshProcessorVertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> materialIndices;

	float maxParentError = 0;
	float maxParentEdgeLength = 0;
	KAABBBox parentLodBound;
	uint32_t parentBegin = (uint32_t)m_Clusters.size();

	for (uint32_t partitionNum = m_MaxPartitionNum - 2; partitionNum >= m_MaxPartitionNum / 2; partitionNum -= 2)
	{
		uint32_t targetTriangleNum = partitionNum * numParent;
		if (!simplification.Simplify(MeshSimplifyTarget::TRIANGLE, targetTriangleNum, vertices, indices, materialIndices, maxParentError))
		{
			continue;
		}

		if (numParent == 1)
		{
			mergedCluster->Init(vertices, indices, materialIndices);
			m_Clusters.push_back(mergedCluster);
			break;
		}

		KMeshTriangleClusterBuilder builder;
		if (!builder.Init(vertices, indices, materialIndices, m_MinPartitionNum, m_MaxPartitionNum))
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

		KMeshClusterGroupPtr newGroup = KMeshClusterGroupPtr(KNEW KMeshClusterGroup());
		newGroup->level = level;
		newGroup->index = (uint32_t)m_ClusterGroups.size();
		newGroup->color = KMeshCluster::RandomColor();

		newGroup->clusters.resize(numChildren);
		for (uint32_t i = 0; i < numChildren; ++i)
		{
			uint32_t idx = childrenBegin + i;
			m_Clusters[idx]->groupIndex = newGroup->index;

			maxParentEdgeLength = std::max(maxParentEdgeLength, m_Clusters[idx]->edgeLength);
			maxParentError = std::max(maxParentError, m_Clusters[idx]->lodError);
			parentLodBound = parentLodBound.Merge(m_Clusters[idx]->lodBound);

			newGroup->bound = newGroup->bound.Merge(m_Clusters[idx]->bound);
			// 这里要这样获取index 因为图划分后m_Clusters被重排了
			newGroup->clusters[i] = m_Clusters[idx]->index;
		}

		newGroup->generatingClusters.resize(numParent);
		for (uint32_t i = 0; i < numParent; ++i)
		{
			uint32_t idx = parentBegin + i;
			m_Clusters[idx]->index = parentBegin + i;

			m_Clusters[idx]->edgeLength = maxParentEdgeLength;
			m_Clusters[idx]->lodError = maxParentError;
			m_Clusters[idx]->lodBound = parentLodBound;

			m_Clusters[idx]->level = newGroup->level;
			m_Clusters[idx]->generatingGroupIndex = newGroup->index;

			newGroup->generatingClusters[i] = idx;
		}

		newGroup->edgeLength = maxParentEdgeLength;
		newGroup->maxParentError = maxParentError;
		newGroup->parentLodBound = parentLodBound;

		m_ClusterGroups.push_back(newGroup);
	}
}

void KVirtualGeometryBuilder::ClusterTriangle(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices)
{
	KMeshTriangleClusterBuilder builder;
	builder.Init(vertices, indices, materialIndices, m_MinPartitionNum, m_MaxPartitionNum);
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
	edgeHash.Init();

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

	for (size_t v0 = 0; v0 < verticesNum; ++v0)
	{
		edgeHash.ForEach(v0, [v0, &clusterAdj, &edgeHash](KPositionHashKey v1, size_t clusterID)
		{
			edgeHash.ForEachTri(v1, v0, [&clusterAdj, clusterID](size_t anotherClusterID)
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

bool KVirtualGeometryBuilder::ColorDebugClusters(const std::vector<KMeshClusterPtr>& clusters, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices)
{
	vertices.clear();
	indices.clear();
	materialIndices.clear();

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
		materialIndices.insert(materialIndices.end(), cluster.materialIndices.begin(), cluster.materialIndices.end());

		clusterIndexBegin += (uint32_t)clusterVertices.size();
	}

	return true;
}

bool KVirtualGeometryBuilder::ColorDebugClusterGroups(const std::vector<KMeshClusterPtr>& clusters, const std::vector<KMeshClusterGroupPtr>& groups, const std::vector<uint32_t>& ids, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices)
{
	vertices.clear();
	indices.clear();
	materialIndices.clear();

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
			materialIndices.insert(materialIndices.end(), cluster.materialIndices.begin(), cluster.materialIndices.end());

			clusterIndexBegin += (uint32_t)clusterVertices.size();
		}
	}

	return true;
}

void KVirtualGeometryBuilder::BuildDAG(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices, uint32_t minPartitionNum, uint32_t maxPartitionNum, uint32_t minClusterGroup, uint32_t maxClusterGroup)
{
	m_MinPartitionNum = minPartitionNum;
	m_MaxPartitionNum = maxPartitionNum;
	m_MinClusterGroup = minClusterGroup;
	m_MaxClusterGroup = maxClusterGroup;

	m_MaxPartitionNum = std::min(m_MaxPartitionNum, KVirtualGeometryDefine::MAX_CLUSTER_TRIANGLE);
	m_MaxClusterGroup = std::min(m_MaxClusterGroup, KVirtualGeometryDefine::MAX_CLUSTER_GROUP);

	m_MinPartitionNum = std::min(m_MinPartitionNum, m_MaxPartitionNum);
	m_MaxClusterGroup = std::min(m_MaxClusterGroup, m_MaxClusterGroup);

	m_LevelNum = 0;

	ClusterTriangle(vertices, indices, materialIndices);

	uint32_t levelClusterBegin = 0;
	uint32_t levelClusterNum = (uint32_t)m_Clusters.size();
	uint32_t currentLevel = 0;

	for (uint32_t i = 0; i < (uint32_t)m_Clusters.size(); ++i)
	{
		m_Clusters[i]->index = i;
	}

	while (levelClusterNum > 1)
	{
		uint32_t levelClusterEnd = levelClusterBegin + levelClusterNum - 1;

		if (levelClusterNum <= m_MaxClusterGroup)
		{
			uint32_t newLevelBegin = (uint32_t)m_Clusters.size();
			DAGReduce(levelClusterBegin, levelClusterEnd, currentLevel);

			if (m_Clusters.size() > newLevelBegin)
			{
				levelClusterBegin = newLevelBegin;
				levelClusterNum = (uint32_t)(m_Clusters.size() - levelClusterBegin);
				++currentLevel;
				continue;
			}
			else
			{
				break;
			}			
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

		uint32_t newLevelBegin = (uint32_t)m_Clusters.size();
		uint32_t newLevelGroupBegin = (uint32_t)m_ClusterGroups.size();

		for (const KRange& range : partitioner.ranges)
		{
			uint32_t clusterBegin = levelClusterBegin + range.begin;
			uint32_t clusterEnd = levelClusterBegin + range.end;
			size_t prevClusterNum = m_Clusters.size();
			DAGReduce(clusterBegin, clusterEnd, currentLevel);
			// Reduce fail
			if (prevClusterNum == m_Clusters.size())
			{
				m_Clusters.resize(newLevelBegin);
				m_ClusterGroups.resize(newLevelGroupBegin);
				break;
			}
		}

		if (m_Clusters.size() > newLevelBegin)
		{
			levelClusterBegin = newLevelBegin;
			levelClusterNum = (uint32_t)m_Clusters.size() - levelClusterBegin;
			++currentLevel;
		}
		else
		{
			break;
		}
	}

	{
		KMeshClusterGroupPtr rootGroup = KMeshClusterGroupPtr(KNEW KMeshClusterGroup());
		rootGroup->level = currentLevel;
		rootGroup->clusters.resize(levelClusterNum);
		rootGroup->index = (uint32_t)m_ClusterGroups.size();
		rootGroup->color = KMeshCluster::RandomColor();
		rootGroup->maxParentError = std::numeric_limits<float>::max();
		for (uint32_t i = 0; i < levelClusterNum; ++i)
		{
			uint32_t idx = levelClusterBegin + i;
			m_Clusters[idx]->index = idx;
			m_Clusters[idx]->groupIndex = rootGroup->index;
			rootGroup->clusters[i] = m_Clusters[idx]->index;
			rootGroup->parentLodBound = rootGroup->parentLodBound.Merge(m_Clusters[idx]->lodBound);
			rootGroup->bound = rootGroup->bound.Merge(m_Clusters[idx]->bound);
		}
			
		m_ClusterGroups.push_back(rootGroup);
	}

	std::sort(m_Clusters.begin(), m_Clusters.end(), [](const KMeshClusterPtr& lhs, const KMeshClusterPtr& rhs) -> bool { return lhs->index < rhs->index; });

	m_LevelNum = currentLevel;

	m_MinTriangleNum = 0;
	m_MaxTriangleNum = 0;
	m_MaxError = 0;

	for (size_t i = m_ClusterGroups.size(); i >= 1; --i)
	{
		uint32_t groupIndex = (uint32_t)i - 1;
		const KMeshClusterGroup& group = *m_ClusterGroups[groupIndex];
		for (uint32_t clusterIndex : group.clusters)
		{
			KMeshClusterPtr cluster = m_Clusters[clusterIndex];
			assert(cluster->groupIndex == groupIndex);
			if (groupIndex == (m_ClusterGroups.size() - 1))
			{
				m_MinTriangleNum += (uint32_t)cluster->indices.size() / 3;
			}
			if (cluster->generatingGroupIndex == KVirtualGeometryDefine::INVALID_INDEX)
			{
				assert(cluster->level == 0);
				m_MaxTriangleNum += (uint32_t)cluster->indices.size() / 3;
			}
			m_MaxError = glm::max(m_MaxError, cluster->lodError);
		}
	}
}

void KVirtualGeometryBuilder::BuildMaterialRanges()
{
	for (size_t i = 0; i < m_Clusters.size(); ++i)
	{
		m_Clusters[i]->BuildMaterialRange();
	}
}

void KVirtualGeometryBuilder::SortClusterGroup()
{
	struct Morton
	{
		uint32_t index;
		uint32_t code;
	};

	std::vector<uint32_t> groupRemap;
	std::vector<Morton> morton;
	groupRemap.resize(m_ClusterGroups.size());
	morton.resize(m_ClusterGroups.size());

	KAABBBox fullBound;

	for (uint32_t i = 0; i < (uint32_t)m_ClusterGroups.size(); ++i)
	{
		assert(m_ClusterGroups[i]->index == i);
		fullBound = fullBound.Merge(m_ClusterGroups[i]->bound);
	}

	for (uint32_t i = 0; i < (uint32_t)m_ClusterGroups.size(); ++i)
	{
		glm::uvec3 center = 1023.0f * (m_ClusterGroups[i]->bound.GetCenter() - fullBound.GetMin()) / fullBound.GetExtend();
		morton[i].index = (uint32_t)i;
		morton[i].code = KMath::MortonCode3(center[0]) | (KMath::MortonCode3(center[1]) << 1) | (KMath::MortonCode3(center[2]) << 2);
		glm::uvec3 rcenter = glm::uvec3(KMath::ReverseMortonCode3(morton[i].code), KMath::ReverseMortonCode3(morton[i].code >> 1), KMath::ReverseMortonCode3(morton[i].code >> 2));
		assert(center[0] == rcenter[0] && center[1] == rcenter[1] && center[2] == rcenter[2]);
	}

	std::sort(m_ClusterGroups.begin(), m_ClusterGroups.end(), [&morton](KMeshClusterGroupPtr lhs, KMeshClusterGroupPtr rhs)
	{
		if (lhs->level != rhs->level)
		{
			return lhs->level > rhs->level;
		}
		else
		{
			return morton[lhs->index].code < morton[rhs->index].code;
		}
	});

	for (uint32_t i = 0; i < (uint32_t)m_ClusterGroups.size(); ++i)
	{
		groupRemap[m_ClusterGroups[i]->index] = i;
		m_ClusterGroups[i]->index = i;
	}

	for (size_t i = 0; i < m_Clusters.size(); ++i)
	{
		KMeshClusterPtr cluster = m_Clusters[i];
		if (cluster->groupIndex != KVirtualGeometryDefine::INVALID_INDEX)
		{
			cluster->groupIndex = groupRemap[cluster->groupIndex];
		}
		if (cluster->generatingGroupIndex != KVirtualGeometryDefine::INVALID_INDEX)
		{
			cluster->generatingGroupIndex = groupRemap[cluster->generatingGroupIndex];
		}
	}

	for (uint32_t i = 0; i < (uint32_t)m_ClusterGroups.size(); ++i)
	{
		KMeshClusterGroupPtr group = m_ClusterGroups[i];
		morton.resize(group->clusters.size());
		const KAABBBox& groupBound = group->bound;

		for (size_t j = 0; j < group->clusters.size(); ++j)
		{
			uint32_t clusterIndex = group->clusters[j];
			glm::uvec3 center = 1023.0f * (m_Clusters[clusterIndex]->bound.GetCenter() - fullBound.GetMin()) / fullBound.GetExtend();
			morton[j].index = (uint32_t)j;
			morton[j].code = KMath::MortonCode3(center[0]) | (KMath::MortonCode3(center[1]) << 1) | (KMath::MortonCode3(center[2]) << 2);
			glm::uvec3 rcenter = glm::uvec3(KMath::ReverseMortonCode3(morton[j].code), KMath::ReverseMortonCode3(morton[j].code >> 1), KMath::ReverseMortonCode3(morton[j].code >> 2));
			assert(center[0] == rcenter[0] && center[1] == rcenter[1] && center[2] == rcenter[2]);
		}

		std::sort(morton.begin(), morton.end(), [](const Morton& lhs, const Morton& rhs)
		{
			return lhs.code < rhs.code;
		});

		std::vector<uint32_t> prevClusters = group->clusters;
		for (size_t j = 0; j < group->clusters.size(); ++j)
		{
			group->clusters[j] = prevClusters[morton[j].index];
		}
	}
}

void KVirtualGeometryBuilder::ConstrainCluster()
{
	for (size_t i = 0; i < m_Clusters.size(); ++i)
	{
		while (m_Clusters[i]->vertices.size() > m_MaxClusterVertex)
		{
			KMeshClusterPtr cluster = m_Clusters[i];
			uint32_t newClusterIndicesNum = 3 * ((uint32_t)cluster->indices.size() / 6);
			if (newClusterIndicesNum > 0)
			{
				KMeshClusterPtr newClusterA = KMeshClusterPtr(new KMeshCluster());
				newClusterA->Init(cluster->vertices, cluster->indices, cluster->materialIndices, KRange(0, newClusterIndicesNum - 1));
				KMeshClusterPtr newClusterB = KMeshClusterPtr(new KMeshCluster());
				newClusterB->Init(cluster->vertices, cluster->indices, cluster->materialIndices, KRange(newClusterIndicesNum, (uint32_t)cluster->indices.size() - 1));

				newClusterA->CopyProperty(*cluster);
				newClusterA->BuildMaterialRange();
				newClusterB->CopyProperty(*cluster);
				newClusterB->BuildMaterialRange();

				newClusterA->index = cluster->index;
				newClusterB->index = (uint32_t)m_Clusters.size();

				KMeshClusterGroupPtr group = m_ClusterGroups[cluster->groupIndex];

				m_Clusters[i] = newClusterA;
				m_Clusters.push_back(newClusterB);

				group->clusters.push_back(newClusterB->index);

				if (cluster->generatingGroupIndex != KVirtualGeometryDefine::INVALID_INDEX)
				{
					KMeshClusterGroupPtr generatingGroup = m_ClusterGroups[cluster->generatingGroupIndex];
					generatingGroup->generatingClusters.push_back(newClusterB->index);
				}
			}
		}
	}

	for (size_t i = 0; i < m_Clusters.size(); ++i)
	{
		assert(m_Clusters[i]->index == i);
	}
}

void KVirtualGeometryBuilder::BuildReuseBatch()
{
	for (size_t i = 0; i < m_Clusters.size(); ++i)
	{
		KMeshClusterPtr cluster = m_Clusters[i];
		for (KMeshClusterMaterialRange& materialRange : cluster->materialRanges)
		{
			std::vector<bool> vertexUseBits;
			vertexUseBits.resize(materialRange.length * 3);

			auto ResetUseBit = [&vertexUseBits]()
			{
				for (size_t i = 0; i < vertexUseBits.size(); ++i)
				{
					vertexUseBits[i] = false;
				}
			};

			ResetUseBit();

			uint32_t batchVertexNum = 0;
			uint32_t batchTriangleNum = 0;

			materialRange.batchTriCounts.clear();

			for (uint32_t i = 0; i < materialRange.length; ++i)
			{
				uint32_t triIndex = materialRange.start + i;
				uint32_t v[3] = { 0 };

				uint32_t newVertexNum = 0;
				for (size_t k = 0; k < 3; ++k)
				{
					v[k] = cluster->indices[3 * triIndex + k];
					newVertexNum += !vertexUseBits[v[k]];
				}

				if (batchVertexNum + newVertexNum > KVirtualGeometryDefine::MAX_CLUSTER_REUSE_BATCH)
				{
					materialRange.batchTriCounts.push_back(batchTriangleNum);
					batchVertexNum = 0;
					batchTriangleNum = 0;
					--i;
					ResetUseBit();
					continue;
				}

				for (size_t k = 0; k < 3; ++k)
				{
					vertexUseBits[v[k]] = true;
				}

				batchVertexNum += newVertexNum;
				batchTriangleNum += 1;

				if (batchTriangleNum == KVirtualGeometryDefine::MAX_CLUSTER_REUSE_BATCH)
				{
					materialRange.batchTriCounts.push_back(batchTriangleNum);
					batchVertexNum = 0;
					batchTriangleNum = 0;
					ResetUseBit();
				}
			}

			if (batchTriangleNum > 0)
			{
				materialRange.batchTriCounts.push_back(batchTriangleNum);
			}
		}
	}
}

void KVirtualGeometryBuilder::BuildPage()
{
	m_Pages.pages.clear();
	m_Pages.pages.push_back(KVirtualGeometryPage());

	m_ClusterGroupParts.clear();
	m_ClusterGroupParts.reserve(m_ClusterGroups.size());

	KVirtualGeometryPage* currentPage = &(*m_Pages.pages.rbegin());
	currentPage->dataByteSize = sizeof(uint32_t) * 4;
	currentPage->isRootPage = true;

	KMeshClusterGroupPartPtr currentPart = nullptr;

	m_Pages.numRootPage = 1;

	for (uint32_t groupIndex = 0; groupIndex < (uint32_t)m_ClusterGroups.size(); ++groupIndex)
	{
		KMeshClusterGroup& group = *m_ClusterGroups[groupIndex];

		for (uint32_t localClusterIndex = 0; localClusterIndex < (uint32_t)group.clusters.size(); ++localClusterIndex)
		{
			KMeshClusterPtr cluster = m_Clusters[group.clusters[localClusterIndex]];
			uint32_t clusterByteSize = 0;
			clusterByteSize += KVirtualGeometryEncoding::BYTE_SIZE_PER_VERTEX * (uint32_t)cluster->vertices.size();
			clusterByteSize += KVirtualGeometryEncoding::BYTE_SIZE_PER_INDEX * (uint32_t)cluster->indices.size();
			for (const KMeshClusterMaterialRange& materialRange : cluster->materialRanges)
			{
				clusterByteSize += KVirtualGeometryEncoding::BYTE_PER_MATERIAL * (uint32_t)materialRange.batchTriCounts.size();
			}
			clusterByteSize += KVirtualGeometryEncoding::BYTE_PER_CLUSTER_BATCH;

			uint32_t maxPageSize = currentPage->isRootPage ? KVirtualGeometryDefine::MAX_ROOT_PAGE_SIZE : KVirtualGeometryDefine::MAX_STREAMING_PAGE_SIZE;

			assert(clusterByteSize <= maxPageSize);

			if ((currentPage->clusterGroupPartNum + 1 > KVirtualGeometryDefine::MAX_CLUSTER_PART_IN_PAGE)
				|| (currentPage->clusterNum + 1 > KVirtualGeometryDefine::MAX_CLUSTER_IN_PAGE)
				|| (currentPage->dataByteSize + clusterByteSize > maxPageSize)
				)
			{
				m_Pages.pages.push_back(KVirtualGeometryPage());
				currentPage = &(*m_Pages.pages.rbegin());
				currentPage->dataByteSize = sizeof(uint32_t) * 4;
				currentPage->isRootPage = group.level == m_LevelNum;
				m_Pages.numRootPage += currentPage->isRootPage;
				currentPart = nullptr;
			}

			if (currentPage->clusterGroupPartNum == 0)
			{
				currentPage->clusterGroupPartStart = (uint32_t)m_ClusterGroupParts.size();
			}

			if (!currentPart || currentPart->groupIndex != groupIndex)
			{
				m_ClusterGroupParts.push_back(KMeshClusterGroupPartPtr(KNEW KMeshClusterGroupPart()));
				currentPart = *m_ClusterGroupParts.rbegin();
				currentPart->index = (uint32_t)m_ClusterGroupParts.size() - 1;

				currentPart->groupIndex = groupIndex;
				currentPart->level = group.level;

				currentPart->lodBound = group.parentLodBound;
				currentPart->lodError = group.maxParentError;
				currentPart->pageIndex = (uint32_t)m_Pages.pages.size() - 1;
				currentPart->clusterStart = currentPage->clusterNum;

				++currentPage->clusterGroupPartNum;
			}

			cluster->partIndex = currentPart->index;
			cluster->offsetInPart = currentPart->clusterNum;

			currentPart->clusters.push_back(m_Clusters[group.clusters[localClusterIndex]]);
			currentPage->dataByteSize += clusterByteSize;
			++currentPart->clusterNum;
			++currentPage->clusterNum;

			if (localClusterIndex == 0)
			{
				group.pageStart = currentPart->pageIndex;
				group.partStart = cluster->partIndex;
			}
			if (localClusterIndex == (uint32_t)group.clusters.size() - 1)
			{
				group.pageEnd = currentPart->pageIndex;
				group.partEnd = cluster->partIndex;
			}
		}
	}
}

void KVirtualGeometryBuilder::BuildFixup()
{
	auto ComputeClusterFixupHash = [](const KVirtualGeomertyClusterFixup& fixup) -> uint32_t
	{
		uint32_t hash = 0;
		KHash::HashCombine(hash, fixup.fixupPage);
		KHash::HashCombine(hash, fixup.dependencyPageStart);
		KHash::HashCombine(hash, fixup.dependencyPageEnd);
		KHash::HashCombine(hash, fixup.clusterIndexInPage);
		return hash;
	};

	auto ComputeHierarchyFixupHash = [](const KVirtualGeomertyHierarchyFixup& fixup) -> uint32_t
	{
		uint32_t hash = 0;
		KHash::HashCombine(hash, fixup.fixupPage);
		KHash::HashCombine(hash, fixup.dependencyPageStart);
		KHash::HashCombine(hash, fixup.dependencyPageEnd);
		KHash::HashCombine(hash, fixup.partIndex);
		return hash;
	};

	std::vector<std::unordered_set<uint32_t>> clusterFixupHashes;
	std::vector<std::unordered_set<uint32_t>> hierarchyFixupHashes;

	clusterFixupHashes.resize(m_Pages.pages.size());
	hierarchyFixupHashes.resize(m_Pages.pages.size());

	m_PageFixup.clusterFixups.resize(m_Pages.pages.size());
	for (uint32_t partIndex = 0; partIndex < (uint32_t)m_ClusterGroupParts.size(); ++partIndex)
	{
		KMeshClusterGroupPartPtr part = m_ClusterGroupParts[partIndex];
		KMeshClusterGroupPtr group = m_ClusterGroups[part->groupIndex];
		for (KMeshClusterPtr cluster : part->clusters)
		{
			if (cluster->generatingGroupIndex != -1)
			{
				KMeshClusterGroupPtr generatingGroup = m_ClusterGroups[cluster->generatingGroupIndex];
				KVirtualGeomertyClusterFixup newFixup;
				newFixup.fixupPage = part->pageIndex;
				newFixup.dependencyPageStart = generatingGroup->pageStart;
				newFixup.dependencyPageEnd = generatingGroup->pageEnd;
				newFixup.clusterIndexInPage = part->clusterStart + cluster->offsetInPart;

				for (uint32_t page = generatingGroup->pageStart; page <= generatingGroup->pageEnd; ++page)
				{
					uint32_t hash = ComputeClusterFixupHash(newFixup);
					if (clusterFixupHashes[page].find(hash) == clusterFixupHashes[page].end())
					{
						m_PageFixup.clusterFixups[page].push_back(newFixup);
						clusterFixupHashes[page].insert(hash);
					}
				}
			}
		}
	}

	m_PageFixup.hierarchyFixups.resize(m_Pages.pages.size());
	for (uint32_t partIndex = 0; partIndex < (uint32_t)m_ClusterGroupParts.size(); ++partIndex)
	{
		KMeshClusterGroupPartPtr part = m_ClusterGroupParts[partIndex];
		KMeshClusterGroup& group = *m_ClusterGroups[part->groupIndex];
		for (uint32_t partIndex2 = 0; partIndex2 < (uint32_t)m_ClusterGroupParts.size(); ++partIndex2)
		{
			KMeshClusterGroupPartPtr part2 = m_ClusterGroupParts[partIndex2];
			if (part->groupIndex == part2->groupIndex)
			{
				KVirtualGeomertyHierarchyFixup newFixup;
				newFixup.fixupPage = part2->pageIndex;
				newFixup.dependencyPageStart = group.pageStart;
				newFixup.dependencyPageEnd = group.pageEnd;
				newFixup.partIndex = partIndex2;

				uint32_t page = part->pageIndex;
				uint32_t hash = ComputeHierarchyFixupHash(newFixup);
				if (hierarchyFixupHashes[page].find(hash) == hierarchyFixupHashes[page].end())
				{
					m_PageFixup.hierarchyFixups[page].push_back(newFixup);
					hierarchyFixupHashes[page].insert(hash);
				}
			}
		}
	}
}

void KVirtualGeometryBuilder::BuildPageDependency(uint32_t pageIndex, KVirtualGeomertyPageDependency& dependency)
{
	const std::vector<KVirtualGeomertyClusterFixup>& clusterFixups = m_PageFixup.clusterFixups[pageIndex];
	for (const KVirtualGeomertyClusterFixup& fixup : clusterFixups)
	{
		if (fixup.fixupPage != pageIndex)
		{
			auto it = std::find(dependency.dependencies.begin(), dependency.dependencies.end(), fixup.fixupPage);
			if (it == dependency.dependencies.end())
			{
				dependency.dependencies.push_back(fixup.fixupPage);
			}
		}
	}
}

void KVirtualGeometryBuilder::BuildPageStorage()
{
	m_PageStorages.storages.resize(m_Pages.pages.size());

	for (uint32_t i = 0; i < (uint32_t)m_Pages.pages.size(); ++i)
	{
		KVirtualGeometryPageStorage& pageStorage = m_PageStorages.storages[i];
		const KVirtualGeometryPage& page = m_Pages.pages[i];

		size_t maxClusterNum = page.clusterGroupPartNum * m_MaxClusterGroup;

		pageStorage.batchStorage.batches.reserve(maxClusterNum);
		pageStorage.vertexStorage.vertices.reserve(maxClusterNum * m_MaxClusterVertex * KVirtualGeometryEncoding::FLOAT_PER_VERTEX);
		pageStorage.indexStorage.indices.reserve(maxClusterNum * m_MaxPartitionNum * 3);

		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;
		uint32_t mateiralOffset = 0;

		for (uint32_t localPartIndex = 0; localPartIndex < page.clusterGroupPartNum; ++localPartIndex)
		{
			uint32_t partIndex = page.clusterGroupPartStart + localPartIndex;
			KMeshClusterGroupPartPtr clustersPart = m_ClusterGroupParts[partIndex];

			for (KMeshClusterPtr cluster : clustersPart->clusters)
			{
				KMeshClusterBatch newBatch;

				newBatch.vertexFloatOffset = vertexOffset;
				newBatch.indexIntOffset = indexOffset;
				newBatch.materialIntOffset = mateiralOffset;
				newBatch.partIndex = partIndex;
				newBatch.lodBoundCenterError = glm::vec4(cluster->lodBound.GetCenter(), cluster->lodError);
				newBatch.lodBoundHalfExtendRadius = glm::vec4(0.5f * cluster->lodBound.GetExtend(), 0.5f * glm::length(cluster->lodBound.GetExtend()));

				KMeshClusterGroupPtr group = m_ClusterGroups[cluster->groupIndex];

				newBatch.parentBoundCenterError = glm::vec4(group->parentLodBound.GetCenter(), group->maxParentError);
				newBatch.parentBoundHalfExtendRadius = glm::vec4(0.5f * group->parentLodBound.GetExtend(), 0.5f * glm::length(group->parentLodBound.GetExtend()));

				newBatch.triangleNum = (uint32_t)cluster->indices.size() / 3;

				for (const KMeshProcessorVertex& vertex : cluster->vertices)
				{
					pageStorage.vertexStorage.vertices.emplace_back(vertex.pos[0]);
					pageStorage.vertexStorage.vertices.emplace_back(vertex.pos[1]);
					pageStorage.vertexStorage.vertices.emplace_back(vertex.pos[2]);

					pageStorage.vertexStorage.vertices.emplace_back(vertex.normal[0]);
					pageStorage.vertexStorage.vertices.emplace_back(vertex.normal[1]);
					pageStorage.vertexStorage.vertices.emplace_back(vertex.normal[2]);

					pageStorage.vertexStorage.vertices.emplace_back(vertex.uv[0]);
					pageStorage.vertexStorage.vertices.emplace_back(vertex.uv[1]);
				}

				pageStorage.indexStorage.indices.insert(pageStorage.indexStorage.indices.end(), cluster->indices.begin(), cluster->indices.end());

				const std::vector<KMeshClusterMaterialRange>& materialRanges = cluster->materialRanges;
				uint32_t batchNum = 0;

				pageStorage.materialStorage.materials.reserve(pageStorage.materialStorage.materials.size() + KVirtualGeometryEncoding::INT_PER_MATERIAL * materialRanges.size());

				for (const KMeshClusterMaterialRange& materialRange : materialRanges)
				{
					uint32_t batchBegin = materialRange.start;
					for (uint32_t batchTriCount : materialRange.batchTriCounts)
					{
						pageStorage.materialStorage.materials.push_back(materialRange.materialIndex);
						pageStorage.materialStorage.materials.push_back(batchBegin);
						pageStorage.materialStorage.materials.push_back(batchBegin + batchTriCount - 1);
						batchBegin += batchTriCount;
						++batchNum;
					}
				}

				newBatch.leaf = true;
				newBatch.batchNum = batchNum;
				pageStorage.batchStorage.batches.push_back(newBatch);

				vertexOffset = (uint32_t)pageStorage.vertexStorage.vertices.size();
				indexOffset = (uint32_t)pageStorage.indexStorage.indices.size();
				mateiralOffset = (uint32_t)pageStorage.materialStorage.materials.size();
			}
		}

		uint32_t pageSize = sizeof(uint32_t) * 4;
		pageStorage.vertexStorageByteOffset = pageSize;
		pageSize += sizeof(float) * (uint32_t)pageStorage.vertexStorage.vertices.size();
		pageStorage.indexStorageByteOffset = pageSize;
		pageSize += sizeof(uint32_t) * (uint32_t)pageStorage.indexStorage.indices.size();
		pageStorage.materialStorageByteOffset = pageSize;
		pageSize += sizeof(uint32_t) * (uint32_t)pageStorage.materialStorage.materials.size();
		pageStorage.batchStorageByteOffset = pageSize;
		pageSize += sizeof(KMeshClusterBatch) * (uint32_t)pageStorage.batchStorage.batches.size();
		assert(pageSize == page.dataByteSize);
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
			return bvhNodes[lhs]->lodBound.GetCenter().x < bvhNodes[lhs]->lodBound.GetCenter().x;
		}
		else if (axis == AXIS_Y)
		{
			return bvhNodes[lhs]->lodBound.GetCenter().y < bvhNodes[lhs]->lodBound.GetCenter().y;
		}
		else
		{
			return bvhNodes[lhs]->lodBound.GetCenter().z < bvhNodes[lhs]->lodBound.GetCenter().z;
		}
	});

	uint32_t num = range.end - range.begin + 1;
	uint32_t halfNum = (range.end - range.begin + 1) / 2;

	KAABBBox halfBound[2];

	for (uint32_t i = 0; i < halfNum; ++i)
	{
		halfBound[0] = halfBound[0].Merge(bvhNodes[sorted[i]]->lodBound);
		halfBound[1] = halfBound[1].Merge(bvhNodes[sorted[i + halfNum]]->lodBound);
	}

	for (uint32_t i = 2 * halfNum; i < num; ++i)
	{
		halfBound[1] = halfBound[1].Merge(bvhNodes[sorted[i]]->lodBound);
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
	KMeshClusterBVHNodePtr root = KMeshClusterBVHNodePtr(KNEW KMeshClusterBVHNode());
	root->partIndex = KVirtualGeometryDefine::INVALID_INDEX;
	root->lodError = 0;
	bvhNodes.push_back(root);

	if (nodeNum <= KVirtualGeometryDefine::MAX_BVH_NODES)
	{
		for (uint32_t childIndex : indices)
		{
			root->lodBound = root->lodBound.Merge(bvhNodes[childIndex]->lodBound);
			root->lodError = std::max(root->lodError, bvhNodes[childIndex]->lodError);
		}
		root->children = indices;
		return rootIndex;
	}
	else
	{
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
			root->lodBound = root->lodBound.Merge(bvhNodes[childIndex]->lodBound);
			root->lodError = std::max(root->lodError, bvhNodes[childIndex]->lodError);
			childOffset += childNum;
		}
	}

	return rootIndex;
}

void KVirtualGeometryBuilder::BuildClusterBVH()
{
	std::vector<KMeshClusterBVHNodePtr> bvhNodes;
	bvhNodes.resize(m_ClusterGroupParts.size());

	for (uint32_t partIndex = 0; partIndex < (uint32_t)m_ClusterGroupParts.size(); ++partIndex)
	{
		KMeshClusterBVHNodePtr newLeaf = KMeshClusterBVHNodePtr(KNEW KMeshClusterBVHNode());
		KMeshClusterGroupPartPtr clusterPart = m_ClusterGroupParts[partIndex];
		newLeaf->partIndex = partIndex;
		newLeaf->lodBound = clusterPart->lodBound;
		newLeaf->lodError = clusterPart->lodError;
		bvhNodes[partIndex] = newLeaf;
	}

	uint32_t rootIndex = 0;

	if (bvhNodes.size() == 1)
	{
		KMeshClusterBVHNodePtr root = KMeshClusterBVHNodePtr(KNEW KMeshClusterBVHNode());
		root->children = { 0 };
		bvhNodes.push_back(root);
		rootIndex = 1;
	}
	else
	{
		uint32_t maxLevel = 0;
		for (KMeshClusterBVHNodePtr node : bvhNodes)
		{
			if (m_ClusterGroupParts[node->partIndex]->level > maxLevel)
			{
				maxLevel = m_ClusterGroupParts[node->partIndex]->level;
			}
		}

		std::vector<std::vector<uint32_t>> indicesByLevel;
		indicesByLevel.resize(maxLevel + 1);

		for (uint32_t index = 0; index < (uint32_t)bvhNodes.size(); ++index)
		{
			KMeshClusterBVHNodePtr node = bvhNodes[index];
			uint32_t level = m_ClusterGroupParts[node->partIndex]->level;
			indicesByLevel[level].push_back(index);
		}

		std::vector<uint32_t> roots;
		roots.reserve(maxLevel);

		for (uint32_t level = 0; level <= maxLevel; ++level)
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

void KVirtualGeometryBuilder::RecurselyVisitBVH(uint32_t index, std::function<void(uint32_t index)> visitFunc)
{
	visitFunc(index);

	if (m_BVHNodes[index]->partIndex == KVirtualGeometryDefine::INVALID_INDEX)
	{
		for (uint32_t child : m_BVHNodes[index]->children)
		{
			RecurselyVisitBVH(child, visitFunc);
		}
	}
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

	if (m_ClusterGroups.size() > 0)
	{
		size_t groupIndex = m_ClusterGroups.size() - 1;
		const KMeshClusterGroup& group = *m_ClusterGroups[groupIndex];
		for (uint32_t clusterIndex : group.clusters)
		{
			KMeshClusterPtr cluster = m_Clusters[clusterIndex];
			DAGCutElement element(clusterIndex, cluster->lodError);
			clusterHeap.push(element);
			curTriangleCount += (uint32_t)cluster->indices.size() / 3;
			clusterInHeap[clusterIndex] = true;
		}
	}

	float minError = std::numeric_limits<float>::max();
	float curError = -1;

	while (!clusterHeap.empty())
	{
		DAGCutElement element = clusterHeap.top();

		KMeshClusterPtr cluster = m_Clusters[element.clusterIndex];
		curError = element.error;
		assert(curError == cluster->lodError);

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
		for (uint32_t clusterIndex : group.clusters)
		{
			if (clusterInHeap[clusterIndex])
			{
				continue;
			}
			DAGCutElement element(clusterIndex, m_Clusters[clusterIndex]->lodError);
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

void KVirtualGeometryBuilder::ColorDebugDAGCut(uint32_t targetTriangleCount, float targetError, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices, uint32_t& triangleCount, float& error) const
{
	std::vector<uint32_t> clusterIndices;
	FindDAGCut(targetTriangleCount, targetError, clusterIndices, triangleCount, error);
	ColorDebugClusters(m_Clusters, clusterIndices, vertices, indices, materialIndices);
}

void KVirtualGeometryBuilder::ColorDebugCluster(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices) const
{
	vertices.clear();
	indices.clear();
	materialIndices.clear();

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

	ColorDebugClusters(m_Clusters, clusterIndices, vertices, indices, materialIndices);
}

void KVirtualGeometryBuilder::ColorDebugClusterGroup(uint32_t level, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices) const
{
	vertices.clear();
	indices.clear();
	materialIndices.clear();

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

	ColorDebugClusterGroups(m_Clusters, m_ClusterGroups, groupIndices, vertices, indices, materialIndices);
}

void KVirtualGeometryBuilder::GetAllBVHBounds(std::vector<KAABBBox>& bounds)
{
	bounds.clear();
	if (m_BVHRoot != KVirtualGeometryDefine::INVALID_INDEX)
	{
		bounds.reserve(m_BVHNodes.size());
		RecurselyVisitBVH(m_BVHRoot, [this, &bounds](uint32_t nodeIndex)
		{
			bounds.push_back(m_BVHNodes[nodeIndex]->lodBound);
		});
	}
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

			assert(cluster->groupIndex != KVirtualGeometryDefine::INVALID_INDEX);
			if (cluster->groupIndex != KVirtualGeometryDefine::INVALID_INDEX)
			{
				const KMeshClusterGroup& groupAsChildren = *m_ClusterGroups[cluster->groupIndex];

				{
					size_t t = 0;
					std::stringstream ssParents;
					ssParents << "\"";
					for (uint32_t parent : groupAsChildren.generatingClusters)
					{
						++t;
						ssParents << parent;
						if (t != groupAsChildren.generatingClusters.size())
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
					for (uint32_t adj : groupAsChildren.clusters)
					{
						if (adj == i)
						{
							continue;
						}
						++t;
						ssAdjacencies << adj;
						if (t != groupAsChildren.clusters.size() - 1)
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
					for (uint32_t child : groupAsParent.clusters)
					{
						++t;
						ssChildren << child;
						if (t != groupAsParent.clusters.size())
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
					for (uint32_t adj : groupAsParent.generatingClusters)
					{
						if (adj == i)
						{
							continue;
						}
						++t;
						ssAdjacencies << adj;
						if (t != groupAsParent.generatingClusters.size() - 1)
						{
							ssAdjacencies << ",";
						}
					}
					ssAdjacencies << "\"";
					adjacenciesAsParent = ssAdjacencies.str();
				}
			}

			ss << i << "," << cluster->level << "," << (cluster->groupIndex == KVirtualGeometryDefine::INVALID_INDEX) << "," << cluster->indices.size() / 3 << "," << cluster->lodError << "," << adjacenciesAsChildren << "," << adjacenciesAsParent << "," << children << "," << parents;
			ss << std::endl;
		}

		std::string data = ss.str();
		dataStream->Write(data.c_str(), data.length());
		dataStream->Close();
	}
}

void KVirtualGeometryBuilder::BuildStreaming()
{
	m_PageDependencies.pageDependencies.resize(m_Pages.pages.size());
	for (uint32_t pageIndex = 0; pageIndex < (uint32_t)m_Pages.pages.size(); ++pageIndex)
	{
		BuildPageDependency(pageIndex, m_PageDependencies.pageDependencies[pageIndex]);
	}

	m_PageClusters.clusters.resize(m_Clusters.size());
	for (size_t clusterIndex = 0; clusterIndex < m_Clusters.size(); ++clusterIndex)
	{
		m_PageClusters.clusters[clusterIndex].partIndex = m_Clusters[clusterIndex]->partIndex;
		m_PageClusters.clusters[clusterIndex].offsetInPart = m_Clusters[clusterIndex]->offsetInPart;
	}

	m_PageClusters.groups.resize(m_ClusterGroups.size());
	for (size_t groupIndex = 0; groupIndex < m_ClusterGroups.size(); ++groupIndex)
	{
		m_PageClusters.groups[groupIndex].groupPartStart = m_ClusterGroups[groupIndex]->partStart;
		m_PageClusters.groups[groupIndex].groupPartEnd = m_ClusterGroups[groupIndex]->partEnd;
	}

	m_PageClusters.parts.resize(m_ClusterGroupParts.size());
	for (size_t partIndex = 0; partIndex < m_ClusterGroupParts.size(); ++partIndex)
	{
		m_PageClusters.parts[partIndex].hierarchyIndex = m_ClusterGroupParts[partIndex]->hierarchyIndex;
		m_PageClusters.parts[partIndex].level = m_ClusterGroupParts[partIndex]->level;
	}
}

void KVirtualGeometryBuilder::Build(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices)
{
	BuildDAG(vertices, indices, materialIndices, 124, 128, 4, 32);
	BuildMaterialRanges();
	ConstrainCluster();
	SortClusterGroup();
	BuildReuseBatch();
	BuildPage();
	BuildFixup();
	BuildPageStorage();
	BuildClusterBVH();
	BuildMeshClusterHierarchies(m_BVHRoot);
	BuildStreaming();

	m_Bound.SetNull();
	for (const KMeshProcessorVertex& vertex : vertices)
	{
		m_Bound = m_Bound.Merge(vertex.pos);
	}
}

bool KVirtualGeometryBuilder::GetPages(KVirtualGeometryPages& pages, KVirtualGeometryPageStorages& pageStorages, KVirtualGeomertyFixup& pageFixup, KVirtualGeomertyPageDependencies& pageDependencies, KVirtualGeomertyPageClustersData& pageClusters) const
{
	pages = m_Pages;
	pageStorages = m_PageStorages;
	pageFixup = m_PageFixup;
	pageDependencies = m_PageDependencies;
	pageClusters = m_PageClusters;
	return true;
}

bool KVirtualGeometryBuilder::GetMeshClusterStorages(KMeshClusterBatchStorage& batchStorage, KMeshClustersVertexStorage& vertexStorage, KMeshClustersIndexStorage& indexStorage, KMeshClustersMaterialStorage& materialStorage) const
{
	batchStorage.batches.clear();
	vertexStorage.vertices.clear();
	indexStorage.indices.clear();
	materialStorage.materials.clear();

	for (size_t pageIndex = 0; pageIndex < m_Pages.pages.size(); ++pageIndex)
	{
		const KVirtualGeometryPage& page = m_Pages.pages[pageIndex];
		const KVirtualGeometryPageStorage& pageStorage = m_PageStorages.storages[pageIndex];

		batchStorage.batches.insert(batchStorage.batches.end(), pageStorage.batchStorage.batches.begin(), pageStorage.batchStorage.batches.end());
		vertexStorage.vertices.insert(vertexStorage.vertices.end(), pageStorage.vertexStorage.vertices.begin(), pageStorage.vertexStorage.vertices.end());
		indexStorage.indices.insert(indexStorage.indices.end(), pageStorage.indexStorage.indices.begin(), pageStorage.indexStorage.indices.end());
		materialStorage.materials.insert(materialStorage.materials.end(), pageStorage.materialStorage.materials.begin(), pageStorage.materialStorage.materials.end());
	}

	uint32_t vertexFloatOffset = (m_Pages.pages.size() > 0) ? (uint32_t)m_PageStorages.storages[0].vertexStorage.vertices.size() : 0;
	uint32_t indexIntOffset = (m_Pages.pages.size() > 0) ? (uint32_t)m_PageStorages.storages[0].indexStorage.indices.size() : 0;
	uint32_t materialIntOffset = (m_Pages.pages.size() > 0) ? (uint32_t)m_PageStorages.storages[0].materialStorage.materials.size() : 0;
	uint32_t batchOffset = (m_Pages.pages.size() > 0) ? (uint32_t)m_PageStorages.storages[0].batchStorage.batches.size() : 0;
	for (size_t pageIndex = 1; pageIndex < m_Pages.pages.size(); ++pageIndex)
	{
		const KVirtualGeometryPage& page = m_Pages.pages[pageIndex];
		const KVirtualGeometryPageStorage& pageStorage = m_PageStorages.storages[pageIndex];
		uint32_t pageBatchSize = (uint32_t)pageStorage.batchStorage.batches.size();

		for (uint32_t localBatchIndex = 0; localBatchIndex < pageBatchSize; ++localBatchIndex)
		{
			size_t batchIndex = batchOffset + localBatchIndex;
			batchStorage.batches[batchIndex].vertexFloatOffset += vertexFloatOffset;
			batchStorage.batches[batchIndex].indexIntOffset += indexIntOffset;
			batchStorage.batches[batchIndex].materialIntOffset += materialIntOffset;
		}
		vertexFloatOffset += (uint32_t)m_PageStorages.storages[pageIndex].vertexStorage.vertices.size();
		indexIntOffset += (uint32_t)m_PageStorages.storages[pageIndex].indexStorage.indices.size();
		materialIntOffset += (uint32_t)m_PageStorages.storages[pageIndex].materialStorage.materials.size();
		batchOffset += (uint32_t)m_PageStorages.storages[pageIndex].batchStorage.batches.size();
	}

	return true;
}

uint32_t KVirtualGeometryBuilder::BuildMeshClusterHierarchies(uint32_t index)
{
	uint32_t hierarchyIndex = (uint32_t)m_Hierarchies.size();
	m_Hierarchies.push_back(KMeshClusterHierarchy());

	KMeshClusterHierarchy newHierarchy = m_Hierarchies[hierarchyIndex];

	KMeshClusterBVHNodePtr bvhNode = m_BVHNodes[index];
	newHierarchy.partIndex = bvhNode->partIndex;
	for (uint32_t i = 0; i < KVirtualGeometryDefine::MAX_BVH_NODES; ++i)
	{
		newHierarchy.children[i] = KVirtualGeometryDefine::INVALID_INDEX;
	}

	KAABBBox lodBound;
	float maxError = 0;

	if (newHierarchy.partIndex == KVirtualGeometryDefine::INVALID_INDEX)
	{
		for (uint32_t i = 0; i < KVirtualGeometryDefine::MAX_BVH_NODES; ++i)
		{
			if (i < bvhNode->children.size())
			{
				uint32_t childIndex = BuildMeshClusterHierarchies(bvhNode->children[i]);
				newHierarchy.children[i] = childIndex;

				KAABBBox childBound;
				const glm::vec4& lodBoundCenterError = m_Hierarchies[childIndex].lodBoundCenterError;
				const glm::vec4& lodBoundHalfExtendRadius = m_Hierarchies[childIndex].lodBoundHalfExtendRadius;
				childBound.InitFromHalfExtent(glm::vec3(lodBoundCenterError.x, lodBoundCenterError.y, lodBoundCenterError.z), glm::vec3(lodBoundHalfExtendRadius.x, lodBoundHalfExtendRadius.y, lodBoundHalfExtendRadius.z));

				lodBound = lodBound.Merge(childBound);
				maxError = std::max(maxError, lodBoundCenterError.w);
			}
		}
	}
	else
	{
		KMeshClusterGroupPartPtr part = m_ClusterGroupParts[bvhNode->partIndex];
		KMeshClusterGroupPtr group = m_ClusterGroups[part->groupIndex];

		lodBound = group->parentLodBound;
		maxError = group->maxParentError;

		part->hierarchyIndex = hierarchyIndex;
	}

	newHierarchy.lodBoundCenterError = glm::vec4(lodBound.GetCenter(), maxError);
	newHierarchy.lodBoundHalfExtendRadius = glm::vec4(0.5f * lodBound.GetExtend(), 0.5f * glm::length(lodBound.GetExtend()));

	m_Hierarchies[hierarchyIndex] = std::move(newHierarchy);
	return hierarchyIndex;
}

bool KVirtualGeometryBuilder::GetMeshClusterHierarchies(std::vector<KMeshClusterHierarchy>& hierarchies) const
{
	hierarchies = m_Hierarchies;
	return true;
}

bool KVirtualGeometryBuilder::GetMeshClusterGroupParts(std::vector<KMeshClusterPtr>& clusters, std::vector<KMeshClusterGroupPartPtr>& parts, std::vector<KMeshClusterGroupPtr>& groups) const
{
	clusters = m_Clusters;
	parts = m_ClusterGroupParts;
	groups = m_ClusterGroups;
	return true;
}