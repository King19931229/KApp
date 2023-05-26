#include "Publish/Mesh/KMeshProcessor.h"
#include "Publish/Mesh/KMeshSimplification.h"
#include <set>

namespace KMeshProcessor
{
	bool CalcTBN(std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		uint32_t vertexCount	= (uint32_t)vertices.size();
		uint32_t indexCount		= (uint32_t)indices.size();

		std::vector<uint32_t> counters;
		counters.resize(vertices.size());
		assert(indexCount % 3 == 0);

		for (uint32_t tri = 0; tri < indexCount / 3; ++tri)
		{
			for (uint32_t idx = 0; idx < 3; ++idx)
			{
				uint32_t idx0 = indices[tri * 3 + idx];
				uint32_t idx1 = indices[tri * 3 + (idx + 1) % 3];
				uint32_t idx2 = indices[tri * 3 + (idx + 2) % 3];

				uint32_t arrayIdx = idx0;

				glm::vec2 uv0 = vertices[idx0].uv;
				glm::vec2 uv1 = vertices[idx1].uv;
				glm::vec2 uv2 = vertices[idx2].uv;

				glm::vec3 e1 = vertices[idx1].pos - vertices[idx0].pos;
				glm::vec3 e2 = vertices[idx2].pos - vertices[idx0].pos;

				float delta_u1 = uv1[0] - uv0[0];
				float delta_u2 = uv2[0] - uv0[0];
				float delta_v1 = uv1[1] - uv0[1];
				float delta_v2 = uv2[1] - uv0[1];

				bool needRandomTangent = false;

				glm::vec3 tangent, bitangent, normal;

				// We have delta uv
				if (abs(delta_v1 * delta_u2 - delta_v2 * delta_u1) > 1e-2f)
				{
					tangent = (delta_v1 * e2 - delta_v2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
					bitangent = (-delta_u1 * e2 + delta_u2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);

					if (glm::length(tangent) > 1e-2f && glm::length(bitangent) > 1e-2f)
					{
						tangent = glm::normalize(tangent);
						bitangent = glm::normalize(bitangent);
					}
					else
					{
						needRandomTangent = true;
					}
				}
				else
				{
					needRandomTangent = true;
				}

				if (needRandomTangent)
				{
					normal = glm::normalize(glm::cross(e1, e2));
					if (abs(glm::dot(normal, glm::vec3(1.0f, 0.0f, 0.0f)) > 1.0f - 1e-2f))
					{
						tangent = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
					}
					else
					{
						tangent = glm::normalize(glm::cross(normal, glm::vec3(1.0f, 0.0f, 0.0f)));
					}
					bitangent = glm::normalize(glm::cross(normal, tangent));
				}
				else
				{
					normal = glm::normalize(glm::cross(tangent, bitangent));
				}

				// https://blog.csdn.net/n5/article/details/104215400
				float cosine = glm::dot(glm::normalize(e1), glm::normalize(e2));
				float areaWeight = glm::length(e1) * glm::length(e2) * sqrtf(1.0f - cosine * cosine);
				float angleWeight = acosf(cosine);
				float weight = needRandomTangent ? 1.0f : areaWeight * angleWeight;

				vertices[arrayIdx].normal	+= normal * weight;
				vertices[arrayIdx].tangent	+= tangent * weight;
				vertices[arrayIdx].binormal	+= bitangent * weight;
				counters[arrayIdx]			+= 1;
			}
		}

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			assert(counters[i] > 0);
			vertices[i].tangent		= glm::normalize(vertices[i].tangent);
			vertices[i].binormal	= glm::normalize(vertices[i].binormal);
			vertices[i].normal		= glm::normalize(vertices[i].normal);
		}

		return true;
	}

	bool Simplify(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices,
		MeshSimplifyTarget target, uint32_t targetCount,
		std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices)
	{
		KMeshSimplification simplification;
		if (simplification.Init(oldVertices, oldIndices))
		{
			return simplification.Simplification(target, targetCount, newVertices, newIndices);
		}
		return false;
	}

	bool ConvertForMeshProcessor(const KAssetImportResult& input, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
	{
		auto FindPNTIndex = [](const std::vector<KAssetVertexComponentGroup>& group) -> int32_t
		{
			for (int32_t i = 0; i < (int32_t)group.size(); ++i)
			{
				const KAssetVertexComponentGroup& componentGroup = group[i];
				if (componentGroup.size() == 3)
				{
					if (componentGroup[0] == AVC_POSITION_3F && componentGroup[1] == AVC_NORMAL_3F && componentGroup[2] == AVC_UV_2F)
					{
						return i;
					}
				}
			}
			return -1;
		};

		int32_t vertexPositionIndex = FindPNTIndex(input.components);
		if (vertexPositionIndex < 0)
		{
			return false;
		}

		uint32_t totalVertexCount = 0;
		uint32_t totalIndexCount = 0;

		for (size_t partIndex = 0; partIndex < input.parts.size(); ++partIndex)
		{
			const KAssetImportResult::ModelPart& part = input.parts[partIndex];
			if (part.vertexCount == 0 || part.indexCount == 0 || part.indexCount % 3 != 0)
			{
				return false;
			}
			totalVertexCount += part.vertexCount;
			totalIndexCount += part.indexCount;
		}

		vertices.resize(totalVertexCount);
		indices.resize(totalIndexCount);

		struct VertexPositionLayout
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		for (size_t partIndex = 0; partIndex < input.parts.size(); ++partIndex)
		{
			const KAssetImportResult::ModelPart& part = input.parts[partIndex];
			const KAssetImportResult::VertexDataBuffer& vertexData = input.verticesDatas[vertexPositionIndex];

			uint32_t indexBase = part.indexBase;
			uint32_t indexCount = part.indexCount;

			uint32_t vertexBase = part.vertexBase;
			uint32_t vertexCount = part.vertexCount;

			uint32_t indexMin = std::numeric_limits<uint32_t>::max();

			if (input.index16Bit)
			{
				const uint16_t* pIndices = (const uint16_t*)input.indicesData.data();
				pIndices += indexBase;
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					indices[i + indexBase] = pIndices[i];
				}
			}
			else
			{
				const uint32_t* pIndices = (const uint32_t*)input.indicesData.data();
				pIndices += indexBase;
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					indices[i + indexBase] = pIndices[i];
				}
			}

			const VertexPositionLayout* pVerticesData = (const VertexPositionLayout*)vertexData.data();
			pVerticesData += vertexBase;

			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				const VertexPositionLayout& srcVertex = pVerticesData[i];
				vertices[i + vertexBase].pos = srcVertex.pos;
				vertices[i + vertexBase].uv = srcVertex.uv;
				vertices[i + vertexBase].normal = srcVertex.normal;
				vertices[i + vertexBase].partIndex = (int32_t)partIndex;
			}
		}

		return true;
	}

	bool ConvertFromMeshProcessor(KAssetImportResult& output, const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, const std::vector<KAssetImportResult::Material>& originalMats)
	{
		int32_t maxPartIndex = -1;
		std::set<int32_t> allPartIndices;
		for (size_t i = 0; i < oldVertices.size(); ++i)
		{
			maxPartIndex = std::max(maxPartIndex, oldVertices[i].partIndex);
			allPartIndices.insert(oldVertices[i].partIndex);
		}
		std::vector<int32_t> sortMaterialIndices = std::vector<int32_t>(allPartIndices.begin(), allPartIndices.end());

		std::vector<int32_t> partIndexRemap;
		partIndexRemap.resize(maxPartIndex >= 0 ? (maxPartIndex + 1) : 0);
		for (int32_t i = 0; i < partIndexRemap.size(); ++i)
		{
			partIndexRemap[i] = -1;
		}
		for (int32_t i = 0; i < sortMaterialIndices.size(); ++i)
		{
			partIndexRemap[sortMaterialIndices[i]] = i;
		}

		std::vector<KAssetImportResult::ModelPart> parts;

		std::vector<std::vector<KMeshProcessorVertex>> verticesByMaterial;
		std::vector<std::vector<uint32_t>> indicesByMaterial;

		parts.resize(sortMaterialIndices.size());
		verticesByMaterial.resize(sortMaterialIndices.size());
		indicesByMaterial.resize(sortMaterialIndices.size());

		std::unordered_map<uint32_t, uint32_t> indexRemap;
		for (uint32_t index : oldIndices)
		{
			const KMeshProcessorVertex& vertex = oldVertices[index];
			int32_t partIndex = partIndexRemap[vertex.partIndex];
			KMeshProcessorVertex newVertex = vertex;
			newVertex.partIndex = partIndex;

			uint32_t newIndex = 0;

			auto it = indexRemap.find(index);
			if (it != indexRemap.end())
			{
				newIndex = it->second;
			}
			else
			{
				newIndex = (int32_t)indexRemap.size();
				indexRemap[index] = newIndex;
				verticesByMaterial[partIndex].push_back(newVertex);
			}

			indicesByMaterial[partIndex].push_back(newIndex);
		}

		std::vector<KMeshProcessorVertex> vertices;
		std::vector<uint32_t> indices;

		vertices.reserve(oldVertices.size());
		indices.reserve(oldIndices.size());

		for (size_t partIndex = 0; partIndex < parts.size(); ++partIndex)
		{
			vertices.insert(vertices.end(), verticesByMaterial[partIndex].begin(), verticesByMaterial[partIndex].end());
			indices.insert(indices.end(), indicesByMaterial[partIndex].begin(), indicesByMaterial[partIndex].end());
		}

		uint32_t indexBase = 0;
		uint32_t vertexBase = 0;

		for (size_t partIndex = 0; partIndex < parts.size(); ++partIndex)
		{
			KAssetImportResult::ModelPart& part = parts[partIndex];
			part.indexBase = indexBase;
			part.indexCount = (uint32_t)indicesByMaterial[partIndex].size();
			part.vertexBase = 0;
			part.vertexCount = (uint32_t)verticesByMaterial[partIndex].size();
			part.material = originalMats[partIndexRemap[partIndex]];

			indexBase += part.indexCount;
			vertexBase += part.vertexCount;
		}

		struct VertexPositionLayout
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		std::vector<VertexPositionLayout> outputVertices;
		outputVertices.resize(vertices.size());

		KAABBBox bound;
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			outputVertices[i].pos = vertices[i].pos;
			outputVertices[i].normal = vertices[i].normal;
			outputVertices[i].uv = vertices[i].uv;
			bound.Merge(vertices[i].pos, bound);
		}

		output.components = { { AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F } };
		output.parts = parts;
		output.vertexCount = (uint32_t)vertices.size();
		output.indexCount = (uint32_t)indices.size();

		output.index16Bit = false;
		output.indicesData.resize(sizeof(indices[0]) * indices.size());
		memcpy(output.indicesData.data(), indices.data(), output.indicesData.size());

		KAssetImportResult::VertexDataBuffer vertexBuffer;
		vertexBuffer.resize(sizeof(outputVertices[0]) * outputVertices.size());
		memcpy(vertexBuffer.data(), outputVertices.data(), vertexBuffer.size());

		output.verticesDatas = { vertexBuffer };

		output.extend.min[0] = bound.GetMin()[0];
		output.extend.min[1] = bound.GetMin()[1];
		output.extend.min[2] = bound.GetMin()[2];

		output.extend.max[0] = bound.GetMax()[0];
		output.extend.max[1] = bound.GetMax()[1];
		output.extend.max[2] = bound.GetMax()[2];

		return true;
	}
}