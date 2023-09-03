#include "Publish/Mesh/KMeshProcessor.h"
#include "Publish/Mesh/KMeshSimplification.h"
#include <set>

namespace KMeshProcessor
{
	struct PNT
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	int32_t FindPNTIndex(const std::vector<KAssetVertexComponentGroup>& group)
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

	int32_t FindColorIndex(const std::vector<KAssetVertexComponentGroup>& group, uint32_t colorIndex)
	{
		assert(colorIndex <= MAX_COLOR_CHANNEL - 1);
		AssetVertexComponent targetComponent = (AssetVertexComponent)(AVC_COLOR0_3F + colorIndex);

		if (targetComponent > AVC_COLOR5_3F)
		{
			return -1;
		}

		for (int32_t i = 0; i < (int32_t)group.size(); ++i)
		{
			const KAssetVertexComponentGroup& componentGroup = group[i];
			if (componentGroup.size() == 1)
			{
				if (componentGroup[0] == targetComponent)
				{
					return i;
				}
			}
		}

		return -1;
	}

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

	bool RemoveDuplicated(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices)
	{
		newVertices.clear();
		newIndices.clear();

		newVertices.reserve(oldVertices.size());
		newIndices.reserve(oldIndices.size());

		std::unordered_map<size_t, size_t> mapIndices;

		for (uint32_t index : oldIndices)
		{
			const KMeshProcessorVertex& vertex = oldVertices[index];
			size_t hash = KMeshProcessorVertexHash(vertex);
			auto it = mapIndices.find(hash);
			uint32_t newIndex = 0;
			if (it == mapIndices.end())
			{
				newIndex = (uint32_t)newVertices.size();
				mapIndices.insert({ hash, newIndex });
				newVertices.push_back(vertex);
			}
			else
			{
				newIndex = (uint32_t)it->second;
			}
			newIndices.push_back(newIndex);
		}

		return true;
	}

	bool RemoveEqual(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices)
	{
		struct MotornPair
		{
			uint32_t motron = -1;
			uint32_t index = -1;

			inline bool operator<(const MotornPair& rhs) const
			{
				return motron < rhs.motron;
			}
		};
		std::vector<MotornPair> motrons;

		{
			float maxPositionComponent = 0.0f;
			for (size_t i = 0; i < oldVertices.size(); ++i)
			{
				for (int32_t j = 0; j < 3; ++j)
				{
					float c = oldVertices[i].pos[j];
					maxPositionComponent = glm::max(maxPositionComponent, glm::abs(c));
				}
			}

			// float posScale = (maxPositionComponent > 1.0f) ? KMath::ScaleFactorToSameExponent(maxPositionComponent, 1.00f) : 1.0f;
			float posScale = (maxPositionComponent > 1e-5f) ? 1.0f / maxPositionComponent : 1.0f;

			motrons.resize(oldVertices.size());

			for (size_t i = 0; i < oldVertices.size(); ++i)
			{
				const glm::vec3& pos = oldVertices[i].pos;
				uint32_t z = uint32_t(1023 * posScale * (0.30f * pos.x + 0.33f * pos.y + 0.37f * pos.z));
				motrons[i].motron = KMath::MortonCode3(z);
				motrons[i].index = (uint32_t)i;
			}

			std::sort(motrons.begin(), motrons.end());
		}

		newVertices.clear();
		newVertices.reserve(oldVertices.size());
		newIndices.clear();
		newIndices.reserve(oldIndices.size());

		const int64_t maxSearchCount = 5;

		std::unordered_map<uint32_t, uint32_t> remap;
		for (int64_t i = 0; i < (int64_t)motrons.size(); ++i)
		{
			uint32_t newIndex = -1;

			const uint32_t oldIndex = motrons[i].index;
			for (int64_t j = i - 1, k = 0; j >= 0 && k < maxSearchCount; --j, ++k)
			{
				const uint32_t nearIndex = motrons[j].index;
				if (oldVertices[oldIndex].AlmostEqual(oldVertices[nearIndex]))
				{
					newIndex = remap[nearIndex];
					break;
				}
			}

			if (newIndex == -1)
			{
				newIndex = (uint32_t)newVertices.size();
				newVertices.push_back(oldVertices[oldIndex]);
			}

			remap[oldIndex] = newIndex;
		}

		for (size_t i = 0; i < oldIndices.size(); ++i)
		{
			uint32_t oldIndex = oldIndices[i];
			uint32_t newIndex = remap[oldIndex];
			newIndices.push_back(newIndex);
		}

		return true;
	}

	bool Simplify(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, const std::vector<uint32_t>& oldMaterialIndices,
		MeshSimplifyTarget target, uint32_t targetCount,
		std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices, std::vector<uint32_t>& newMaterialIndices,
		float& error)
	{
		KMeshSimplification simplification;
		if (simplification.Init(oldVertices, oldIndices, oldMaterialIndices, 1, 3))
		{
			return simplification.Simplify(target, targetCount, newVertices, newIndices, newMaterialIndices, error);
		}
		return false;
	}

	bool ConvertForMeshProcessor(const KMeshRawData& input, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices)
	{
		int32_t PNTIndex = FindPNTIndex(input.components);
		if (PNTIndex < 0)
		{
			return false;
		}

		int32_t colorIndex[MAX_COLOR_CHANNEL] = { -1 };
		for (uint32_t i = 0; i < MAX_COLOR_CHANNEL; ++i)
		{
			colorIndex[i] = FindColorIndex(input.components, i);
		}

		uint32_t totalVertexCount = 0;
		uint32_t totalIndexCount = 0;

		for (size_t partIndex = 0; partIndex < input.parts.size(); ++partIndex)
		{
			const KMeshRawData::ModelPart& part = input.parts[partIndex];
			if (part.vertexCount == 0 || part.indexCount == 0 || part.indexCount % 3 != 0)
			{
				return false;
			}
			totalVertexCount += part.vertexCount;
			totalIndexCount += part.indexCount;
		}

		vertices.resize(totalVertexCount);
		indices.resize(totalIndexCount);
		materialIndices.resize(totalIndexCount / 3);

		for (size_t partIndex = 0; partIndex < input.parts.size(); ++partIndex)
		{
			const KMeshRawData::ModelPart& part = input.parts[partIndex];

			uint32_t indexBase = part.indexBase;
			uint32_t indexCount = part.indexCount;

			uint32_t triangleBase = indexBase / 3;
			uint32_t triangleCount = indexCount / 3;

			uint32_t vertexBase = part.vertexBase;
			uint32_t vertexCount = part.vertexCount;

			constexpr uint32_t indexMin = std::numeric_limits<uint32_t>::max();

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

			for (uint32_t i = 0; i < triangleCount; ++i)
			{
				materialIndices[i + triangleBase] = (uint32_t)partIndex;
			}

			const KMeshRawData::VertexDataBuffer& PNTData = input.verticesDatas[PNTIndex];
			const PNT* pnts = (const PNT*)PNTData.data();
			pnts += vertexBase;

			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				const PNT& pnt = pnts[i];
				vertices[i + vertexBase].pos = pnt.pos;
				vertices[i + vertexBase].uv = pnt.uv;
				vertices[i + vertexBase].normal = pnt.normal;
			}

			for (uint32_t c = 0; c < 5; ++c)
			{
				if (colorIndex[c] >= 0)
				{
					const KMeshRawData::VertexDataBuffer& colorData = input.verticesDatas[colorIndex[c]];
					const glm::vec3* colors = (const glm::vec3*)colorData.data();
					colors += vertexBase;
					for (uint32_t i = 0; i < vertexCount; ++i)
					{
						vertices[i + vertexBase].color[c] = colors[i];
					}
				}
			}
		}

		return true;
	}

	bool ConvertFromMeshProcessor(KMeshRawData& output, const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, const std::vector<uint32_t>& oldMaterialIndices, const std::vector<KMeshRawData::Material>& originalMats)
	{
		std::vector<KMeshRawData::ModelPart> parts;
		parts.resize(originalMats.size());

		std::vector<KMeshProcessorVertex> vertices;
		std::vector<uint32_t> indices;

		vertices = oldVertices;

		std::vector<std::vector<uint32_t>> indicesByMaterial;
		indicesByMaterial.resize(originalMats.size());

		assert(oldMaterialIndices.size() * 3 == oldIndices.size());

		for (size_t triIndex = 0; triIndex < oldMaterialIndices.size(); ++triIndex)
		{
			uint32_t materialIndex = oldMaterialIndices[triIndex];
			if (materialIndex >= originalMats.size())
			{
				return false;
			}
			for (size_t i = 0; i < 3; ++i)
			{
				indicesByMaterial[materialIndex].push_back(oldIndices[3 * triIndex + i]);
			}
		}

		indices.resize(oldIndices.size());

		uint32_t indexBase = 0;
		uint32_t vertexBase = 0;

		for (size_t partIndex = 0; partIndex < parts.size(); ++partIndex)
		{
			KMeshRawData::ModelPart& part = parts[partIndex];
			const std::vector<uint32_t>& index = indicesByMaterial[partIndex];
			part.indexBase = indexBase;
			part.indexCount = (uint32_t)index.size();
			part.vertexBase = 0;
			part.vertexCount = (uint32_t)oldVertices.size();
			part.material = originalMats[partIndex];

			for (uint32_t i = 0; i < part.indexCount; ++i)
			{
				indices[part.indexBase + i] = index[i];
			}

			indexBase += part.indexCount;
			vertexBase += part.vertexCount;
		}

		KAABBBox bound;
		std::vector<KMeshRawData::VertexDataBuffer> vertexBuffers;
		std::vector<KAssetVertexComponentGroup> components;
		{
			KMeshRawData::VertexDataBuffer vertexBuffer;
			vertexBuffer.resize(sizeof(PNT) * vertices.size());
			PNT* pnts = (PNT*)vertexBuffer.data();
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				pnts[i].pos = vertices[i].pos;
				pnts[i].normal = vertices[i].normal;
				pnts[i].uv = vertices[i].uv;
				bound = bound.Merge(vertices[i].pos);
			}
			components.push_back({ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F });
			vertexBuffers.push_back(std::move(vertexBuffer));
		}

		for (uint32_t c = 0; c < MAX_COLOR_CHANNEL; ++c)
		{
			KMeshRawData::VertexDataBuffer vertexBuffer;
			vertexBuffer.resize(sizeof(glm::vec3)* vertices.size());
			glm::vec3* colors = (glm::vec3*)vertexBuffer.data();
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				colors[i] = vertices[i].color[c];
			}
			components.push_back({ (AssetVertexComponent)(AVC_COLOR0_3F + c) });
			vertexBuffers.push_back(std::move(vertexBuffer));
		}

		output.parts = parts;
		output.vertexCount = (uint32_t)vertices.size();
		output.indexCount = (uint32_t)indices.size();
		output.components = std::move(components);
		output.verticesDatas = std::move(vertexBuffers);

		output.index16Bit = false;
		output.indicesData.resize(sizeof(indices[0]) * indices.size());
		memcpy(output.indicesData.data(), indices.data(), output.indicesData.size());

		output.extend.min[0] = bound.GetMin()[0];
		output.extend.min[1] = bound.GetMin()[1];
		output.extend.min[2] = bound.GetMin()[2];

		output.extend.max[0] = bound.GetMax()[0];
		output.extend.max[1] = bound.GetMax()[1];
		output.extend.max[2] = bound.GetMax()[2];

		return true;
	}
}